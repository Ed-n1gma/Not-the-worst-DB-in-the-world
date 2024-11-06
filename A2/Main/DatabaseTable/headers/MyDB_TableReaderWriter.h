
#ifndef TABLE_RW_H
#define TABLE_RW_H

#include "MyDB_BufferManager.h"
#include "MyDB_Record.h"
#include "MyDB_RecordIterator.h"
#include "MyDB_Table.h"
#include <cstddef>
#include <memory>

// create a smart pointer for the catalog
using namespace std;
class MyDB_PageRecIterator;
class MyDB_PageReaderWriter;
class MyDB_TableReaderWriter;
typedef shared_ptr<MyDB_TableReaderWriter> MyDB_TableReaderWriterPtr;

class MyDB_TableReaderWriter {
  public:
    // ANYTHING ELSE YOU NEED HERE

    // create a table reader/writer for the specified table, using the specified
    // buffer manager
    MyDB_TableReaderWriter(MyDB_TablePtr forMe, MyDB_BufferManagerPtr myBuffer);
    MyDB_TableReaderWriter(const MyDB_TableReaderWriter &toCopy);
    MyDB_TableReaderWriter& operator=(const MyDB_TableReaderWriter& toCopy);
    friend class MyDB_TableRecIterator;

    // gets an empty record from this table
    MyDB_RecordPtr getEmptyRecord();

    // append a record to the table
    void append(MyDB_RecordPtr appendMe);

    // return an itrator over this table... each time returnVal->next () is
    // called, the resulting record will be placed into the record pointed to
    // by iterateIntoMe
    MyDB_RecordIteratorPtr getIterator(MyDB_RecordPtr iterateIntoMe);

    // load a text file into this table... overwrites the current contents
    void loadFromTextFile(string fromMe);

    // dump the contents of this table into a text file
    void writeIntoTextFile(string toMe);

    // access the i^th page in this file
    MyDB_PageReaderWriter operator[](size_t i);

    // access the last page in the file
    MyDB_PageReaderWriter last();

  private:
    // ANYTHING YOU NEED HERE
    MyDB_TablePtr myTable;
    MyDB_BufferManagerPtr myBuffer;
};

class MyDB_TableRecIterator : public MyDB_RecordIterator {
  public:
    // put the contents of the next record in the file/page into the iterator
    // record this should be called BEFORE the iterator record is first examined
    void getNext();

    // return true iff there is another record in the file/page
    bool hasNext();

    // destructor and contructor
    MyDB_TableRecIterator(MyDB_TableReaderWriter trw, MyDB_RecordPtr recordPtr);

  private:
    MyDB_RecordPtr myRec;
    MyDB_TableReaderWriter TRW;
    MyDB_RecordIteratorPtr pageIterator;
    int pgId;
};
#endif
