#include <iostream>
#include <vector>

#include "Schema.h"
#include "Function.h"
#include "ParseTree.h"
#include "Statistics.h"
#include "Comparison.h"

class QueryPlan {

private:
	ostream o;

	void makeLeafs();
	void makeJoin();
	void orderJoin();
	void makeSum();
	void makeProj();
	void makeDist();
	void makeWrite();

public:
	QueryPlan(Statistics *st);

	QueryNode *root;
	vector<QueryNode*> nodes;

	Statistics *s;
	AndList *andList;

	void recycleList(AndList *alist) {
		concatList(andList, alist);
	}

	void print();
	static void concatList(AndList *&left, AndList *&right);

};

class QueryNode {

	string opName;
	Schema* outSchema;
	char* relNames[12];
	size_t numRels;
	int estimate, cost;
	Statistics *s;
	int pout;
	static int pipeId;

public:
	QueryNode(const string &op, Schema* out, Statistics* s);
	QueryNode(const string &op, Schema* out, char* rName, Statistics* s);
	QueryNode(const string &op, Schema* out, char* rNames[], size_t num, Statistics* s);

	virtual void print(size_t level = 0);
	virtual void printOperator(size_t level = 0);
	virtual void printSchema(size_t level = 0);
	virtual void printAnnot(size_t level = 0) = 0; 
	virtual void printPipe(size_t level = 0) = 0;
	virtual void printChildren(size_t level = 0)= 0;

	static AndList* pushSelection(AndList*& alist, Schema* target);
	static bool contains(OrList* ors, Schema* target);
	static bool contains(ComparisonOp* cmp, Schema* target);

};

class LeafNode : private QueryNode {

	LeafNode(AndList *&boolean, AndList *&target, char *relName, char *alias, Statistics *s);
	void printOp(size_t level = 0);
	void printAnnot(size_t level = 0);
	void printPipe(size_t level);
	void printChildren(size_t level) {}

	CNF selOp;
	Record rec;
};

class UnaryNode : protected QueryNode {

	QueryNode *child;
	int pin;

	UnaryNode(const string &opName, Schema *out, QueryNode *qn, Statistics *s);
	void printPipe(size_t level);
	void printChildren(size_t level) { 
		child->print(level + 1); 
	}

};

class BinNode : protected QueryNode {
	
	QueryNode *left;
	QueryNode *right;
	int pleft, pright;

	BinNode(const string &opName, QueryNode *l, QueryNode *r, Statistics *s);

	void printPipe(size_t level);
	void printChildren(size_t level) {

		left->print(level + 1);
		right->print(level + 1);
	}
};

class ProjectNode : private UnaryNode {

	ProjectNode(NameList *atts, QueryNode *qn);
	void printAnnot(size_t level = 0);
	int keepMe[100];
	int numAttsIn, numAttsOut;
};

class DupNode : private UnaryNode {

	DupNode(QueryNode *qn);
	void printAnnot(size_t level = 0) {}
	OrderMaker DupOrder;
};

class SumNode : private UnaryNode {

	SumNode(FuncOperator *parseTree, QueryNode *qn);
	Schema *resSchema(FuncOperator *parseTree, QueryNode *qn);
	void printAnnot(size_t level = 0);
	Function f;
};

class GroupByNode : private UnaryNode {

	GroupByNode(NameList *gAtts, FuncOperator *parseTree, QueryNode *qn);
	Schema *resSchema(NameList *gAtts, FuncOperator *parseTree, QueryNode *qn);
	void printAnnot(size_t level = 0);
	OrderMaker grpOrder;
	Function f;
};

class JoinNode : private QueryNode {

	JoinNode(AndList *&boolean, AndList *&target, QueryNode *l, QueryNode *r, Statistics *s);
	void printAnnot(size_t level = 0);
	CNF selOp;
	Record rec;
};

class WriteNode : private QueryNode {

	FILE *out;
	WriteNode(FILE *out, QueryNode *qn);
	void printAnnot(size_t level = 0);
};
