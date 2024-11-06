
#ifndef SORT_C
#define SORT_C

#include "Sorting.h"
#include "IterComparator.h"
#include "MyDB_PageHandle.h"
#include "MyDB_PageListIteratorAlt.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_RecordIteratorAlt.h"
#include "MyDB_TableReaderWriter.h"
#include "MyDB_TableRecIterator.h"
#include "MyDB_TableRecIteratorAlt.h"
#include <cstddef>
#include <memory>
#include <queue>
#include <vector>

using namespace std;

void mergeIntoFile(MyDB_TableReaderWriter &TRW,
                   vector<MyDB_RecordIteratorAltPtr> &iters,
                   function<bool()> compareFunc, MyDB_RecordPtr lRec,
                   MyDB_RecordPtr rRec) {
    // learn something new here, priority_queue takes in the element type,
    // container and compare function (in our case its in our comparator)
    // but it only takes a type of that class, we can later put the object
    // into its constructor
    IterComparator myComparator(compareFunc, lRec, rRec);
    // now we have a priority queue (min heap)
    priority_queue<MyDB_RecordIteratorAltPtr, vector<MyDB_RecordIteratorAltPtr>,
                   IterComparator>
        queueIter(myComparator);
    
    // push everything into the queue
    for (auto recIterAltPtr : iters) {
        queueIter.push(recIterAltPtr);
    }
    MyDB_RecordPtr tempRec = TRW.getEmptyRecord();
    MyDB_RecordIteratorAltPtr tempIter;
    // get the iterator with smallest record
    while (!queueIter.empty()) {
        tempIter = queueIter.top();
        tempIter->getCurrent(tempRec);
        // append to the target table
        TRW.append(tempRec);
        queueIter.pop();
        // if have next, push back to the queue, otherwise just through it away
        if (tempIter->advance()) {
            queueIter.push(tempIter);
        }
    }
}

vector<MyDB_PageReaderWriter>
mergeIntoList(MyDB_BufferManagerPtr bufManager, MyDB_RecordIteratorAltPtr lIter,
              MyDB_RecordIteratorAltPtr rIter, function<bool()> compareFunc,
              MyDB_RecordPtr lRec, MyDB_RecordPtr rRec) {
    // return vector
    vector<MyDB_PageReaderWriter> ret;
    // get an anonymous page reader writer
    MyDB_PageReaderWriter tempPRW(*bufManager);
    // initialize
    lIter->getCurrent(lRec);
    rIter->getCurrent(rRec);
    // put them in a pair, for easier coding
    MyDB_RecordPtr recPair[2] = {rRec, lRec};
    MyDB_RecordIteratorAltPtr iterPair[2] = {rIter, lIter};
    bool which = compareFunc();
    // merge
    do {
        // read in records
        iterPair[which]->getCurrent(recPair[which]);
        // which = 1, if lRec < rRec and 0 otherwise
        // that gives us recPair[which] is the smaller one
        which = compareFunc();
        if (!tempPRW.append(recPair[which])) {
            // page is full, push in vec, get a new one
            ret.push_back(tempPRW);
            tempPRW = MyDB_PageReaderWriter(*bufManager);
            tempPRW.append(recPair[which]);
        }
    } while (iterPair[which]->advance()); // breaks when one of the iterator is out of record
    // here the other iterator might still has something
    do {
        iterPair[!which]->getCurrent(recPair[!which]);
        if (!tempPRW.append(recPair[!which])) {
            ret.push_back(tempPRW);
            tempPRW = MyDB_PageReaderWriter(*bufManager);
            tempPRW.append(recPair[!which]);
        }
    } while (iterPair[!which]->advance()); // breaks when one of the iterator is out of record
    // remember to push the last tempPRW
    ret.push_back(tempPRW);
    // here we should have our sorted list of pages
    return ret;
}

void sort(int runSize, MyDB_TableReaderWriter &fromTable,
          MyDB_TableReaderWriter &toTable, function<bool()> compareFunc,
          MyDB_RecordPtr lRec, MyDB_RecordPtr rRec) {
    int numPages = fromTable.getNumPages();
    // -----------------------------------------sortphase----------------------------------------- 
    //store pages in vector
    vector<vector<MyDB_PageReaderWriter>> run;
    vector<vector<MyDB_PageReaderWriter>> allRuns;
    // load run size of pages
    for (int i = 0; i < numPages; i++) {
        // sort this page into an anonymous page and apend that anonymous page in run
        run.push_back({*fromTable[i].sort(compareFunc, lRec, rRec)});
        // sort when hit the run size or this is the last batch of pages
        if (run.size() == runSize || i == numPages - 1) {
            // sort until only one list of pages
            while (run.size() != 1) {
                // sort every 2 adjacent pages, if we have odd runsize, leave it
                // for the next run
                vector<vector<MyDB_PageReaderWriter>> temp;
                for (int j = 0; j < run.size(); j += 2) {
                    // only happens when run.size() is an odd number, then save
                    // the extra last one for the next loop
                    if (j == run.size() - 1) {
                        temp.push_back(run[j]);
                    } else {
                        MyDB_RecordIteratorAltPtr lIter =
                            make_shared<MyDB_PageListIteratorAlt>(run[j]);
                        MyDB_RecordIteratorAltPtr rIter =
                            make_shared<MyDB_PageListIteratorAlt>(run[j + 1]);
                        // merge them into list, and push to the temp
                        temp.push_back(mergeIntoList(fromTable.getBufferMgr(),
                                                     lIter, rIter, compareFunc,
                                                     lRec, rRec));
                    }
                }
                run = temp;
            }
            // this run is sorted
            allRuns.push_back(run.front());
            // clear for the next run
            run.clear();
        }
    }
    // -----------------------------------------merge
    // phase----------------------------------------- transfer the runs into
    // iterators
    vector<MyDB_RecordIteratorAltPtr> iters;
    for (auto &r : allRuns) {
        iters.push_back(make_shared<MyDB_PageListIteratorAlt>(r));
    }
    // merge them into toTable
    mergeIntoFile(toTable, iters, compareFunc, lRec, rRec);
}

#endif
