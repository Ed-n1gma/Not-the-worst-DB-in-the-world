
#ifndef REG_SELECTION_C                                        
#define REG_SELECTION_C

#include "RegularSelection.h"
#include <cstddef>
#include <iostream>

RegularSelection :: RegularSelection (MyDB_TableReaderWriterPtr inputIn, MyDB_TableReaderWriterPtr outputIn,
                string selectionPredicateIn, vector <string> projectionsIn) {

	input = inputIn;
	output = outputIn;
	selectionPredicate = selectionPredicateIn;
	projections = projectionsIn;
}

void RegularSelection :: run () {

	MyDB_RecordPtr inputRec = input->getEmptyRecord ();
	MyDB_RecordPtr outputRec = output->getEmptyRecord ();
	
	// compile all of the coputations that we need here
	vector <func> finalComputations;
	for (string s : projections) {
		finalComputations.push_back (inputRec->compileComputation (s));
	}
	func pred = inputRec->compileComputation (selectionPredicate);

	// now, iterate through the B+-tree query results
	MyDB_RecordIteratorAltPtr myIter = input->getIteratorAlt ();
	size_t counter = 0;
	while (myIter->advance ()) {

		myIter->getCurrent (inputRec);
		counter++;
		// if (counter%100000 == 0) {
		// 	cout << "DEBUG: read " << counter << " records\n";
		// }

		// see if it is accepted by the predicate
		if (!pred()->toBool ()) {
			continue;
		}

		// run all of the computations
		int i = 0;
		for (auto &f : finalComputations) {
			outputRec->getAtt (i++)->set (f());
		}

		outputRec->recordContentHasChanged ();
		output->append (outputRec);
	}
}

#endif
