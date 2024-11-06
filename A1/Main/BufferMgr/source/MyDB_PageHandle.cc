
#ifndef PAGE_HANDLE_C
#define PAGE_HANDLE_C

#include "MyDB_PageHandle.h"
#include "MyDB_Clock.h"
#include <algorithm>
#include <cstddef>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <sys/stat.h>

using namespace std;

void MyDB_PageHandleBase::readBlocks(const string &filename, streampos start,
                                     streamsize size) {
    checkFile(filename, start, size);
    ifstream fs;
    // binary mode ensures we are reading file byte by byte
    fs.open(filename, ios::in | ios::binary);
    if (!fs.is_open()) {
        // cout << "can't open file: " << filename << "\n";
        return;
    }
    fs.clear();
    // seek get, put the pointer to specified location
    fs.seekg(start, ios::beg);
    fs.read(page->getBytes(), size);
    if (!fs) {
        // cout << "can't read file: " << filename << "\n";
        return;
    }
    fs.close();
}

// TODO: passed the compile, can't write back to disk, waiting for DEBUG!!!!!
void MyDB_PageHandleBase::writeBlocks(const string &filename, streampos start,
                                      streamsize size) {
    // cout << "writeBlocks: write bytes ";
    // for (size_t i = 0; i < size; i++) {
    //     cout << page->getBytes()[i];
    // }
    // cout << "\n";
    // cout << "at: page " << start / size << ", for " << size << " bytes\n";
    ofstream fs;
    // Made a Mistake!!!
    // have to add ios::in, otherwise the filestream always empty the file first
    // then do the write
    // result: everything is null except one page
    fs.open(filename, ios::in | ios::out | ios::binary);
    if (!fs.is_open()) {
        // cout << "can't open file: " << filename << "\n";
        return;
    }
    fs.clear();
    // seek put
    fs.seekp(start, ios::beg);
    fs.write(page->getBytes(), size);
    if (!fs) {
        // cout << "can't write file: " << filename << "\n";
        return;
    }
    fs.close();
}

void MyDB_PageHandleBase::checkFile(const string &filename, streampos start,
                                    streamsize size) {
    fstream fs(filename, ios::app | ios::out);

    if (!fs.is_open()) {
        // File doesn't exist, create
        fs.open(filename, ios::out | ios::binary);
        if (!fs.is_open()) {
            // cout << "can't create file: " << filename << "\n";
            return;
        }
        fs.close();
        // to endure I have the permisson to read/write
        if (chmod(filename.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) !=
            0) {
            // cout << "Failed to set permissions for file: " << filename
            //           << "\n";
            return;
        }
        // Reopen in read/write mode
        fs.open(filename, ios::out | ios::binary);
    }
    fs.seekg(0, ios::end);
    // get the size of a file
    streamsize currentSize = fs.tellg();
    if (currentSize < start + size) {
        // Extend the file
        fs.clear();
        fs.seekp(currentSize, ios::beg);
        std::streamsize extendSize = start + size - currentSize;
        vector<char> buffer(extendSize, '\0');
        // Made a mistake
        // put bytes in the first arg and it only writes one byte
        fs.write(buffer.data(), buffer.size());
        if (!fs) {
            // cout << "can't extend file: " << filename << "\n";
            return;
        }
    }

    fs.close();
}

void *MyDB_PageHandleBase ::getBytes() {
    // in the buffer
    if (valid) {
        // only do a write operation when necessary
        // write operation takes a lot of cpu cycles
        if (!page->ref)
            page->ref = true;
        return page->getBytes();
    } else { // not in the buffer
        // tell clock we need a page
        size_t pgidx = clock->evict();
        // get the old handle of the assigned page
        MyDB_PageHandle oldHandle = clock->getOrinHandle(pgidx);
        // use this instead of oldHandle in case it is a nullptr
        page = clock->getPageFromPool(pgidx);
        // the page was assigned to a handle
        // we assured that the page must have a handle to be dirty
        // because handle clears the dirty flag(by writing back to disk) in
        // disconstructor
        if (oldHandle != nullptr) {
            // Made a Mistake!!!
            // Need to invalidate the ole handle
            // results: handle thinks the page is still its and try to access,
            // every function will fail
            oldHandle->valid = false;
            // dirty, write back to disk
            if (page->dirty) {
                writeBlocks(oldHandle->getStorageLoc(),
                            oldHandle->getPgNum() * clock->pageSize,
                            clock->pageSize);
                page->dirty = false;
            }
        }
        // map the idx to a new location
        clock->idx2loc[pgidx] = {storageLoc, pgNum};
        // read to the buffer
        readBlocks(storageLoc, pgNum * clock->pageSize, clock->pageSize);
        // update ref
        page->ref = true;
        // the flag says we need to pin this page
        if (pin) {
            if (clock->canPin()) {
                page->pinned = true;
                clock->pinnedNum++;
                pin = false;
            } else {
                // cout << "can't pin the page: too many pinned pages!\n";
            }
        }
        // now we have a valid handle
        valid = true;
        return page->getBytes();
    }
}

void MyDB_PageHandleBase ::wroteBytes() {
    if (!valid)
        return;
    page->dirty = true;
}

inline MyDB_Page *MyDB_PageHandleBase ::getPage() { return page; }
inline string MyDB_PageHandleBase ::getStorageLoc() { return storageLoc; }
inline long MyDB_PageHandleBase ::getPgNum() { return pgNum; }

void MyDB_PageHandleBase ::makePin() {
    // make sure we still have the space
    if (clock->canPin()) {
        if (!valid) {
            // do it later, when it has been assigned a page
            pin = true;
            return;
        }
        // only write when it needs to
        if (!page->pinned) {
            page->pinned = true;
            clock->pinnedNum++;
            // Made a Mistake
            // forgot to change the pin
            // result: always trying to pin the page, refNum will be wrong
            pin = false;
        }
    }
}

void MyDB_PageHandleBase ::unpinned() {
    if (!valid)
        return;
    // only write when it needs to
    if (page->pinned) {
        page->pinned = false;
        clock->pinnedNum--;
        pin = false;
    }
}

MyDB_PageHandleBase ::MyDB_PageHandleBase(string _storageLoc, long _pgNum,
                                          MyDB_Clock _clock, bool _anonymous)
    : valid(false), anonymous(false), page(nullptr), storageLoc(_storageLoc),
      pgNum(_pgNum), clock(_clock) {}

// destructor is called when there is no handle to a page (sharepointer
// refnum down to 0)
MyDB_PageHandleBase ::~MyDB_PageHandleBase() {
    // cout << "deallocating\n";
    if (valid) {
        // first make page unpinned
        if (page->pinned) {
            page->pinned = false;
            clock->pinnedNum--;
        }
        // an anonymous, we just forget it
        if (anonymous) {
            // change the ref so it can be evicted sooner in the clock
            page->ref = false;
        }
        // write back manually, because if this pointer dies, we don't know
        // where to write it
        if (page->dirty) {
            writeBlocks(storageLoc, pgNum * clock->pageSize, clock->pageSize);
            page->dirty = false;
        }
        // Made a Mistake!!!
        // always remember to update the lookup table
        // results: the table still has the mapping from the last test case
        // (handle die out of scope and called the deconstructor)
        clock->idx2loc.erase(clock->getPageIdx(page));
    }
    // remove it from the lookup table
    clock->loc2view.erase({storageLoc, pgNum});
}

#endif
