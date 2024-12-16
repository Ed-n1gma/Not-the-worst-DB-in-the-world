
#ifndef LOG_OP_CC
#define LOG_OP_CC

#include "MyDB_LogicalOps.h"
#include "ExprTree.h"
#include "MyDB_BufferManager.h"
#include "MyDB_Table.h"
#include "MyDB_TableReaderWriter.h"
#include "RegularSelection.h"
#include "ScanJoin.h"
#include "SortMergeJoin.h"
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

void mergePred(const vector<ExprTreePtr>& pred, string& toMe) {
	toMe = "";
	if (pred.size() == 0) {
		toMe = "bool[true]";
	} else {
		// compile all the predicates into one big string with "&&"
		toMe = pred[0]->toString();
		for(int i = 1; i < pred.size(); i++) {
			toMe = "&&(" + pred[i]->toString() + ", " + toMe + ")";
		}
	}
}

bool isEQCheck(ExprTreePtr expr, MyDB_TableReaderWriterPtr left, MyDB_TableReaderWriterPtr right, pair<string, string>& toMe) {
	if (toMe.first != "bool[true]") return false;
	if (expr->isEq()) {
		ExprTreePtr lhs = expr->getLHS();
		ExprTreePtr rhs = expr->getRHS();
		if (lhs->isId() && rhs->isId()) {
			auto leftAtts = left->getTable()->getSchema()->getAtts();
			auto rightAtts = right->getTable()->getSchema()->getAtts();
			for(auto itemL : leftAtts) {
				if (lhs->toString() == '[' + itemL.first + ']') {
					for(auto itemR : rightAtts) {
						if (rhs->toString() == '[' + itemR.first + ']') {
							toMe = {lhs->toString(), rhs->toString()};
							return true;
						}
					}
				}
			}
			for(auto itemL : leftAtts) {
				if (rhs->toString() == '[' + itemL.first + ']') {
					for(auto itemR : rightAtts) {
						if (lhs->toString() == '[' + itemR.first + ']') {
							toMe = {rhs->toString(), lhs->toString()};
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

string getRealAttName(const string& fromMe) {
	size_t pos = fromMe.find('_');
	return fromMe.substr(pos + 1);
}

MyDB_TableReaderWriterPtr LogicalTableScan :: execute (map <string, MyDB_TableReaderWriterPtr> &allTableReaderWriters,
	map <string, MyDB_BPlusTreeReaderWriterPtr> &allBPlusReaderWriters) {
	// TODO: optimize for BPlusSelection
	vector<string> projections;
	auto outAtts = outputSpec->getSchema()->getAtts();
	for(const auto& item : outAtts) {
		projections.push_back('[' + item.first + ']');
	}
	MyDB_TableReaderWriterPtr inputRW;
	
	MyDB_TableReaderWriterPtr outputRW;
	string inputName = inputSpec->getName();
	MyDB_BufferManagerPtr myBuffer;
	if (allTableReaderWriters.find(inputName) != allTableReaderWriters.end()) {
		myBuffer = allTableReaderWriters[inputName]->getBufferMgr();
	} else if (allBPlusReaderWriters.find(inputName) != allBPlusReaderWriters.end()) {
		myBuffer = allTableReaderWriters[inputName]->getBufferMgr();
	}
	inputRW = make_shared<MyDB_TableReaderWriter>(inputSpec, myBuffer);
	// create a temp reader writer for the output table
	outputRW = make_shared<MyDB_TableReaderWriter>(outputSpec, myBuffer);
	string myPred;
	mergePred(selectionPred, myPred);
	// ready to go, run it
	RegularSelection mySelect(inputRW, outputRW, myPred, projections);
	mySelect.run();
	return outputRW;
}

MyDB_TableReaderWriterPtr LogicalJoin :: execute (map <string, MyDB_TableReaderWriterPtr> &allTableReaderWriters,
	map <string, MyDB_BPlusTreeReaderWriterPtr> &allBPlusReaderWriters) {
	string lefetJoinPred;
	string rightJoinPred;
	MyDB_TableReaderWriterPtr left;
	MyDB_TableReaderWriterPtr right;
	string myPred;
	vector<string> projections;
	pair<string, string> eqCheck = {"bool[true]", "bool[true]"};
	// optimization: if left of right is a TableScan, put the predicate in the merge
	// why better? no writing to a tempTable and read it again. => Less I/O
	if (leftInputOp->isTableScan()) {
		shared_ptr<LogicalTableScan> scan = dynamic_pointer_cast<LogicalTableScan>(leftInputOp);
		vector<ExprTreePtr> temp = scan->getOutputSelectionPredicate();
		string leftName = scan->getInputTableName();
		if (allTableReaderWriters.find(leftName) != allTableReaderWriters.end()) {
			left = allTableReaderWriters[leftName];
		} else if (allBPlusReaderWriters.find(leftName) != allBPlusReaderWriters.end()) {
			left = allBPlusReaderWriters[leftName];
		}
		left = make_shared<MyDB_TableReaderWriter>(scan->getInputTable(), left->getBufferMgr());
		mergePred(temp, lefetJoinPred);
	} else {
		// execute leftOp to get the left table
		// push the selection down, don't need the left pred now
		lefetJoinPred = "bool[true]";
		left = leftInputOp->execute(allTableReaderWriters, allBPlusReaderWriters);
	}

	if (rightInputOp->isTableScan()) {
		shared_ptr<LogicalTableScan> scan = dynamic_pointer_cast<LogicalTableScan>(rightInputOp);
		vector<ExprTreePtr> temp = scan->getOutputSelectionPredicate();
		string rightName = scan->getInputTableName();
		if (allTableReaderWriters.find(rightName) != allTableReaderWriters.end()) {
			right = allTableReaderWriters[rightName];
		} else if (allBPlusReaderWriters.find(rightName) != allBPlusReaderWriters.end()) {
			right = allBPlusReaderWriters[rightName];
		}
		right = make_shared<MyDB_TableReaderWriter>(scan->getInputTable(), right->getBufferMgr());
		mergePred(temp, rightJoinPred);
	} else {
		rightJoinPred = "bool[true]";
		right = rightInputOp->execute(allTableReaderWriters, allBPlusReaderWriters);
	}
	// projections
	for(const auto& item : outputSpec->getSchema()->getAtts()) {
		projections.push_back('[' + item.first + ']');
	}
	// create a temp reader writer for the output
	MyDB_TableReaderWriterPtr out = make_shared<MyDB_TableReaderWriter>(outputSpec, left->getBufferMgr());

	if (outputSelectionPredicate.size() == 0) {
		// cout << "ERROR: There should be at least one equality check in the selection clause.\n";
		// that means we have a table in the from clase but not in the where clause
		// TODO: find that unimportant table and scan the other one
		// auto leftAtts = left->getEmptyRecord()->getSchema()->getAtts();
		// auto rightAtts = right->getEmptyRecord()->getSchema()->getAtts();
		// auto outAtts = outputSpec->getSchema()->getAtts();
		// bool isLeft = false;
		// for(auto itemO : outAtts) {
		// 	if (isLeft) break;
		// 	for(auto itemL : leftAtts) {
		// 		if (itemL.first == itemO.first) {
		// 			isLeft = true;
		// 			break;
		// 		}
		// 	}
		// }
		// if (isLeft) {
		// 	RegularSelection mySelect(left, out, "bool[true]", projections);
		// 	mySelect.run();
		// 	return out;
		// } else {
		// 	RegularSelection mySelect(right, out, "bool[true]", projections);
		// 	mySelect.run();
		// 	return out;
		// }
		myPred = "bool[true]";
		eqCheck = {"bool[true]", "bool[true]"};
	} else {
		if (isEQCheck(outputSelectionPredicate[0], left, right, eqCheck)) {
			if (outputSelectionPredicate.size() == 1) {
				myPred = "bool[true]";
			} else {
				myPred = outputSelectionPredicate[1]->toString();
				for(int i = 2; i < outputSelectionPredicate.size(); i++) {
					myPred = "&&(" + outputSelectionPredicate[i]->toString() + ", " + myPred + ")";
				}
			}
		} else {
			myPred = outputSelectionPredicate[0]->toString();
			for(int i = 1; i < outputSelectionPredicate.size(); i++) {
				if (isEQCheck(outputSelectionPredicate[i], left, right, eqCheck)) {
					continue;
				}
				myPred = "&&(" + outputSelectionPredicate[i]->toString() + ", " + myPred + ")";
			}
		}
	}
	// cout << "Join on " << eqCheck.first << " " << eqCheck.second << endl;
	
	// Optimization: see if the smaller one of the table can be fitted into the bufferMgr, if so, scanJoin
	if (min(left->getNumPages(), right->getNumPages()) > left->getBufferMgr()->getNumPages() / 2) {
		SortMergeJoin myJoin(left, right, out, myPred, projections, eqCheck, lefetJoinPred, rightJoinPred);
		myJoin.run();
	} else {
		ScanJoin myJoin(left, right, out, myPred, projections, {eqCheck}, lefetJoinPred, rightJoinPred);
		myJoin.run();
	}
	if (!leftInputOp->isTableScan()) {
		left->getBufferMgr()->killTable(left->getTable());
	}
	if (!rightInputOp->isTableScan()) {
		right->getBufferMgr()->killTable(right->getTable());
	}
	// for(auto item : out->getTable()->getSchema()->getAtts()) {
	// 	cout << item.first << " ";
	// }
	// cout << endl;
	return out;
}

#endif
