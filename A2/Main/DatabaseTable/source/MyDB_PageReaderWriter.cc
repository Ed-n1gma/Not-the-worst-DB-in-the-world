
#ifndef PAGE_RW_C
#define PAGE_RW_C

#include "MyDB_PageReaderWriter.h"
#include "MyDB_Record.h"
#include <cstddef>
#include <memory>

MyDB_PageRecIterator::MyDB_PageRecIterator(MyDB_PageReaderWriter* prw, MyDB_RecordPtr recordPtr) {
    myRec = recordPtr;
    myHandle = prw->myHandle;
    pgSize = prw->pgSize;
    myLoc = 0;
}

MyDB_PageRecIterator::MyDB_PageRecIterator(const MyDB_PageRecIterator& toCopy){
    *this = toCopy;
}

MyDB_PageRecIterator& MyDB_PageRecIterator::operator=(const MyDB_PageRecIterator& toCopy) {
    this->myRec = toCopy.myRec;
    this->myHandle = toCopy.myHandle;
    this->pgSize = toCopy.pgSize;
    this->myLoc = toCopy.myLoc;
    return *this;
}

bool MyDB_PageRecIterator::hasNext() {
    MyDB_PageHeader* header = (MyDB_PageHeader*) myHandle->getBytes();
    // empty page (was cleared), move to the next
    if (header->numRec == 0) return false;
    // not empty and on the first data, has next
    if (myLoc == 0) return true;
    // reach the end, no
    if (myLoc == header->numBytesUsed) return false;
    return true;
}

void MyDB_PageRecIterator::getNext() {
    MyDB_PageHeader* header = (MyDB_PageHeader*) myHandle->getBytes();
    // read a new record
    myRec->fromBinary(&((header->myBytes)[myLoc]));
    // inform record the content has changed
    myRec->recordContentHasChanged();
    // update the myLoc
    myLoc += myRec->getBinarySize();
}

MyDB_PageReaderWriter::MyDB_PageReaderWriter(MyDB_PageHandle handle, size_t _pgSize)
    : myHandle(handle), pgSize(_pgSize) {};

MyDB_PageReaderWriter::MyDB_PageReaderWriter(const MyDB_PageReaderWriter &toCopy) {
    *this=toCopy;
}

MyDB_PageReaderWriter& MyDB_PageReaderWriter::operator=(const MyDB_PageReaderWriter &right) {
    if (this == &right)
        return *this;
    this->myHandle = right.myHandle;
    this->myHandle = right.myHandle;
    this->myType = right.myType;
    this->pgSize = right.pgSize;
    return *this;
}

void MyDB_PageReaderWriter ::clear() {
    // just change the header;
    MyDB_PageHeader *header = (MyDB_PageHeader *)myHandle->getBytes();
    header->numBytesUsed = 0;
    header->numRec = 0;
    myHandle->wroteBytes();
    myType = MyDB_PageType::RegularPage;
}

MyDB_PageType MyDB_PageReaderWriter ::getType() { return myType; }

MyDB_RecordIteratorPtr MyDB_PageReaderWriter ::getIterator(MyDB_RecordPtr record) {
    return make_shared<MyDB_PageRecIterator>(this, record);
}

void MyDB_PageReaderWriter ::setType(MyDB_PageType type) { 
    myType = type;
}

bool MyDB_PageReaderWriter ::append(MyDB_RecordPtr record) { 
    // check whether the record fits the page
    size_t recSize = record->getBinarySize();
    // cout << "Append recSize: " << recSize << endl;
    MyDB_PageHeader* header = (MyDB_PageHeader*) myHandle->getBytes();
    // cout << "numBytesUsed: " << header->numBytesUsed << endl;
    if (sizeof(MyDB_PageHeader) + header->numBytesUsed + recSize > pgSize) {
        // too large, failed
        // cout << "should change to a new page\n" << endl;
        return false;
    }
    record->toBinary((void*)&((header->myBytes)[header->numBytesUsed]));
    header->numBytesUsed += recSize;
    header->numRec++;
    myHandle->wroteBytes();
    // cout << "First Byte: " << (header->myBytes)[header->numBytesUsed] <<endl;
    
    
    return true; 
}

#endif
