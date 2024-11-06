
#ifndef BPLUS_C
#define BPLUS_C

#include "MyDB_BPlusTreeReaderWriter.h"
#include "MyDB_AttVal.h"
#include "MyDB_INRecord.h"
#include "MyDB_PageListIteratorSelfSortingAlt.h"
#include "MyDB_PageListIteratorAlt.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_Record.h"
#include "MyDB_TableReaderWriter.h"
#include "RecordComparator.h"
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>

MyDB_BPlusTreeReaderWriter ::MyDB_BPlusTreeReaderWriter(
    string orderOnAttName, MyDB_TablePtr forMe, MyDB_BufferManagerPtr myBuffer)
    : MyDB_TableReaderWriter(forMe, myBuffer) {

    // find the ordering attribute
    auto res = forMe->getSchema()->getAttByName(orderOnAttName);

    // remember information about the ordering attribute
    orderingAttType = res.second;
    whichAttIsOrdering = res.first;
    rootLocation = -1;
}

void MyDB_BPlusTreeReaderWriter ::init() {
    // At the beginning the tree should have a root page and a leaf page
    MyDB_PageReaderWriter root = (*this)[0];
    MyDB_PageReaderWriter leaf = (*this)[1];
    root.setType(MyDB_PageType::DirectoryPage);
    leaf.setType(MyDB_PageType::RegularPage);
    // the key value will automatically be Infinity
    MyDB_INRecordPtr temp = getINRecord();
    // so just set the pointer
    temp->setPtr(1);
    temp->recordContentHasChanged();
    // append to the root page
    root.append(temp);
    // set the root location
    getTable()->setRootLocation(0);
    rootLocation = 0;
}

MyDB_RecordIteratorAltPtr
MyDB_BPlusTreeReaderWriter ::getSortedRangeIteratorAlt(MyDB_AttValPtr low, MyDB_AttValPtr high) {
    // printTree();
    vector<MyDB_PageReaderWriter> list;
    discoverPages(rootLocation, list, low, high);
    // cout << "getSortedRangeIteratorAlt: " << list.size() << endl;
    MyDB_RecordPtr lRec = getEmptyRecord();
    MyDB_RecordPtr rRec = getEmptyRecord();
    MyDB_RecordPtr myRec = getEmptyRecord();
    MyDB_RecordPtr lowRec = getEmptyRecord();
    MyDB_RecordPtr highRec = getEmptyRecord();
    auto comparator = buildComparator(lRec, rRec);
    lowRec->getAtt(whichAttIsOrdering)->set(low);
    highRec->getAtt(whichAttIsOrdering)->set(high);
    auto lowComparator = buildComparator(myRec, lowRec);
    auto highComparator = buildComparator(highRec, myRec);
    return make_shared<MyDB_PageListIteratorSelfSortingAlt>(list, lRec, rRec, comparator, 
                                                            myRec, lowComparator, 
		                                                    highComparator, true);
}

MyDB_RecordIteratorAltPtr
MyDB_BPlusTreeReaderWriter ::getRangeIteratorAlt(MyDB_AttValPtr low, MyDB_AttValPtr high) {
    // printTree();
    vector<MyDB_PageReaderWriter> list;
    discoverPages(rootLocation, list, low, high);
    // cout << "getSortedRangeIteratorAlt: " << list.size() << endl;
    MyDB_RecordPtr lRec = getEmptyRecord();
    MyDB_RecordPtr rRec = getEmptyRecord();
    MyDB_RecordPtr myRec = getEmptyRecord();
    MyDB_RecordPtr lowRec = getEmptyRecord();
    MyDB_RecordPtr highRec = getEmptyRecord();
    auto comparator = buildComparator(lRec, rRec);
    lowRec->getAtt(whichAttIsOrdering)->set(low);
    highRec->getAtt(whichAttIsOrdering)->set(high);
    auto lowComparator = buildComparator(myRec, lowRec);
    auto highComparator = buildComparator(highRec, myRec);
    return make_shared<MyDB_PageListIteratorSelfSortingAlt>(list, lRec, rRec, comparator, 
                                                            myRec, lowComparator, 
		                                                    highComparator, false);
}

bool MyDB_BPlusTreeReaderWriter ::discoverPages(int startPage, vector<MyDB_PageReaderWriter> &retList, MyDB_AttValPtr low, MyDB_AttValPtr high) {
    if (rootLocation < 0) init();
    MyDB_PageReaderWriter PRW = (*this)[startPage];
	// this is a leaf page, return false so we know the next level is leaf level
    if (PRW.getType() == MyDB_PageType::RegularPage) {
		// when we got here, it is already checked to fall within the range
		retList.push_back(PRW);
        return false;
    } else {
        // get an iterator, assume that the InRecords in internal page are sorted
        MyDB_INRecordPtr temp = getINRecord();
        auto iter = PRW.getIterator(temp);
        // make records for comparison
        MyDB_INRecordPtr lowRec = make_shared<MyDB_INRecord>(low);
        MyDB_INRecordPtr highRec = make_shared<MyDB_INRecord>(high);
        auto outOfLowScope = buildComparator(temp, lowRec);
        auto outOfHighScope = buildComparator(highRec, temp);
		// keep looking until we find a InRecord within the range
        while (iter->hasNext()) {
            iter->getNext();
			// found one
            if (!outOfLowScope()) break;
			// there may be pages after that fall into that range, so continue
        }
		// here we either found a page within the range, or its just the last page and still out of range
		if (outOfLowScope()) return true;

        MyDB_AttValPtr lastVal;
        if (discoverPages(temp->getPtr(), retList, low, high)) {
            // the child page of this page is still internal
            while (iter->hasNext()) {
                iter->getNext();
                if (outOfHighScope())  {
                    if (lastVal == nullptr) lastVal = temp->getKey()->getCopy();
                    else {
                        if (lastVal < temp->getKey()) return true;
                    }
                }
                discoverPages(temp->getPtr(), retList, low, high);
				// this is the last entry that falls into the range
				// the pages after that won't have the pages we are looking for
				// so just return
            }
        } else {
            // the child page of this page is a leaf page
            // no need for recursion, just add them to the list.
            while (iter->hasNext()) {
                iter->getNext();
                // fall into the range, not need to recurse
                // since we know the children are leaves
                // just append the possible page into the return list
                retList.push_back((*this)[temp->getPtr()]);
				if (outOfHighScope()) return true;
            }
        }
    }
    return true;
}

void MyDB_BPlusTreeReaderWriter ::append(MyDB_RecordPtr appendMe) {
    if (rootLocation < 0) init();
    MyDB_INRecordPtr res = append(rootLocation, appendMe);
    if (res != nullptr) {
        // root page is full, it's already been split
        // but we need to set a new root
        // because res is a new InRecord (isn't writen into the page yet)
        // res points to the newPage (lower half), old root is the larger half
        int newPgId = getNumPages();
        MyDB_PageReaderWriter newPage = (*this)[newPgId];
        newPage.setType(MyDB_PageType::DirectoryPage);
        // new root page consist of newInRec that points to the lower half of the old root
        newPage.append(res);
        // and an infinity that points to the larger half of the old root
        MyDB_INRecordPtr temp = getINRecord();
        temp->setPtr(rootLocation);
        // append
        newPage.append(temp);
        // update the root
        getTable()->setRootLocation(newPgId);
        rootLocation = newPgId;
    }
}

MyDB_INRecordPtr MyDB_BPlusTreeReaderWriter ::split(MyDB_PageReaderWriter pageToSplit, MyDB_RecordPtr recToInsert) {
	MyDB_RecordPtr lRecPtr, rRecPtr, temp;
    // get a new page to put the smaller half
	MyDB_PageReaderWriter newPage = (*this)[getNumPages()];
	// to split we should know what kind of record we are inserting
	if (recToInsert->getSchema() == nullptr) {
		lRecPtr = getINRecord();
		rRecPtr = getINRecord();
		temp = getINRecord();
	} else {
		// a leaf page, sort and split
		lRecPtr = getEmptyRecord();
		rRecPtr = getEmptyRecord();
		temp = getEmptyRecord();
	}
	auto comparator = buildComparator(lRecPtr, rRecPtr);
	MyDB_PageReaderWriterPtr sorted = pageToSplit.sort(comparator, lRecPtr, rRecPtr);
	auto iter = sorted->getIterator(temp);
	comparator = buildComparator(recToInsert, temp);
    // clear the page to hold larger half
	pageToSplit.clear();
    if (recToInsert->getSchema() == nullptr) {
        newPage.setType(MyDB_PageType::DirectoryPage);
        pageToSplit.setType(MyDB_PageType::DirectoryPage);
    } else {
        newPage.setType(MyDB_PageType::RegularPage);
        pageToSplit.setType(MyDB_PageType::RegularPage);
    }
    // whether we inserted recToInsert or not
	bool inserted = false;
    // count the bytes of rec we put into the newPage so we can split in half
	size_t bytesUsed = 0;
    // the return value, new internal rec
    MyDB_INRecordPtr newInternal = getINRecord();
    // point to the new page
	newInternal->setPtr(getNumPages() - 1);
	// small key value records in new page
	while (iter->hasNext()) {
		iter->getNext();
        // used more than half a page
		if (bytesUsed + temp->getBinarySize() >= sorted->getPageSize() / 2) {
            // insert into 
            if (!inserted && comparator()) {
                newPage.append(recToInsert);
                inserted = true;
            }
            // this is the last record of the newPage
            // use the key of the last rec to be the key of the newInternal
            if (recToInsert->getSchema() == nullptr) {
                // this is a internal page, the last element got kicked up (key replaced to infinity, ptr stays the same)
                newInternal->setKey(temp->getAtt(0)->getCopy());
                temp->getAtt(0)->set(orderingAttType->createAttMax());

            } else {
                newInternal->setKey(temp->getAtt(whichAttIsOrdering)->getCopy());
            }
            newPage.append(temp);
            break;
        }
        // insert into 
		if (!inserted && comparator()) {
			newPage.append(recToInsert);
			bytesUsed += recToInsert->getBinarySize();
            inserted = true;
		}
		newPage.append(temp);
		bytesUsed += temp->getBinarySize();
	}
    // // for internal page we don't copy the internal rec but move it (now its in our newInternal)
    // if (recToInsert->getSchema() == nullptr) {
    //     newInternal->setKey(temp->getAtt(0)->getCopy());
    //     MyDB_INRecordPtr inif = getINRecord();
    //     inif->setPtr(temp->getAtt(1)->toInt());
    //     newPage.append(inif);
    // } else {
    //     // for leave page we have to copy it
    //     if (!inserted && comparator()) {
	// 		pageToSplit.append(recToInsert);
    //         inserted = true;
	// 	}
	// 	pageToSplit.append(temp);
    //     newInternal->setKey(temp->getAtt(whichAttIsOrdering)->getCopy());
    // }

	// large half in place
	while (iter->hasNext()) {
		iter->getNext();
		if (!inserted && comparator()) {
			pageToSplit.append(recToInsert);
            inserted = true;
		}
		pageToSplit.append(temp);
	}
    if (!inserted) pageToSplit.append(recToInsert);

    return newInternal;
}

MyDB_INRecordPtr MyDB_BPlusTreeReaderWriter ::append(int whichPage, MyDB_RecordPtr appendMe) {
    MyDB_PageReaderWriter prw = (*this)[whichPage];
    if (appendMe->getSchema() != nullptr) {
        // leaf rec
        if (prw.getType() == MyDB_PageType::RegularPage) {
            // we reach the leaf level, just append
            if (prw.append(appendMe)) return nullptr;
            else {
                // split
                return split(prw, appendMe);
            }
        } else {
            // whichPage is an internal page
            MyDB_INRecordPtr temp = getINRecord();
            auto iter = prw.getIterator(temp);
            auto comparator = buildComparator(temp, appendMe);
            while (iter->hasNext()) {
                iter->getNext();
                // found an entry (appendme.key <= entry.key)
                if (!comparator()) {
                    // try to append
                    MyDB_INRecordPtr newInRec = append(temp->getPtr(), appendMe);
                    if (newInRec != nullptr) {
                        // there is a split
                        // append the new InRec to this page
                        if (prw.append(newInRec)) {
                            // sort every time we append a internal rec
                            MyDB_INRecordPtr lRec = getINRecord();
                            MyDB_INRecordPtr rRec = getINRecord();
                            auto sortcomparator = buildComparator(lRec, rRec);
                            prw.sortInPlace(sortcomparator, lRec, rRec);
                            return nullptr;
                        } else {
                            // if there's a split too, the upper layer will know 
                            // by looking at the return value
                            return split(prw, newInRec);
                        }
                    }
                    break;
                }
            }
        }
    }
    return nullptr;
}

MyDB_INRecordPtr MyDB_BPlusTreeReaderWriter ::getINRecord() {
    return make_shared<MyDB_INRecord>(orderingAttType->createAttMax());
}

void MyDB_BPlusTreeReaderWriter ::printTree() {
    // this function is intended for debugging the small data only (100 records)
    // so only need to print out every thing it has in the table from page 0 to last
    for(int i = 0; i < getNumPages(); i++) {
        MyDB_PageReaderWriter prw = (*this)[i];
        
        if (prw.getType() == MyDB_PageType::DirectoryPage) {
            printf("[Dir]Page %d: \n", i);
            auto temp = getINRecord();
            auto iter = prw.getIterator(temp);
            while (iter->hasNext()) {
                iter->getNext();
                cout << temp->getKey()->toString() << "|" << temp->getPtr() << "|\n";
                
            }
        } else {
            printf("[Leaf]Page %d: \n", i);
            auto temp = getEmptyRecord();
            auto iter = prw.getIterator(temp);
            while (iter->hasNext()) {
                iter->getNext();
                cout << temp << "\n";
            }
        }
    }
}

MyDB_AttValPtr MyDB_BPlusTreeReaderWriter ::getKey(MyDB_RecordPtr fromMe) {

    // in this case, got an IN record
    if (fromMe->getSchema() == nullptr)
        return fromMe->getAtt(0)->getCopy();

    // in this case, got a data record
    else
        return fromMe->getAtt(whichAttIsOrdering)->getCopy();
}

function<bool()>
MyDB_BPlusTreeReaderWriter ::buildComparator(MyDB_RecordPtr lhs,
                                             MyDB_RecordPtr rhs) {

    MyDB_AttValPtr lhAtt, rhAtt;

    // in this case, the LHS is an IN record
    if (lhs->getSchema() == nullptr) {
        lhAtt = lhs->getAtt(0);

        // here, it is a regular data record
    } else {
        lhAtt = lhs->getAtt(whichAttIsOrdering);
    }

    // in this case, the LHS is an IN record
    if (rhs->getSchema() == nullptr) {
        rhAtt = rhs->getAtt(0);

        // here, it is a regular data record
    } else {
        rhAtt = rhs->getAtt(whichAttIsOrdering);
    }

    // now, build the comparison lambda and return
    if (orderingAttType->promotableToInt()) {
        return [lhAtt, rhAtt] { return lhAtt->toInt() < rhAtt->toInt(); };
    } else if (orderingAttType->promotableToDouble()) {
        return [lhAtt, rhAtt] { return lhAtt->toDouble() < rhAtt->toDouble(); };
    } else if (orderingAttType->promotableToString()) {
        return [lhAtt, rhAtt] { return lhAtt->toString() < rhAtt->toString(); };
    } else {
        cout << "This is bad... cannot do anything with the >.\n";
        exit(1);
    }
}

function<bool()>
MyDB_BPlusTreeReaderWriter ::buildComparatorMyOwn(MyDB_RecordPtr lhs,
                                             MyDB_RecordPtr rhs) {

    MyDB_AttValPtr lhAtt, rhAtt;

    // in this case, the LHS is an IN record
    if (lhs->getSchema() == nullptr) {
        lhAtt = lhs->getAtt(0);

        // here, it is a regular data record
    } else {
        lhAtt = lhs->getAtt(whichAttIsOrdering);
    }

    // in this case, the LHS is an IN record
    if (rhs->getSchema() == nullptr) {
        rhAtt = rhs->getAtt(0);

        // here, it is a regular data record
    } else {
        rhAtt = rhs->getAtt(whichAttIsOrdering);
    }

    // now, build the comparison lambda and return
    if (orderingAttType->promotableToInt()) {
        return [lhAtt, rhAtt] { return lhAtt->toInt() <= rhAtt->toInt(); };
    } else if (orderingAttType->promotableToDouble()) {
        return [lhAtt, rhAtt] { return lhAtt->toDouble() <= rhAtt->toDouble(); };
    } else if (orderingAttType->promotableToString()) {
        return [lhAtt, rhAtt] { return lhAtt->toString() <= rhAtt->toString(); };
    } else {
        cout << "This is bad... cannot do anything with the >.\n";
        exit(1);
    }
}

#endif
