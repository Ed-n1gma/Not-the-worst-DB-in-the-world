
#ifndef TABLE_RW_C
#define TABLE_RW_C

#include "MyDB_TableReaderWriter.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_Record.h"
#include "MyDB_Table.h"
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <memory>

using namespace std;

MyDB_TableRecIterator::MyDB_TableRecIterator(MyDB_TableReaderWriter trw,
                                             MyDB_RecordPtr recordPtr)
    : myRec(recordPtr), TRW(trw), pgId(0) {
    pageIterator = TRW[pgId].getIterator(myRec);
}

bool MyDB_TableRecIterator::hasNext() {
    // break if we have a next, or pgId == TRW.myTable->lastPage(), which means this is our last page
    while (pgId < TRW.myTable->lastPage() && !pageIterator->hasNext()) {
        // if this page doesn't have next, swich to next page
        pgId++;
        pageIterator = TRW[pgId].getIterator(myRec);
    }
    // last page has next? ture. no? false;
    return pageIterator->hasNext();
}

// hasNext made sure that getNext is almost certain to get the next record
void MyDB_TableRecIterator::getNext() {
    pageIterator->getNext();
}

MyDB_TableReaderWriter ::MyDB_TableReaderWriter(MyDB_TablePtr table,
                                                MyDB_BufferManagerPtr buffer)
    : myTable(table), myBuffer(buffer) {}

MyDB_TableReaderWriter ::MyDB_TableReaderWriter(
    const MyDB_TableReaderWriter &toCopy) {
    *this = toCopy;
}

MyDB_TableReaderWriter& MyDB_TableReaderWriter::operator=(const MyDB_TableReaderWriter& toCopy) {
    this->myTable = toCopy.myTable;
    this->myBuffer = toCopy.myBuffer;
    return *this;
}

MyDB_PageReaderWriter MyDB_TableReaderWriter ::operator[](size_t i) {
    // if out of bound, return the last
    // I don't want user to right a page that's way beyond the current last page.
    if (i >= myTable->lastPage()) return last();

    MyDB_PageReaderWriter temp(myBuffer->getPage(myTable, i),
                               myBuffer->getPageSize());
    return temp;
}

MyDB_RecordPtr MyDB_TableReaderWriter ::getEmptyRecord() {
    return make_shared<MyDB_Record>(myTable->getSchema());
}

MyDB_PageReaderWriter MyDB_TableReaderWriter ::last() {
    // there has never been anything
    // written to the table
    if (myTable->lastPage() < 0) {
        // get a page
        MyDB_PageReaderWriter temp(myBuffer->getPage(myTable, 0),
                                   myBuffer->getPageSize());
        // set the last to 0
        myTable->setLastPage(0);
        temp.clear();
        return temp;
    }
    MyDB_PageReaderWriter temp(myBuffer->getPage(myTable, myTable->lastPage()),
                               myBuffer->getPageSize());
    return temp;
}

void MyDB_TableReaderWriter ::append(MyDB_RecordPtr record) {
    // every time we append the record is almost certained to have changed
    record->recordContentHasChanged();
    MyDB_PageReaderWriter lastOne = last();
    if (!lastOne.append(record)) {
        // failed to append, need a new page
        size_t newLast = myTable->lastPage() + 1;
        MyDB_PageReaderWriter temp(myBuffer->getPage(myTable, newLast),
                                   myBuffer->getPageSize());
        temp.clear();
        myTable->setLastPage(newLast);
        temp.append(record);
    }
    return;
}

void MyDB_TableReaderWriter ::loadFromTextFile(string filename) {
    // read lines, and one line per record
    // record can load from stream
    string line;
    ifstream myFile(filename);
    // set the last page to be 0 and then call append()
    // this will get an empty new 0-th page
    myTable->setLastPage(-1);
    MyDB_RecordPtr tempRec = getEmptyRecord();
    if (myFile.is_open()) {
        while (getline(myFile, line)) {
            // load from the string (one line from the file)
            tempRec->fromString(line);
            // as long as we have a right append, it will work
            append(tempRec);
        }
        myFile.close();
    }
    return;
}

MyDB_RecordIteratorPtr MyDB_TableReaderWriter ::getIterator(MyDB_RecordPtr record) {
    return make_shared<MyDB_TableRecIterator>(*this, record);
}

void MyDB_TableReaderWriter ::writeIntoTextFile(string filename) {
    ofstream myFile(filename);
    MyDB_RecordPtr temp = getEmptyRecord();
    auto it = getIterator(temp);
    if(myFile.is_open()) {
        while (it->hasNext()) {
            it->getNext();
            myFile << temp << '\n';
        }
        myFile.close();
    }
    return;
}

#endif
