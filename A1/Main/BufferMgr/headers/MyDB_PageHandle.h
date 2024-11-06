
#ifndef PAGE_HANDLE_H
#define PAGE_HANDLE_H

#include "MyDB_Clock.h"
#include <cstddef>
#include <fstream>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <memory>

// page handles are basically smart pointers
using namespace std;
class MyDB_PageHandleBase;
class MyDB_BufferManager;
typedef shared_ptr<MyDB_PageHandleBase> MyDB_PageHandle;
// weak_pointer for lookup table
typedef weak_ptr<MyDB_PageHandleBase> MyDB_PageHandleView;

class MyDB_PageHandleBase {

  public:
    // THESE METHODS MUST BE IMPLEMENTED WITHOUT CHANGING THE DEFINITION

    // access the raw bytes in this page... if the page is not currently
    // in the buffer, then the contents of the page are loaded from
    // secondary storage.
    void *getBytes();

    // let the page know that we have written to the bytes.  Must always
    // be called once the page's bytes have been written.  If this is not
    // called, then the page will never be marked as dirty, and the page
    // will never be written to disk.
    void wroteBytes();

    // unpinned the page
    void unpinned();

    // get the page
    MyDB_Page *getPage();

    // together these two are the disk location of a handel (and page if handle
    // is valid) get filename
    string getStorageLoc();
    // get the page number of file;
    long getPgNum();

    MyDB_PageHandleBase(string _storageLoc, long _pgNum, MyDB_Clock _clock,
                        bool _anonymous);
    // There are no more references to the handle when this is called...
    // this should decrmeent a reference count to the number of handles
    // to the particular page that it references.  If the number of
    // references to a pinned page goes down to zero, then the page should
    // become unpinned.
    ~MyDB_PageHandleBase();

  private:
    // read a blocks of memory from disk to memory
    void readBlocks(const string &filename, streampos start, streamsize size);
    // write back the dirty page
    void writeBlocks(const string &filename, streampos start, streamsize size);
    // check file exist or not, does the file have enough space for read.
    // write back is after read so it must have enough space
    void checkFile(const string &filename, streampos start, streamsize size);
    // pin the page
    void makePin();

  public:
    friend class MyDB_BufferManager;
    // use valid to indicate that this handle really points to a page
    bool valid;
    bool anonymous;

  private:
    // if pin is true, the first time of calling getBytes() will make the page
    // pinned (if still availale)
    bool pin;
    MyDB_Page *page;
    string storageLoc;
    long pgNum;
    MyDB_Clock clock;
};

#endif
