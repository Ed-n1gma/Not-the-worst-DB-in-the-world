
#ifndef AGG_CC
#define AGG_CC

#include "MyDB_AttVal.h"
#include "MyDB_BufferManager.h"
#include "MyDB_Record.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableReaderWriter.h"
#include "Aggregate.h"
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

Aggregate :: Aggregate (MyDB_TableReaderWriterPtr inputIn, MyDB_TableReaderWriterPtr outputIn,
		vector <pair <MyDB_AggType, string>> aggsToComputeIn,
		vector <string> groupingsIn, string selectionPredicateIn) {
    input = inputIn;
    output = outputIn;
    aggsToCompute = aggsToComputeIn;
    groupings = groupingsIn;
    selectionPredicate = selectionPredicateIn;
}

void Aggregate :: run () {
    // create the schema to fit the stats of group
    // there is only sum, count, avg
    // avg can be computed using count
    // we can design our stat to be constisting of only sum and count
    // and return sum / count if avg
    // save the first one to be count to make sure there is count to devide the sum
    MyDB_SchemaPtr mySchema = make_shared <MyDB_Schema> ();
    // don't know what to name this extra column without getting conflit with the att in inputTable
    // so i put my name on it
    int i = 0;
    // add the grouping att first
    for(i = 0; i < groupings.size(); i++) {
        auto t = output->getTable()->getSchema()->getAtts()[i];
        t.first = to_string(i);
        mySchema->appendAtt(t);
    }
	mySchema->appendAtt (make_pair ("ShuangWu", make_shared <MyDB_IntAttType> ()));
    i++;
    // simpily name it after the index of the aggs
	for(const auto& agg: aggsToCompute) {
        switch (agg.first) {
        case MyDB_AggType::sum:
            mySchema->appendAtt (make_pair (to_string(i++), make_shared <MyDB_DoubleAttType> ()));
            break;
        case MyDB_AggType::avg:
            mySchema->appendAtt (make_pair (to_string(i++), make_shared <MyDB_DoubleAttType> ()));
            break;
        default:
            mySchema->appendAtt (make_pair (to_string(i++), make_shared <MyDB_IntAttType> ()));
        }
    }
    
    // use this schema (template for group) to build a record
    MyDB_RecordPtr tempGroup = make_shared<MyDB_Record>(mySchema);
    // put group recs in pinned anounomous pages
    vector<MyDB_PageReaderWriter> pagesForGroups;
    MyDB_BufferManagerPtr myBufferMgr = input->getBufferMgr();
    // hash groupings to the location of the group recs in memeory
    unordered_map<size_t, void*> hashTable;
    // ready to iterate
    MyDB_RecordPtr tempRec = input->getEmptyRecord();
    auto iter = input->getIterator(tempRec);
    // filter
    auto predicate = tempRec->compileComputation(selectionPredicate);
    // make hashing fuction from grouping
    vector<func> hashingFuncs;
    for(const auto& g : groupings) {
        hashingFuncs.push_back(tempRec->compileComputation(g));
    }
    // to update the group rec I need to compile functions
    // and combine 2 records together so that the infomation in the tempRec will goes into the groups
	MyDB_SchemaPtr combinedSchema = make_shared <MyDB_Schema> ();
	for (auto &p : input->getTable ()->getSchema ()->getAtts ())
		combinedSchema->appendAtt (p);
	for (auto &p : mySchema->getAtts ())
		combinedSchema->appendAtt (p);

	// get the combined record
	MyDB_RecordPtr combinedRec = make_shared <MyDB_Record> (combinedSchema);
	combinedRec->buildFrom (tempRec, tempGroup);
    // initialization functions
    vector<func> initFuncs;
    // initialize to 1
    initFuncs.push_back(combinedRec->compileComputation("int [1]"));
    for(const auto& agg: aggsToCompute) {
        switch (agg.first) {
        case MyDB_AggType::cnt:
            initFuncs.push_back(combinedRec->compileComputation("int [1]"));
            break;
        default:
            // initialize to the grouping value of the first rec
            initFuncs.push_back(combinedRec->compileComputation(agg.second));
        }
    }
    // update functions when a rec got hashed to a existing group
    vector<func> updateFuncs;
    // increment the count
    i = groupings.size() + 1;
    updateFuncs.push_back(combinedRec->compileComputation("+ (int [1], [ShuangWu])"));
    for(const auto& agg: aggsToCompute) {
        switch (agg.first) {
        case MyDB_AggType::cnt:
            updateFuncs.push_back(combinedRec->compileComputation("+ (int [1], [" + to_string(i++) + "])"));
            break;
        default:
            updateFuncs.push_back(combinedRec->compileComputation("+ (" + agg.second + ", [" + to_string(i++) + "])"));
        }
    }
    MyDB_PageReaderWriter tempPage(true, *myBufferMgr);
    
    // iterate through
    while (iter->hasNext()) {
        iter->getNext();
        if (!predicate()->toBool()) continue;
        size_t hashVal = 0;
        // hash this rec
        for(const auto& f : hashingFuncs) {
            hashVal ^= f()->hash();
        }
        // already has a group, update it
        if (hashTable.find(hashVal) != hashTable.end()) {
            tempGroup->fromBinary(hashTable[hashVal]);
            i = groupings.size();
            for (auto &f : updateFuncs) {
                tempGroup->getAtt (i++)->set (f());
            }
            tempGroup->recordContentHasChanged();
            tempGroup->toBinary(hashTable[hashVal]);
        } else {
            // also initialize the group atts
            for(i = 0; i < groupings.size(); i++) {
                tempGroup->getAtt(i)->set(combinedRec->compileComputation(groupings[i])());
            }
            // initialize the group rec
            for (auto &f : initFuncs) {
                tempGroup->getAtt (i++)->set (f());
            }
            tempGroup->recordContentHasChanged();
            void* location = tempPage.appendAndReturnLocation(tempGroup);
            // append failed, is full
            if (location == nullptr) {
                // full need to get a new page
                pagesForGroups.push_back(tempPage);
                tempPage = MyDB_PageReaderWriter(true, *myBufferMgr);
                location = tempPage.appendAndReturnLocation(tempGroup);
            }
            hashTable[hashVal] = location;
        }
    }
    // output phase
    // final output function
    vector<func> finalFuncs;
    i = groupings.size() + 1;
    for(const auto& agg: aggsToCompute) {
        switch (agg.first) {
        case MyDB_AggType::avg:
            finalFuncs.push_back(tempGroup->compileComputation("/ ([" + to_string(i++) + "], [ShuangWu])"));
            break;
        default:
            // just out put this thing
            finalFuncs.push_back(tempGroup->compileComputation("[" + to_string(i++) + "]"));
        }
    }
    MyDB_RecordPtr outRec = output->getEmptyRecord();
    for(const auto& x : hashTable) {
        tempGroup->fromBinary(x.second);
        for(i = 0; i < groupings.size(); i++) {
            outRec->getAtt(i)->set(tempGroup->getAtt(i));
        }
        for(const auto& f : finalFuncs) {
            outRec->getAtt(i++)->set(f());
        }
        outRec->recordContentHasChanged();
        output->append(outRec);
    }
}

#endif

