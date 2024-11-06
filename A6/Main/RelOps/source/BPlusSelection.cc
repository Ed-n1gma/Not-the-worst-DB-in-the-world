
#ifndef BPLUS_SELECTION_C                                        
#define BPLUS_SELECTION_C

#include "BPlusSelection.h"
#include "MyDB_Record.h"

BPlusSelection :: BPlusSelection (MyDB_BPlusTreeReaderWriterPtr inputIn, MyDB_TableReaderWriterPtr outputIn,
                MyDB_AttValPtr lowIn, MyDB_AttValPtr highIn,
                string selectionPredicateIn, vector <string> projectionsIn) {
    input = inputIn;
    output = outputIn;
    low = lowIn;
    high = highIn;
    selectionPredicate = selectionPredicateIn;
    projections = projectionsIn;
}

void BPlusSelection :: run () {
    auto iter = input->getRangeIteratorAlt(low, high);
    MyDB_RecordPtr tempRec = input->getEmptyRecord();
    // predicate
    auto predicate = tempRec->compileComputation(selectionPredicate);
    // projections
    vector <func> finalComputations;
	for (string s : projections) {
		finalComputations.push_back (tempRec->compileComputation (s));
	}
    // this is the output record
	MyDB_RecordPtr outputRec = output->getEmptyRecord ();
    while (iter->advance()) {
        iter->getCurrent(tempRec);
        // check to see if it is accepted by the join predicate
        if (predicate ()->toBool ()) {

            // run all of the computations
            int i = 0;
            for (auto &f : finalComputations) {
                outputRec->getAtt (i++)->set (f());
            }

            // the record's content has changed because it 
            // is now a composite of two records whose content
            // has changed via a read... we have to tell it this,
            // or else the record's internal buffer may cause it
            // to write old values
            outputRec->recordContentHasChanged ();
            output->append (outputRec);	
        }
    }
}

#endif
