
#ifndef SFW_QUERY_CC
#define SFW_QUERY_CC

#include "SFWQuery.h"
#include "ExprTree.h"
#include "MyDB_LogicalOps.h"
#include "MyDB_Schema.h"
#include "MyDB_Stats.h"
#include "MyDB_Table.h"
#include "ParserTypes.h"
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

size_t SFWQuery::tempCount = 0;

string SFWQuery::generateKey(const vector<string>& subset) {
	vector<string> sortedSubset = subset;
	sort(sortedSubset.begin(), sortedSubset.end());
	string key;
	for(const string& s : sortedSubset) {
		key += s + ", ";
	}
	return key;
}

// builds and optimizes a logical query plan for a SFW query, returning the logical query plan
pair <LogicalOpPtr, double> SFWQuery :: optimizeQueryPlan (map <string, MyDB_TablePtr> &allTables) {
	// here we call the recursive, exhaustive enum. algorithm
	// but first lets create a output Schema (from valuesToSelect) that has atts with a format of alias_attName
	MyDB_SchemaPtr finalProjectSchema = make_shared<MyDB_Schema>();
	// its in our valuesToSelect
	// look into all the tables refed
	for(auto p : tablesToProcess) {
		// get all the atts of this table
		auto myAtts = allTables[p.first]->getSchema()->getAtts();
		for(auto att : myAtts) {
			// see if it is refed by the select clause
			for(auto e : valuesToSelect) {
				if (e->referencesAtt(p.second, att.first)) {
					// remember we put att with name of this format: alias_attName in the schema
					finalProjectSchema->appendAtt({p.second + '_' + att.first, att.second});
					break;
				}
			}	
		}
	}
	// put all the table with different alias in the map
	map<string, MyDB_TablePtr> refedTable;
	for(auto p : tablesToProcess) {
		// change the att name while we do this
		refedTable[p.second] = allTables[p.first]->alias(p.second);
	}
	return optimizeQueryPlan(refedTable, finalProjectSchema, allDisjunctions);
}

// builds and optimizes a logical query plan for a SFW query, returning the logical query plan
pair <LogicalOpPtr, double> SFWQuery :: optimizeQueryPlan (map <string, MyDB_TablePtr> &allTables, 
	MyDB_SchemaPtr totSchema, vector <ExprTreePtr> &allDisjunctions) {
	
	MyDB_StatsPtr myStats = nullptr;
	LogicalOpPtr res = nullptr;
	LogicalOpPtr bestRes = nullptr;
	double cost = 9e99;
	double best = 9e99;

	// some code here...

	// case where no joins
	if (allTables.size () == 1) {
		MyDB_TablePtr thisTable = allTables.begin()->second;
		myStats = make_shared<MyDB_Stats>(thisTable);
		myStats = myStats->costSelection(allDisjunctions);
		best = myStats->getTupleCount();
		// cout << "Cost is " << best << ".\n";
		// create a tempTable for the output
		MyDB_TablePtr temp = make_shared<MyDB_Table>("tempTable" + to_string(tempCount), "tempTableLoc" + to_string(tempCount), totSchema);
		tempCount++;
		res = make_shared<LogicalTableScan>(thisTable, temp, myStats, allDisjunctions);
		
		return make_pair (res, best);
	}
	// we have at least one join
	vector<string> tables;
	for(const auto& item : allTables) {
		tables.push_back(item.first);
	}
	// we've done this before, use the memorized value
	if (subsetsDP.find(generateKey(tables)) != subsetsDP.end()) {
		return subsetsDP[generateKey(tables)];
	}
	int n = tables.size();
	// no emptyset (0) and it self (11111...111)
	for(int i = 1; i < ((1 << n) - 1); i++) {
		vector<string> leftSub;
		vector<string> rightSub;
		for(int j = 0; j < n; j++) {
			if (i & (1 << j)) {
				leftSub.push_back(tables[j]);
			} else {
				rightSub.push_back(tables[j]);
			}
		}
		// split the exprs
		vector<ExprTreePtr> leftExpr;
		vector<ExprTreePtr> rightExpr;
		vector<ExprTreePtr> topExpr;
		
		for(auto e : allDisjunctions) {
			bool refLeft = false, refRight = false;
			for(const auto& s : leftSub) {
				if (e->referencesTable(s)) {
					refLeft = true;
					break;
				}
			}
			for(const auto& s : rightSub) {
				if (e->referencesTable(s)) {
					refRight = true;
					break;
				}
			}
			if (refLeft && refRight) {
				topExpr.push_back(e);
			} else if (refLeft) {
				leftExpr.push_back(e);
			} else if (refRight) {
				rightExpr.push_back(e);
			} else {
				cout << "DEBUG: there is an expr that doesn't ref any of these tables\n";
			}
		}
		// split the attribute
		// the name of the attrs in schema should have a format of alias_attName
		auto projectAtt = totSchema->getAtts();
		MyDB_SchemaPtr leftSchema = make_shared<MyDB_Schema>();
		for(int k = 0; k < leftSub.size(); k++) {
			MyDB_TablePtr thisTable = allTables[leftSub[k]];
			auto theseAtts = thisTable->getSchema()->getAtts();
			string myAlias = leftSub[k];
			for(auto att : theseAtts) {
				bool inProject = false, inTopExpr = false;
				for(auto p : projectAtt) {
					// alias_attName in totSchema
					if (att.first == p.first) {
						inProject = true;
						break;
					}
				}
				for(auto e : topExpr) {
					// we need this at the top
					if (e->referencesAtt(leftSub[k], att.first.substr(myAlias.size() + 1))) {
						inTopExpr = true;
						break;
					}
				}
				// Atts(Left) ^ (A U Atts(TopCNF))
				if (inProject) {
					leftSchema->appendAtt(att);
				} else if (inTopExpr) {
					leftSchema->appendAtt(att);
				}
			}
		}
		MyDB_SchemaPtr rightSchema = make_shared<MyDB_Schema>();
		for(int k = 0; k < rightSub.size(); k++) {
			MyDB_TablePtr thisTable = allTables[rightSub[k]];
			auto theseAtts = thisTable->getSchema()->getAtts();
			string myAlias = rightSub[k];
			for(auto att : theseAtts) {
				bool inProject = false, inTopExpr = false;
				for(auto p : projectAtt) {
					// alias_attName in totSchema
					if (att.first == p.first) {
						inProject = true;
						break;
					}
				}
				for(auto e : topExpr) {
					// we need this at the top
					if (e->referencesAtt(myAlias, att.first.substr(myAlias.size() + 1))) {
						inTopExpr = true;
						break;
					}
				}
				// Atts(right) ^ (A U Atts(TopCNF))
				if (inProject) {
					rightSchema->appendAtt(att);
				} else if (inTopExpr) {
					rightSchema->appendAtt(att);
				}
			}
		}
		pair<LogicalOpPtr, double> leftRes = {nullptr, 9e99};
		pair<LogicalOpPtr, double> rightRes = {nullptr, 9e99};
		// we've done this before, use the memorized value
		if (subsetsDP.find(generateKey(leftSub)) != subsetsDP.end()) {
			leftRes = subsetsDP[generateKey(leftSub)] ;
		} else {
			// call this function on the left
			map<string, MyDB_TablePtr> leftTable;
			for(const auto& s : leftSub) {
				leftTable[s] = allTables[s];
			}
			leftRes = optimizeQueryPlan(leftTable, leftSchema, leftExpr);
		}
		if (subsetsDP.find(generateKey(rightSub)) != subsetsDP.end()) {
			rightRes = subsetsDP[generateKey(rightSub)];
		} else {
			map<string, MyDB_TablePtr> rightTable;
			for(const auto& s : rightSub) {
				rightTable[s] = allTables[s];
			}
			rightRes = optimizeQueryPlan(rightTable, rightSchema, rightExpr);
		}
		myStats = leftRes.first->getStats()->costJoin(topExpr, rightRes.first->getStats());
		cost = leftRes.second + rightRes.second + myStats->getTupleCount();
		// now we have everything we need
		// create a LogicalOps object
		MyDB_TablePtr temp = make_shared<MyDB_Table>("tempTable" + to_string(tempCount), "tempTableLoc" + to_string(tempCount), totSchema);
		tempCount++;
		res = make_shared<LogicalJoin>(leftRes.first, rightRes.first, temp, topExpr, myStats);
		if (cost < best) {
			bestRes = res;
			best = cost;
		}
	}
	subsetsDP[generateKey(tables)] = {bestRes, best};
	return make_pair (bestRes, best);
}

void SFWQuery :: print () {
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


SFWQuery :: SFWQuery (struct ValueList *selectClause, struct FromList *fromClause,
        struct CNF *cnf, struct ValueList *grouping) {
        valuesToSelect = selectClause->valuesToCompute;
        tablesToProcess = fromClause->aliases;
        allDisjunctions = cnf->disjunctions;
        groupingClauses = grouping->valuesToCompute;
}

SFWQuery :: SFWQuery (struct ValueList *selectClause, struct FromList *fromClause,
        struct CNF *cnf) {
        valuesToSelect = selectClause->valuesToCompute;
        tablesToProcess = fromClause->aliases;
	allDisjunctions = cnf->disjunctions;
}

SFWQuery :: SFWQuery (struct ValueList *selectClause, struct FromList *fromClause) {
        valuesToSelect = selectClause->valuesToCompute;
        tablesToProcess = fromClause->aliases;
        allDisjunctions.push_back (make_shared <BoolLiteral> (true));
}

#endif
