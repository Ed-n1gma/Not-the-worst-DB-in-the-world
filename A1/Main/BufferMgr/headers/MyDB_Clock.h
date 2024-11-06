#ifndef CLOCK_H
#define CLOCK_H

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
using namespace std;

// have to define here so it compiles
// include handle.h will cause recurence include
class MyDB_PageHandleBase;
typedef shared_ptr<MyDB_PageHandleBase> MyDB_PageHandle;
typedef weak_ptr<MyDB_PageHandleBase> MyDB_PageHandleView;

// to map a pair we need to define our own hash function
struct pair_hash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ h2;
    }
};

class MyDB_ClockBase;
typedef shared_ptr<MyDB_ClockBase> MyDB_Clock;

// This class stores the ptr to the actual page of memory
// also stores bits and pageId for Clock Algo
class MyDB_Page {
  public:
    MyDB_Page() : ref(false), dirty(false), pinned(false), bytes(nullptr) {}
    void assignBytes(char *_bytes) { bytes = _bytes; }
    char *getBytes() const { return bytes; }

  public:
    // for clock
    bool ref;
    // true when the page has been changed, need to write back to the disk
    bool dirty;
    // pinned
    bool pinned;

  private:
    // ptr to the actual storage
    char *bytes;
};

/* This class is used for getting a page ptr
    When found a ptr idx, use getPageFromPool(size_t PGID),
    if can't find that page in the table (not in memory), use evict() to find a
   victim page's idx then getPageFromPool(idx)
   It also has two lookup tables
*/
class MyDB_ClockBase {
  public:
    vector<MyDB_Page> pages;
    // clock arm
    size_t arm;
    // page num and size
    size_t pageSize;
    size_t numPages;
    // num of pinned pages
    size_t pinnedNum;
    // lookup table
    /* get a handle with location, stores a weak_pointer as a value,
    so that when all the handle die we won't have still one in the table
    (deconstructor will never be called)
    */
    unordered_map<pair<string, long>, MyDB_PageHandleView, pair_hash> loc2view;
    /*
    get the page's disk lacation so that we can write back the page easily
    */
    unordered_map<size_t, pair<string, long>> idx2loc;
    // constructor
    MyDB_ClockBase(size_t _pageSize, size_t _numPages, string _tempFile,
                   char *pool)
        : arm(0), pageSize(_pageSize), numPages(_numPages), pinnedNum(0) {
        pages = vector<MyDB_Page>(numPages);
        // assign ptr (memory entry to every page object)
        for (auto &p : pages) {
            p.assignBytes(pool);
            pool += pageSize;
        }
    }

    /*
    use this function to find the former handle to the newly assigned page
    */
    MyDB_PageHandle getOrinHandle(size_t idx) {
        // only happens when the old handle was an anonymous page handle
        // or it has never been assigned (or handle out of scope)
        if (loc2view.find(idx2loc[idx]) == loc2view.end())
            return nullptr;
        return loc2view[idx2loc[idx]].lock();
    }
    // get a page with idx
    // used when find the page id in the table
    MyDB_Page *getPageFromPool(size_t idx) {
        MyDB_Page *ret = nullptr;
        ret = &pages[idx];
        // only do the write operation (not wrtie to pool or disk, any write)
        // when necessary
        if (!ret->ref)
            ret->ref = true;
        return ret;
    }

    // get a page when pg id is not in the table
    size_t evict() {
        // move the arm until find the page with ref = 0, and unpinned
        while (pages[arm % numPages].ref || pages[arm % numPages].pinned) {
            pages[arm % numPages].ref = false;
            arm++;
        }
        // return the idx then move the arm
        size_t ret = arm % numPages;
        arm++;
        return ret;
    }

    inline size_t getPageIdx(MyDB_Page *page) {
        return (page->getBytes() - pages[0].getBytes()) / pageSize;
    }

    // Stupid Mistake
    // used pageSize instead of numPages
    inline bool canPin() { return pinnedNum < (numPages - 1); }
};

#endif