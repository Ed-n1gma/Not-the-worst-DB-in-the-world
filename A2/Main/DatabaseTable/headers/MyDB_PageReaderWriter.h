
#ifndef PAGE_RW_H
#define PAGE_RW_H

#include "MyDB_PageHandle.h"
#include "MyDB_PageType.h"
#include "MyDB_TableReaderWriter.h"
#include <cstddef>

struct MyDB_PageHeader {
    size_t numBytesUsed;
    size_t numRec;
    char myBytes[0];
};

class MyDB_PageRecIterator : public MyDB_RecordIterator {
  public:
    // put the contents of the next record in the file/page into the iterator
    // record this should be called BEFORE the iterator record is first examined
    void getNext();

    // return true iff there is another record in the file/page
    bool hasNext();

    // destructor and contructor
    MyDB_PageRecIterator(MyDB_PageReaderWriter *prw, MyDB_RecordPtr recordPtr);
	MyDB_PageRecIterator(const MyDB_PageRecIterator& toCopy);
	MyDB_PageRecIterator& operator=(const MyDB_PageRecIterator& toCopy);

  private:
    MyDB_RecordPtr myRec;
    MyDB_PageHandle myHandle;
    size_t pgSize;
    // current location of iterator on the page, access with
    // &header->myBytes[loc]
    size_t myLoc;
};

class MyDB_PageReaderWriter {

  public:
    // ANY OTHER METHODS YOU WANT HERE
    MyDB_PageReaderWriter(MyDB_PageHandle handle, size_t _pgSize);
    MyDB_PageReaderWriter(const MyDB_PageReaderWriter &toCopy);
    MyDB_PageReaderWriter &operator=(const MyDB_PageReaderWriter &right);
    // empties out the contents of this page, so that it has no records in it
    // the type of the page is set to MyDB_PageType :: RegularPage
    void clear();

    // return an itrator over this page... each time returnVal->next () is
    // called, the resulting record will be placed into the record pointed to
    // by iterateIntoMe
    MyDB_RecordIteratorPtr getIterator(MyDB_RecordPtr iterateIntoMe);

    // appends a record to this page... return false is the append fails because
    // there is not enough space on the page; otherwise, return true
    bool append(MyDB_RecordPtr appendMe);

    // gets the type of this page... this is just a value from an ennumeration
    // that is stored within the page
    MyDB_PageType getType();

    // sets the type of the page
    void setType(MyDB_PageType toMe);
    // so it can access the members
    friend class MyDB_PageRecIterator;

  private:
    MyDB_PageType myType;
    MyDB_PageHandle myHandle;
    size_t pgSize;
};

#endif
