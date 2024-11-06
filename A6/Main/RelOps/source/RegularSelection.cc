
#ifndef REG_SELECTION_C                                        
#define REG_SELECTION_C

#include "RegularSelection.h"
#include "MyDB_Record.h"

RegularSelection :: RegularSelection (MyDB_TableReaderWriterPtr inputIn, MyDB_TableReaderWriterPtr outputIn,
		string selectionPredicateIn, vector <string> projectionsIn) {
    input = inputIn;
    output = outputIn;
    selectionPredicate = selectionPredicateIn;
    projections = projectionsIn;
}

void RegularSelection :: run () {
    MyDB_RecordPtr tempRec = input->getEmptyRecord();
    auto predicate = tempRec->compileComputation(selectionPredicate);
    vector <func> finalComputations;
	for (string s : projections) {
		finalComputations.push_back (tempRec->compileComputation (s));
	}
    auto iter = input->getIterator(tempRec);
    MyDB_RecordPtr outRec = output->getEmptyRecord();
    while (iter->hasNext()) {
        iter->getNext();
        if (predicate()->toBool()) {
            int i = 0;
            for (auto &f : finalComputations) {
                outRec->getAtt (i++)->set (f());
            }

            outRec->recordContentHasChanged ();
            output->append (outRec);
        }
    }
}

#endif
