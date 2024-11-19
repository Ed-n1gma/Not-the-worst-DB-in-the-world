
#ifndef PARSER_TYPES_H
#define PARSER_TYPES_H

#include <cstddef>
#include <iostream>
#include <memory>
#include <stdlib.h>
#include "ExprTree.h"
#include "MyDB_AttType.h"
#include "MyDB_AttVal.h"
#include "MyDB_Catalog.h"
#include "MyDB_Schema.h"
#include "MyDB_Table.h"
#include <string>
#include <utility>
#include <string.h>
#include <vector>
#include <algorithm>

using namespace std;

/*************************************************/
/** HERE WE DEFINE ALL OF THE STRUCTS THAT ARE **/
/** PASSED AROUND BY THE PARSER                **/
/*************************************************/

// structure that encapsulates a parsed computation that returns a value
struct Value {

private:

        // this points to the expression tree that computes this value
        ExprTreePtr myVal;

public:
        ~Value () {}

        Value (ExprTreePtr useMe) {
                myVal = useMe;
        }

        Value () {
                myVal = nullptr;
        }
	
	friend struct CNF;
	friend struct ValueList;
	friend struct SFWQuery;
	#include "FriendDecls.h"
};

// structure that encapsulates a parsed CNF computation
struct CNF {

private:

        // this points to the expression tree that computes this value
        vector <ExprTreePtr> disjunctions;

public:
        ~CNF () {}

        CNF (struct Value *useMe) {
              	disjunctions.push_back (useMe->myVal); 
        }

        CNF () {}

	friend struct SFWQuery;
	#include "FriendDecls.h"
};

// structure that encapsulates a parsed list of value computations
struct ValueList {

private:

        // this points to the expression tree that computes this value
        vector <ExprTreePtr> valuesToCompute;

public:
        ~ValueList () {}

        ValueList (struct Value *useMe) {
              	valuesToCompute.push_back (useMe->myVal); 
        }

        ValueList () {}

	friend struct SFWQuery;
	#include "FriendDecls.h"
};


// structure to encapsulate a create table
struct CreateTable {

private:

	// the name of the table to create
	string tableName;

	// the list of atts to create... the string is the att name
	vector <pair <string, MyDB_AttTypePtr>> attsToCreate;

	// true if we create a B+-Tree
	bool isBPlusTree;

	// the attribute to organize the B+-Tree on
	string sortAtt;

public:
	string addToCatalog (string storageDir, MyDB_CatalogPtr addToMe) {

		// make the schema
		MyDB_SchemaPtr mySchema = make_shared <MyDB_Schema>();
		for (auto a : attsToCreate) {
			mySchema->appendAtt (a);
		}

		// now, make the table
		MyDB_TablePtr myTable;

		// just a regular file
		if (!isBPlusTree) {
			myTable =  make_shared <MyDB_Table> (tableName, 
				storageDir + "/" + tableName + ".bin", mySchema);	

		// creating a B+-Tree
		} else {
			
			// make sure that we have the attribute
			if (mySchema->getAttByName (sortAtt).first == -1) {
				cout << "B+-Tree not created.\n";
				return "nothing";
			}
			myTable =  make_shared <MyDB_Table> (tableName, 
				storageDir + "/" + tableName + ".bin", mySchema, "bplustree", sortAtt);	
		}

		// and add to the catalog
		myTable->putInCatalog (addToMe);

		return tableName;
	}

	CreateTable () {}

	CreateTable (string tableNameIn, vector <pair <string, MyDB_AttTypePtr>> atts) {
		tableName = tableNameIn;
		attsToCreate = atts;
		isBPlusTree = false;
	}

	CreateTable (string tableNameIn, vector <pair <string, MyDB_AttTypePtr>> atts, string sortAttIn) {
		tableName = tableNameIn;
		attsToCreate = atts;
		isBPlusTree = true;
		sortAtt = sortAttIn;
	}
	
	~CreateTable () {}

	#include "FriendDecls.h"
};

// structure that stores a list of attributes
struct AttList {

private:

	// the list of attributes
	vector <pair <string, MyDB_AttTypePtr>> atts;

public:
	AttList (string attName, MyDB_AttTypePtr whichType) {
		atts.push_back (make_pair (attName, whichType));
	}

	~AttList () {}

	friend struct SFWQuery;
	#include "FriendDecls.h"
};

struct FromList {

private:

	// the list of tables and aliases
	vector <pair <string, string>> aliases;

public:
	FromList (string tableName, string aliasName) {
		aliases.push_back (make_pair (tableName, aliasName));
	}

	FromList () {}

	~FromList () {}
	
	friend struct SFWQuery;
	#include "FriendDecls.h"
};

enum SyntaxError{
	None,
	// for these two, check the Catalog
	AttNotExists,
	TableNotExists,
	// find substr in exprTree.toString
	SelectAttNotInGrouping,
	// for these 2 need to check every members in SFWQuery and cast type checks
	OperationOnStringAndOtherTypes,
	ArithmeticOnString,
};

// structure that stores an entire SFW query
struct SFWQuery {

private:

	// the various parts of the SQL query
	vector <ExprTreePtr> valuesToSelect;
	vector <pair <string, string>> tablesToProcess;
	vector <ExprTreePtr> allDisjunctions;
	vector <ExprTreePtr> groupingClauses;

	SyntaxError error;
	bool hasIdent;

public:
	SFWQuery () {}

	SFWQuery (struct ValueList *selectClause, struct FromList *fromClause, 
		struct CNF *cnf, struct ValueList *grouping) {
		valuesToSelect = selectClause->valuesToCompute;
		tablesToProcess = fromClause->aliases;
		allDisjunctions = cnf->disjunctions;
		groupingClauses = grouping->valuesToCompute;
	}

	SFWQuery (struct ValueList *selectClause, struct FromList *fromClause, 
		struct CNF *cnf) {
		valuesToSelect = selectClause->valuesToCompute;
		tablesToProcess = fromClause->aliases;
		allDisjunctions = cnf->disjunctions;
	}

	SFWQuery (struct ValueList *selectClause, struct FromList *fromClause) {
		valuesToSelect = selectClause->valuesToCompute;
		tablesToProcess = fromClause->aliases;
		allDisjunctions.push_back (make_shared <BoolLiteral> (true));
	}
	
	~SFWQuery () {}

	void print () {
		cout << "Selecting the following:\n";
		for (auto a : valuesToSelect) {
			cout << "\t" << a->toString () << "\n";
		}
		cout << "From the following:\n";
		for (auto a : tablesToProcess) {
			cout << "\t" << a.first << " AS " << a.second << "\n";
		}
		cout << "Where the following are true:\n";
		for (auto a : allDisjunctions) {
			cout << "\t" << a->toString () << "\n";
		}
		cout << "Group using:\n";
		for (auto a : groupingClauses) {
			cout << "\t" << a->toString () << "\n";
		}
	}
	// check all semantic error
	bool check(MyDB_CatalogPtr myCat, string& msg) {
		// from
		if (!TableInCatalog(myCat, msg)) return false;
		// where
		for(auto et : allDisjunctions) {
			string e = et->toString();
			char* vals = const_cast<char*>(e.c_str());
			if (!climbExprTree(myCat, vals, msg)) return false;
		}
		// group by
		for(auto et : groupingClauses) {
			if (!climbGroup(myCat, et, msg)) return false;
		}
		// select
		for(auto et : valuesToSelect) {
			if (!climbSelect(myCat, et, msg)) return false;
		}
		return true;
	}

	char *findsymbol (char val, char *input) {
	while (*input != val) {
		input++;
	}
	return input + 1;
}

	// traverse all nodes in tree to check type errors
	MyDB_AttTypePtr climbExprTree(MyDB_CatalogPtr myCat, char * &vals, string& msg) {
		while (true) {

		if (vals[0] == 0) {
			cout << "Reached end of string while parsing.\n";
			return nullptr;
		}

		// not equal
		if (vals[0] == '!' && vals[1] == '=') {
			vals = findsymbol ('(', vals);
			auto lres = climbExprTree (myCat, vals, msg);
			// failed, return
			if (lres == nullptr) return nullptr;
			vals = findsymbol (',', vals);
			auto rres = climbExprTree (myCat, vals, msg);
			if (rres == nullptr) return nullptr;
			vals = findsymbol (')', vals);
			// string compare to others
			if ((lres->toString() == "string" && rres->toString() != "string") || (lres->toString() != "string" && rres->toString() == "string")) {
				msg = "NEOps: can't compare [" + lres->toString() + "] and [" + rres->toString() + "].\n"; 
				return nullptr;
			}
			return make_shared<MyDB_BoolAttType>();
		// not
		} else if (vals[0] == '!') {
			vals = findsymbol ('(', vals);
			// find the result
			auto res = climbExprTree (myCat, vals, msg);
			if (res == nullptr) return nullptr;
			// find the r-paren
			vals = findsymbol (')', vals);
			if (!res->isBool()) {
				msg = "NotOps: only accept a boolean value.\n"; 
				return nullptr;
			}
			return res;
		// or
		} else if (vals[0] == '|' && vals[1] == '|') {
			vals = findsymbol ('(', vals);
			auto lres = climbExprTree (myCat, vals, msg);
			if (lres == nullptr) return nullptr;
			auto rres = climbExprTree (myCat, vals, msg);
			if (rres == nullptr) return nullptr;
			vals = findsymbol (')', vals);
			if (lres->isBool() && rres->isBool()) {
				return lres;
			} else {
				msg = "OrOps: only accept 2 boolean values.\n"; 
				return nullptr;
			}
		// plus
		} else if (vals[0] == '+') {
			vals = findsymbol ('(', vals);
			auto lres = climbExprTree (myCat, vals, msg);
			if (lres == nullptr) return nullptr;
			auto rres = climbExprTree (myCat, vals, msg);
			if (rres == nullptr) return nullptr;
			vals = findsymbol (')', vals);
			if (lres->promotableToDouble() && rres->promotableToDouble()) {
				if (lres->promotableToInt()) return rres;
				return lres;
			} else if (lres->toString() == "string" && rres->toString() == "string") {
				return lres;
			} else {
				msg = "PlusOps: can not add [" + lres->toString() + "] with [" + rres->toString() + "].\n"; 
				return nullptr;
			}
		// and
		} else if (vals[0] == '&' && vals[1] == '&') {
			vals = findsymbol ('(', vals);
			auto lres = climbExprTree (myCat, vals, msg);
			if (lres == nullptr) return nullptr;
			auto rres = climbExprTree (myCat, vals, msg);
			if (rres == nullptr) return nullptr;
			vals = findsymbol (')', vals);
			if (lres->isBool() && rres->isBool()) {
				return lres;
			} else {
				msg = "AndOps: only accept 2 boolean values.\n"; 
				return nullptr;
			}
		// equals
		} else if (vals[0] == '=' && vals[1] == '=') {
			vals = findsymbol ('(', vals);
			auto lres = climbExprTree (myCat, vals, msg);
			// failed, return
			if (lres == nullptr) return nullptr;
			vals = findsymbol (',', vals);
			auto rres = climbExprTree (myCat, vals, msg);
			if (rres == nullptr) return nullptr;
			vals = findsymbol (')', vals);
			// string compare to others
			if ((lres->toString() == "string" && rres->toString() != "string") || (lres->toString() != "string" && rres->toString() == "string")) {
				msg = "EOps: can't compare [" + lres->toString() + "] and [" + rres->toString() + "].\n"; 
				return nullptr;
			}
			return make_shared<MyDB_BoolAttType>();
		// greater than
		} else if (vals[0] == '>') {
			vals = findsymbol ('(', vals);
			auto lres = climbExprTree (myCat, vals, msg);
			// failed, return
			if (lres == nullptr) return nullptr;
			vals = findsymbol (',', vals);
			auto rres = climbExprTree (myCat, vals, msg);
			if (rres == nullptr) return nullptr;
			vals = findsymbol (')', vals);
			// string compare to others
			if ((lres->toString() == "string" && rres->toString() != "string") || (lres->toString() != "string" && rres->toString() == "string")) {
				msg = "GTOps: can't compare [" + lres->toString() + "] and [" + rres->toString() + "].\n"; 
				return nullptr;
			}
			return make_shared<MyDB_BoolAttType>();
		// less than
		} else if (vals[0] == '<') {
			vals = findsymbol ('(', vals);
			auto lres = climbExprTree (myCat, vals, msg);
			// failed, return
			if (lres == nullptr) return nullptr;
			vals = findsymbol (',', vals);
			auto rres = climbExprTree (myCat, vals, msg);
			if (rres == nullptr) return nullptr;
			vals = findsymbol (')', vals);
			// string compare to others
			if ((lres->toString() == "string" && rres->toString() != "string") || (lres->toString() != "string" && rres->toString() == "string")) {
				msg = "LTOps: can't compare [" + lres->toString() + "] and [" + rres->toString() + "].\n"; 
				return nullptr;
			}
			return make_shared<MyDB_BoolAttType>();
		// times
		} else if (vals[0] == '*') {
			vals = findsymbol ('(', vals);
			auto lres = climbExprTree (myCat, vals, msg);
			if (lres == nullptr) return nullptr;
			auto rres = climbExprTree (myCat, vals, msg);
			if (rres == nullptr) return nullptr;
			vals = findsymbol (')', vals);
			if (lres->promotableToDouble() && rres->promotableToDouble()) {
				if (lres->promotableToInt()) return rres;
				return lres;
			} else {
				msg = "TimesOps: can not time [" + lres->toString() + "] with [" + rres->toString() + "].\n"; 
				return nullptr;
			}
		// divide
		} else if (vals[0] == '/') {
			vals = findsymbol ('(', vals);
			auto lres = climbExprTree (myCat, vals, msg);
			if (lres == nullptr) return nullptr;
			auto rres = climbExprTree (myCat, vals, msg);
			if (rres == nullptr) return nullptr;
			vals = findsymbol (')', vals);
			if (lres->promotableToDouble() && rres->promotableToDouble()) {
				if (lres->promotableToInt()) return rres; 
				return lres;
			} else {
				msg = "DivOps: can not devide [" + lres->toString() + "] with [" + rres->toString() + "].\n"; 
				return nullptr;
			}
		// minus
		} else if (vals[0] == '-') {
			vals = findsymbol ('(', vals);
			auto lres = climbExprTree (myCat, vals, msg);
			if (lres == nullptr) return nullptr;
			auto rres = climbExprTree (myCat, vals, msg);
			if (rres == nullptr) return nullptr;
			vals = findsymbol (')', vals);
			if (lres->promotableToDouble() && rres->promotableToDouble()) {
				if (lres->promotableToInt()) return rres; 
				return lres;
			} else {
				msg = "MinusOps: can not minus [" + lres->toString() + "] with [" + rres->toString() + "].\n"; 
				return nullptr;
			}
		// attribute
		} else if (vals[0] == '[') {

			// find the right bracket
			vals++;
			int cnt = 0;
			for (; vals[cnt] != ']'; cnt++);

			// copy the string over
			char name[cnt + 1];
			for (cnt = 0; vals[cnt] != ']'; cnt++) {
				name[cnt] = vals[cnt];
			}	
			name[cnt] = 0;

			// find the ]
			vals = findsymbol (']', vals);
	
			return IdentifierRefed(myCat, name, msg);		
		// int
		} else if (strncmp (vals, "int", 3) == 0) {
			vals = findsymbol (']', vals);
			return make_shared<MyDB_IntAttType>();

		} else if (strncmp (vals, "double", 6) == 0) {
			vals = findsymbol (']', vals);
			return make_shared<MyDB_DoubleAttType>();

		} else if (strncmp (vals, "bool", 4) == 0) {
			vals = findsymbol (']', vals);
			return make_shared<MyDB_BoolAttType>();

		} else if (strncmp (vals, "string", 6) == 0) {
			vals = findsymbol (']', vals);
			return make_shared<MyDB_StringAttType>();
		} else if (strncmp (vals, "sum", 3) == 0) {
			vals = findsymbol ('(', vals);
			auto res = climbExprTree (myCat, vals, msg);
			if (res == nullptr) return nullptr;
			vals = findsymbol (')', vals);
			if (!res->promotableToDouble()) {
				msg = "SumOps: can not apply sum operation on [" + res->toString() + "].\n"; 
				return nullptr;
			}
			return res;
		} else if (strncmp (vals, "avg", 3) == 0) {
			vals = findsymbol ('(', vals);
			auto res = climbExprTree (myCat, vals, msg);
			if (res == nullptr) return nullptr;
			vals = findsymbol (')', vals);
			if (!res->promotableToDouble()) {
				msg = "SumOps: can not apply avg operation on [" + res->toString() + "].\n"; 
				return nullptr;
			}
			return make_shared<MyDB_DoubleAttType>();
		} else {
			vals++;
		}
	}
	}
	// check the where tables are in catalog
	bool TableInCatalog(MyDB_CatalogPtr myCat, string& msg) {
		string s;
		vector<string> tableList;
		myCat->getStringList("tables", tableList);
		for(auto table : tablesToProcess) {
			if (find(tableList.begin(), tableList.end(), table.first) == tableList.end()) {
				msg = "Error: Table [" + table.first + "] does not exist.\n";
				return false;
			}
		}
		return true;
	}
	// check all the att are in q refed table
	// and its actually in that table
	// return the type of this att
	MyDB_AttTypePtr IdentifierRefed(MyDB_CatalogPtr myCat, char* ident, string& msg) {
		hasIdent = true;
		string s_ident(ident);
		size_t pos = s_ident.find('_');
		string alias = s_ident.substr(0, pos);
		string attName = s_ident.substr(pos + 1);
		string tableName;
		for(auto item : tablesToProcess) {
			if (item.second == alias) {
				tableName = item.first;
				break;
			}
		}
		if (tableName.empty()) {
			msg = "no Table referenced as " + alias + ".\n";
			return nullptr;
		}
		MyDB_Table table;
		table.fromCatalog(tableName, myCat);
		auto att = table.getSchema()->getAttByName(attName);
		// can't find it in this table
		if (!att.second) {
			msg = "no attribute [" + attName + "] in Table [" + tableName + "].\n";
			return nullptr;
		}
		// found it, return its type
		return att.second;
	}
	// climbing the group by clause
	bool climbGroup(MyDB_CatalogPtr myCat, ExprTreePtr exprT, string& msg) {
		hasIdent = false;
		string e = exprT->toString();
		char* vals = const_cast<char*>(e.c_str());
		if (!climbExprTree(myCat, vals, msg)) return false;
		if (!hasIdent) {
			msg = "Group By: You have to group by based on at least one attribute!\n";
			return false;
		}
		return true;
	}

	bool climbSelect(MyDB_CatalogPtr myCat, ExprTreePtr exprT, string& msg) {
		string e = exprT->toString();
		char* vals = const_cast<char*>(e.c_str());
		if (!climbExprTree(myCat, vals, msg)) return false;
		if (groupingClauses.empty()) return true;
		if (e.find("sum") != string::npos || e.find("avg") != string::npos) return true;
		for(auto agg : groupingClauses) {
			string aggText = agg->toString();
			if (e.find(aggText) != string::npos) return true;
		}
		msg = "Select: When grouping, you can only select aggregates or functions of the grouping attributes\n";
		return false;
	}

	#include "FriendDecls.h"
};



// structure that sores an entire SQL statement
struct SQLStatement {

private:

	// in case we are a SFW query
	SFWQuery myQuery;
	bool isQuery;

	// in case we a re a create table
	CreateTable myTableToCreate;
	bool isCreate;

public:
	SQLStatement (struct SFWQuery* useMe) {
		myQuery = *useMe;
		isQuery = true;
		isCreate = false;
	}

	SQLStatement (struct CreateTable *useMe) {
		myTableToCreate = *useMe;
		isQuery = false;
		isCreate = true;
	}

	bool isCreateTable () {
		return isCreate;
	}

	bool isSFWQuery () {
		return isQuery;
	}

	string addToCatalog (string storageDir, MyDB_CatalogPtr addToMe) {
		return myTableToCreate.addToCatalog (storageDir, addToMe);
	}		
	
	void printSFWQuery () {
		myQuery.print ();
	}

	bool checkSFWQuery(MyDB_CatalogPtr myCat, string& msg) {
		return myQuery.check(myCat, msg);
	}

	#include "FriendDecls.h"
};

#endif
