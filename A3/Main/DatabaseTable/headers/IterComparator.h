

#ifndef ITER_COMPARATOR_H
#define ITER_COMPARATOR_H

#include "MyDB_Record.h"
#include "MyDB_RecordIteratorAlt.h"
#include <iostream>
using namespace std;

class IterComparator {

  public:
    IterComparator(function<bool()> comparatorIn, MyDB_RecordPtr lhsIn,
                   MyDB_RecordPtr rhsIn) {
        comparator = comparatorIn;
        lhs = lhsIn;
        rhs = rhsIn;
    }

    bool operator()(MyDB_RecordIteratorAltPtr lIter,
                    MyDB_RecordIteratorAltPtr rIter) {
        lIter->getCurrent(lhs);
        rIter->getCurrent(rhs);
		// cout << lhs << endl;
		// cout << "\n";
		// cout << rhs << endl;
		// cout << comparator() << endl;
        return !comparator();
    }

  private:
    function<bool()> comparator;
    MyDB_RecordPtr lhs;
    MyDB_RecordPtr rhs;
};

#endif
