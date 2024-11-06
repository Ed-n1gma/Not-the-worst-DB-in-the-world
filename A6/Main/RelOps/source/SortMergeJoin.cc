
#ifndef SORTMERGE_CC
#define SORTMERGE_CC

#include "SortMergeJoin.h"
#include "MyDB_BufferManager.h"
#include "MyDB_PageListIteratorAlt.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_Record.h"
#include "MyDB_TableReaderWriter.h"
#include "Sorting.h"
#include <vector>

SortMergeJoin ::SortMergeJoin(MyDB_TableReaderWriterPtr leftInputIn,
                              MyDB_TableReaderWriterPtr rightInputIn,
                              MyDB_TableReaderWriterPtr outputIn,
                              string finalSelectionPredicateIn,
                              vector<string> projectionsIn,
                              pair<string, string> equalityCheckIn,
                              string leftSelectionPredicateIn,
                              string rightSelectionPredicateIn) {
    runSize = leftInputIn->getBufferMgr()->getNumPages() / 2;
    output = outputIn;
    finalSelectionPredicate = finalSelectionPredicateIn;
    projections = projectionsIn;
    equalityCheck = equalityCheckIn;
    leftTable = leftInputIn;
    rightTable = rightInputIn;
    leftSelectionPredicate = leftSelectionPredicateIn;
    rightSelectionPredicate = rightSelectionPredicateIn;
}

void SortMergeJoin ::run() {
    // sort phase
    // records used to compare
    MyDB_RecordPtr leftLhs = leftTable->getEmptyRecord();
    MyDB_RecordPtr leftRhs = leftTable->getEmptyRecord();
    // build a comparator with equalityCheck
    auto leftCompare =
        buildRecordComparator(leftLhs, leftRhs, equalityCheck.first);
    // sort and put runs in a min priority queue
    auto leftQue =
        buildItertorOverSortedRuns(runSize, *leftTable, leftCompare,
                                   leftLhs, leftRhs, leftSelectionPredicate);

    MyDB_RecordPtr rightLhs = rightTable->getEmptyRecord();
    MyDB_RecordPtr rightRhs = rightTable->getEmptyRecord();
    auto rightCompare =
        buildRecordComparator(rightLhs, rightRhs, equalityCheck.second);
    auto rightQue =
        buildItertorOverSortedRuns(runSize, *rightTable, rightCompare,
                                   rightLhs, rightRhs, rightSelectionPredicate);
    // merge phase
    MyDB_RecordPtr outRec = output->getEmptyRecord();

    MyDB_RecordPtr leftRec = leftTable->getEmptyRecord();
    MyDB_RecordPtr rightRec = rightTable->getEmptyRecord();
    auto lessThen = buildRecordComparator(leftRec, rightRec, equalityCheck.first, equalityCheck.second);
    auto largeThen = buildRecordComparator(rightRec, leftRec, equalityCheck.second, equalityCheck.first);

    // and get the schema that results from combining the left and right records
    MyDB_SchemaPtr mySchemaOut = make_shared<MyDB_Schema>();
    for (auto &p : leftTable->getTable()->getSchema()->getAtts())
        mySchemaOut->appendAtt(p);
    for (auto &p : rightTable->getTable()->getSchema()->getAtts())
        mySchemaOut->appendAtt(p);

    // get the combined record
    MyDB_RecordPtr combinedRec = make_shared<MyDB_Record>(mySchemaOut);
    combinedRec->buildFrom(leftRec, rightRec);
    func finalPredicate =
        combinedRec->compileComputation(finalSelectionPredicate);
    // and get the final set of computatoins that will be used to buld the
    // output record
    vector<func> finalComputations;
    for (const string &s : projections) {
        finalComputations.push_back(combinedRec->compileComputation(s));
    }
    // merge start
    if (!leftQue->advance()) return;
    leftQue->getCurrent(leftRec);
    if (!rightQue->advance()) return;
    bool rightNotEmpty = true;
    rightQue->getCurrent(rightRec);
    // how to deal with bag union? right now I can do 1 * 32 = 32
    // but not 2 * 32 = 64, that is: how to handle duplicate in left?
    // copy every right rec that equals to the left into pinned anounomous pages;
    vector<MyDB_PageReaderWriter> repeatedRight;
    MyDB_BufferManagerPtr myBufferMgr = leftTable->getBufferMgr();
    MyDB_PageReaderWriter tempPage(true, *myBufferMgr);
    bool pageIsEmpty = true;
    while (true) {
        if (rightNotEmpty) {
            // equal
            if ((!largeThen()) && (!lessThen())) {
                // check to see if it is accepted by the join predicate
                if (finalPredicate()->toBool()) {

                    // run all of the computations
                    int i = 0;
                    for (auto &f : finalComputations) {
                        outRec->getAtt(i++)->set(f());
                    }

                    // the record's content has changed because it
                    // is now a composite of two records whose content
                    // has changed via a read... we have to tell it this,
                    // or else the record's internal buffer may cause it
                    // to write old values
                    outRec->recordContentHasChanged();
                    output->append(outRec);
                }
                // push matched rightRec into the page as a copy
                if (!tempPage.append(rightRec)) {
                    // full, get a new one
                    repeatedRight.push_back(tempPage);
                    tempPage = MyDB_PageReaderWriter(true, *myBufferMgr);
                    tempPage.append(rightRec);
                }
                pageIsEmpty = false;
                rightNotEmpty = rightQue->advance();
                if (rightNotEmpty) {
                    rightQue->getCurrent(rightRec);
                } else {
                    if (!leftQue->advance()) break;
                    leftQue->getCurrent(leftRec);
                    repeatedRight.push_back(tempPage);
                }
                continue;
            }
            // left < right
            if (lessThen()) {
                if (!leftQue->advance()) break;
                leftQue->getCurrent(leftRec);
                // could be the end of repeated r, check if the left equals to whatever is inside of repeatedRight
                if (!pageIsEmpty) {
                    repeatedRight.push_back(tempPage);
                    tempPage = MyDB_PageReaderWriter(true, *myBufferMgr);
                    pageIsEmpty = true;
                }
                if (repeatedRight.size() == 0) continue;
                auto iterOverCopiedRecs = MyDB_PageListIteratorAlt(repeatedRight);
                // repeatedRight not empty
                if (iterOverCopiedRecs.advance()) {
                    iterOverCopiedRecs.getCurrent(rightRec);
                    // equal, join everything
                    if ((!largeThen()) && (!lessThen())) {
                        if (finalPredicate()->toBool()) {
                            // run all of the computations
                            int i = 0;
                            for (auto &f : finalComputations) 
                                outRec->getAtt(i++)->set(f());

                            outRec->recordContentHasChanged();
                            output->append(outRec);
                        }
                        while (iterOverCopiedRecs.advance()) {
                            iterOverCopiedRecs.getCurrent(rightRec);
                            if (finalPredicate()->toBool()) {
                                // run all of the computations
                                int i = 0;
                                for (auto &f : finalComputations) 
                                    outRec->getAtt(i++)->set(f());

                                outRec->recordContentHasChanged();
                                output->append(outRec);
                            }
                        }
                        // // next
                        // if (!leftQue->advance()) break;
                        // leftQue->getCurrent(leftRec);
                    } else {
                        // does not match, abandon the repeatedRight
                        repeatedRight = vector<MyDB_PageReaderWriter>();
                        tempPage.clear();
                        pageIsEmpty = true;
                    }
                    // restore right
                    rightQue->getCurrent(rightRec);
                    if ((!largeThen()) && (!lessThen())) {
                        repeatedRight = vector<MyDB_PageReaderWriter>();
                        tempPage.clear();
                        pageIsEmpty = true;
                    }
                    continue;
                }
            }
            // left > right
            if (largeThen()) {
                repeatedRight = vector<MyDB_PageReaderWriter>();
                tempPage.clear();
                if (!rightQue->advance()) break;
                rightQue->getCurrent(rightRec);
                continue;
            }
        } else {
            // right is empty, but left is not, check if the left still matches the repeatedRight
            // could be the end of repeated r, check if the left equals to whatever is inside of repeatedRight
            auto iterOverCopiedRecs = MyDB_PageListIteratorAlt(repeatedRight);
            // repeatedRight not empty
            if (iterOverCopiedRecs.advance()) {
                iterOverCopiedRecs.getCurrent(rightRec);
                // equal, join everything
                if ((!largeThen()) && (!lessThen())) {
                    if (finalPredicate()->toBool()) {
                        // run all of the computations
                        int i = 0;
                        for (auto &f : finalComputations) 
                            outRec->getAtt(i++)->set(f());

                        outRec->recordContentHasChanged();
                        output->append(outRec);
                    }
                    while (iterOverCopiedRecs.advance()) {
                        iterOverCopiedRecs.getCurrent(rightRec);
                        if (finalPredicate()->toBool()) {
                            // run all of the computations
                            int i = 0;
                            for (auto &f : finalComputations) 
                                outRec->getAtt(i++)->set(f());

                            outRec->recordContentHasChanged();
                            output->append(outRec);
                        }
                    }
                }
            } else break; // repeatedRight is empty, don't bother
            if (!leftQue->advance()) break;
            leftQue->getCurrent(leftRec);
            continue;
        }
    }
}

#endif
