#include <iostream>
#include <vector>

#include "Schema.h"
#include "Function.h"
#include "ParseTree.h"
#include "Statistics.h"
#include "Comparison.h"

#define MAX_RELS 12
#define MAX_ATTS 100

class QueryNode;
class QueryPlan {
public:
	QueryPlan(Statistics* st);
	~QueryPlan() {}

	void print();

private:
	void makeLeafs();
	void makeJoins();
	void orderJoins();
	void makeSums();
	void makeProj();
	void makeDistinct();
	void makeWrite();
	int evalOrder(std::vector<QueryNode*> operands, Statistics st, int bestFound); 

	QueryNode* root;
	std::vector<QueryNode*> nodes;

	Statistics* stat;
	AndList* andlist;  

	void recycleList(AndList* alist) { concatList(andlist, alist); }
	static void concatList(AndList*& left, AndList*& right);

	QueryPlan(const QueryPlan&);
	QueryPlan& operator=(const QueryPlan&);
};

class QueryNode {
	friend class QueryPlan;
	friend class UnaryNode;
	friend class BinaryNode;   
	friend class ProjectNode;
	friend class RemDupNode;
	friend class JoinNode;
	friend class SumNode;
	friend class GroupByNode;
	friend class WriteNode;
public:
	virtual ~QueryNode();

protected:
	QueryNode(const std::string& op, Schema* out, Statistics* st);
	QueryNode(const std::string& op, Schema* out, char* rName, Statistics* st);
	QueryNode(const std::string& op, Schema* out, char* rNames[], size_t num, Statistics* st);

	virtual void print(size_t level = 0);
	virtual void printOp(size_t level = 0);
	virtual void printAnnot(size_t level = 0) = 0; // operator specific
	virtual void printPipe(size_t level = 0) = 0;
	virtual void printChildren(size_t level = 0) = 0;

	static AndList* pushSelection(AndList*& alist, Schema* target);
	static bool containedIn(OrList* ors, Schema* target);
	static bool containedIn(ComparisonOp* cmp, Schema* target);

	string opName;
	Schema* outSchema;
	char* relNames[MAX_RELS];
	size_t numRels;
	int estimate, cost;  // estimated number of tuples and total cost
	Statistics* stat;
	int pout;  // output pipe
	static int pipeId;
};

class LeafNode : private QueryNode {  // read from file
	friend class QueryPlan;
	LeafNode(AndList*& boolean, AndList*& pushed,
		char* relName, char* alias, Statistics* st);
	void printOp(size_t level = 0);
	void printAnnot(size_t level = 0);
	void printPipe(size_t level);
	void printChildren(size_t level) {}
	CNF selOp;
	Record literal;
};

class UnaryNode : protected QueryNode {
	friend class QueryPlan;
protected:
	UnaryNode(const std::string& opName, Schema* out, QueryNode* c, Statistics* st);
	virtual ~UnaryNode() { delete child; }
	void printPipe(size_t level);
	void printChildren(size_t level){ child->print(level + 1); }
	QueryNode* child;
	int pin;  
};

class BinaryNode : protected QueryNode {  
	friend class QueryPlan;
protected:
	BinaryNode(const std::string& opName, QueryNode* l, QueryNode* r, Statistics* st);
	virtual ~BinaryNode() { delete left; delete right; }
	void printPipe(size_t level);
	void printChildren(size_t level)
	{
		left->print(level + 1); right->print(level + 1);
	}
	QueryNode* left;
	QueryNode* right;
	int pleft, pright; 
};

class ProjectNode : private UnaryNode {
	friend class QueryPlan;
	ProjectNode(NameList* atts, QueryNode* c);
	void printAnnot(size_t level = 0);
	int keepMe[MAX_ATTS];
	int numAttsIn, numAttsOut;
};

class RemDupNode : private UnaryNode {
	friend class QueryPlan;
	RemDupNode(QueryNode* c);
	void printAnnot(size_t level = 0){}
	OrderMaker remDupOrder;
};

class SumNode : private UnaryNode {
	friend class QueryPlan;
	SumNode(FuncOperator* parseTree, QueryNode* c);
	Schema* resultSchema(FuncOperator* parseTree, QueryNode* c);
	void printAnnot(size_t level = 0);
	Function f;
};

class GroupByNode : private UnaryNode {
	friend class QueryPlan;
	GroupByNode(NameList* gAtts, FuncOperator* parseTree, QueryNode* c);
	Schema* resultSchema(NameList* gAtts, FuncOperator* parseTree, QueryNode* c);
	void printAnnot(size_t level = 0);
	OrderMaker grpOrder;
	Function f;
};

class JoinNode : private BinaryNode {
	friend class QueryPlan;
	JoinNode(AndList*& boolean, AndList*& pushed, QueryNode* l, QueryNode* r, Statistics* st);
	void printAnnot(size_t level = 0);
	CNF selOp;
	Record literal;
};

class WriteNode : private UnaryNode {
	friend class QueryPlan;
	WriteNode(FILE* out, QueryNode* c);
	void printAnnot(size_t level = 0);
	FILE* outFile;
};

