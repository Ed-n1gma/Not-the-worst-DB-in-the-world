
#ifndef BUFFER_MGR_C
#define BUFFER_MGR_C

#include "MyDB_BufferManager.h"
#include <memory>
#include <string>

using namespace std;

MyDB_PageHandle MyDB_BufferManager ::getPage(MyDB_TablePtr whichTable, long i) {
    MyDB_PageHandle handle;
    string location = whichTable->getStorageLoc();
    // in the lookup table
    if (clock->loc2view.find({location, i}) != clock->loc2view.end()) {
        // create a share_ptr from a weak_ptr
        handle = clock->loc2view[{location, i}].lock();
    } else {
        // not in the lookup, create one
        handle = make_shared<MyDB_PageHandleBase>(location, i, clock, false);
        // put a new view
        clock->loc2view[{location, i}] = handle;
    }
    return handle;
}

MyDB_PageHandle MyDB_BufferManager ::getPage() {
    // use a lowest available page
    long i = 0;
    while (clock->loc2view.find({tempFile, i}) != clock->loc2view.end())
        i++;
    MyDB_PageHandle handle =
        make_shared<MyDB_PageHandleBase>(tempFile, i, clock, true);
    // Made A Mistake!!!
    // forgot to add location to view in the table
    // results: only store in the first page
    clock->loc2view[{tempFile, i}] = handle;
    return handle;
}

MyDB_PageHandle MyDB_BufferManager ::getPinnedPage(MyDB_TablePtr whichTable,
                                                   long i) {
    MyDB_PageHandle handle = getPage(whichTable, i);
    handle->makePin();
    return handle;
}

MyDB_PageHandle MyDB_BufferManager ::getPinnedPage() {

    MyDB_PageHandle handle = getPage();
    handle->makePin();
    return handle;
}

void MyDB_BufferManager ::unpin(MyDB_PageHandle unpinMe) {
    unpinMe->unpinned();
}

MyDB_BufferManager ::MyDB_BufferManager(size_t pageSize, size_t numPages,
                                        string tempFile)
    : tempFile(tempFile) {
    bufferPool = new char[pageSize * numPages];
    clock =
        make_shared<MyDB_ClockBase>(pageSize, numPages, tempFile, bufferPool);
}

MyDB_BufferManager ::~MyDB_BufferManager() { delete[] bufferPool; }

#endif
