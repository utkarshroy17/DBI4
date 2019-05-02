#include <cstring>
#include <climits>
#include <string>
#include <algorithm>

#include "Defs.h"
#include "QueryPlan.h"

extern char* catalog_path;
extern char* dbfile_dir;
extern char* tpch_dir;

// parser variables
extern FuncOperator* finalFunction;
extern TableList* tables;
extern AndList* boolean;
extern NameList* groupingAtts;
extern NameList* attsToSelect;
extern int distinctAtts;
extern int distinctFunc;

QueryPlan::QueryPlan(Statistics* st) : stat(st), andlist(NULL) {
	makeLeafs();  
	makeJoins();
	makeSums();
	makeProj();
	makeDistinct();
	makeWrite();

	print();
}

void QueryPlan::print() {
	root->print();
}

void QueryPlan::makeLeafs() {
	for (TableList* table = tables; table; table = table->next) {
		stat->CopyRel(table->tableName, table->aliasAs);
		AndList *pushed;
		LeafNode *newLeaf = new LeafNode(boolean, pushed, table->tableName, table->aliasAs, stat);
		concatList(andlist, pushed);
		nodes.push_back(newLeaf);

	}
}

void QueryPlan::makeJoins() {
	orderJoins();
	while (nodes.size() > 1) {
		QueryNode* node1 = nodes.back();                  
		nodes.pop_back();                               
		QueryNode* node2 = nodes.back();                  
		nodes.pop_back();
		AndList *pushed;
		JoinNode *newJoin = new JoinNode(boolean, pushed, node1, node2, stat);
		concatList(andlist, pushed);
		nodes.push_back(newJoin);
	}
	root = nodes.front();
}

void QueryPlan::makeSums() {
	if (groupingAtts) {
		if (distinctFunc) root = new RemDupNode(root);
		root = new GroupByNode(groupingAtts, finalFunction, root);
	}
	else if (finalFunction) {
		root = new SumNode(finalFunction, root);
	}
}

void QueryPlan::makeProj() {
	if (attsToSelect && !finalFunction && !groupingAtts) 
		root = new ProjectNode(attsToSelect, root);
}

void QueryPlan::makeDistinct() {
	if (distinctAtts) 
		root = new RemDupNode(root);
}

void QueryPlan::makeWrite() {
	root = new WriteNode(stdout, root);
}

void QueryPlan::orderJoins() {
	std::vector<QueryNode*> operands(nodes);
	sort(operands.begin(), operands.end());
	int minCost = INT_MAX, cost;
	do { 
		if ((cost = evalOrder(operands, *stat, minCost)) < minCost && cost > 0) {
			minCost = cost; nodes = operands;
		}
	} while (next_permutation(operands.begin(), operands.end()));
}

int QueryPlan::evalOrder(std::vector<QueryNode*> operands, Statistics st, int bestFound) {  
	std::vector<JoinNode*> freeList;  
	AndList* recycler = NULL;         
	while (operands.size() > 1) {       
		

		QueryNode* node1 = operands.back();
		operands.pop_back();
		QueryNode* node2 = operands.back();
		operands.pop_back();
		AndList *pushed;
		JoinNode *newJoin = new JoinNode(boolean, pushed, node1, node2, &st);
		concatList(recycler, pushed);
		operands.push_back(newJoin);
		freeList.push_back(newJoin);
		if (newJoin->estimate <= 0 || newJoin->cost > bestFound) 
			break;  
	}
	int cost = operands.back()->cost;
	
	concatList(boolean, recycler);   
	return operands.back()->estimate < 0 ? -1 : cost;
}

void QueryPlan::concatList(AndList*& left, AndList*& right) {
	if (!left) { swap(left, right); return; }
	AndList *pre = left, *cur = left->rightAnd;
	for (; cur; pre = cur, cur = cur->rightAnd);
	pre->rightAnd = right;
	right = NULL;
}

int QueryNode::pipeId = 0;

QueryNode::QueryNode(const std::string& op, Schema* out, Statistics* st) :
	opName(op), outSchema(out), numRels(0), estimate(0), cost(0), stat(st), pout(pipeId++) {}

QueryNode::QueryNode(const std::string& op, Schema* out, char* rName, Statistics* st) :
	opName(op), outSchema(out), numRels(0), estimate(0), cost(0), stat(st), pout(pipeId++) {
	cout << "inside query node" << endl;
	if (rName) relNames[numRels++] = strdup(rName);
}

QueryNode::QueryNode(const std::string& op, Schema* out, char* rNames[], size_t num, Statistics* st) :
	opName(op), outSchema(out), numRels(0), estimate(0), cost(0), stat(st), pout(pipeId++) {
	for (; numRels < num; ++numRels)
		relNames[numRels] = strdup(rNames[numRels]);
}

QueryNode::~QueryNode() {
	delete outSchema;
	for (size_t i = 0; i < numRels; ++i)
		delete relNames[i];
}

AndList* QueryNode::pushSelection(AndList*& alist, Schema* target) {
	AndList header; header.rightAnd = alist;  
	
	AndList *cur = alist, *pre = &header, *result = NULL;
	for (; cur; cur = pre->rightAnd)
		if (containedIn(cur->left, target)) {   
			pre->rightAnd = cur->rightAnd;
			cur->rightAnd = result;       
			result = cur;        
		}
		else pre = cur;
	alist = header.rightAnd;  
	return result;
}

bool QueryNode::containedIn(OrList* ors, Schema* target) {
	for (; ors; ors = ors->rightOr)
		if (!containedIn(ors->left, target)) return false;
	return true;
}

bool QueryNode::containedIn(ComparisonOp* cmp, Schema* target) {
	Operand *left = cmp->left, *right = cmp->right;
	return (left->code != NAME || target->Find(left->value) != -1) &&
		(right->code != NAME || target->Find(right->value) != -1);
}

LeafNode::LeafNode(AndList*& boolean, AndList*& pushed, char* relName, char* alias, Statistics* st) :
	QueryNode("Select File", new Schema(catalog_path, relName, alias), relName, st) {
	cout << "inside leaf node" << endl;
	pushed = pushSelection(boolean, outSchema);
	estimate = stat->Estimate(pushed, relNames, numRels);
	selOp.GrowFromParseTree(pushed, outSchema, literal);
}

UnaryNode::UnaryNode(const std::string& opName, Schema* out, QueryNode* c, Statistics* st) :
	QueryNode(opName, out, c->relNames, c->numRels, st), child(c), pin(c->pout) {}

BinaryNode::BinaryNode(const std::string& opName, QueryNode* l, QueryNode* r, Statistics* st) :
	QueryNode(opName, new Schema(*l->outSchema, *r->outSchema), st),
	left(l), right(r), pleft(left->pout), pright(right->pout) {
	for (size_t i = 0; i < l->numRels;)
		relNames[numRels++] = strdup(l->relNames[i++]);
	for (size_t j = 0; j < r->numRels;)
		relNames[numRels++] = strdup(r->relNames[j++]);
}

ProjectNode::ProjectNode(NameList* atts, QueryNode* c) :
	UnaryNode("Project", NULL, c, NULL), numAttsIn(c->outSchema->GetNumAtts()), numAttsOut(0) {
	Attribute resultAtts[MAX_ATTS];
	for (; atts; atts = atts->next, numAttsOut++) {
		resultAtts[numAttsOut].name = atts->name;
		resultAtts[numAttsOut].myType = c->outSchema->FindType(atts->name);
	}
	outSchema = new Schema("", numAttsOut, resultAtts);
}

RemDupNode::RemDupNode(QueryNode* c) :
	UnaryNode("Remove Duplicates", c->outSchema, c, NULL), remDupOrder(c->outSchema) {}

JoinNode::JoinNode(AndList*& boolean, AndList*& pushed, QueryNode* l, QueryNode* r, Statistics* st) :
	BinaryNode("Join", l, r, st) {
	pushed = pushSelection(boolean, outSchema);
	estimate = stat->Estimate(pushed, relNames, numRels);	
	cost = l->cost + estimate + r->cost;
	selOp.GrowFromParseTree(pushed, outSchema, literal);
}

SumNode::SumNode(FuncOperator* parseTree, QueryNode* c) :
	UnaryNode("Sum", resultSchema(parseTree, c), c, NULL) {
	f.GrowFromParseTree(parseTree, *c->outSchema);
}

Schema* SumNode::resultSchema(FuncOperator* parseTree, QueryNode* c) {
	Function fun;
	Attribute atts[2][1] = { {{"sum", Int}}, {{"sum", Double}} };
	fun.GrowFromParseTree(parseTree, *c->outSchema);
	return new Schema("", 1, atts[fun.resultType()]);
}

GroupByNode::GroupByNode(NameList* gAtts, FuncOperator* parseTree, QueryNode* c) :
	UnaryNode("Group by", resultSchema(gAtts, parseTree, c), c, NULL) {
	grpOrder.growFromParseTree(gAtts, c->outSchema);
	f.GrowFromParseTree(parseTree, *c->outSchema);
}

Schema* GroupByNode::resultSchema(NameList* gAtts, FuncOperator* parseTree, QueryNode* c) {
	Function fun;
	Attribute atts[2][1] = { {{"sum", Int}}, {{"sum", Double}} };
	Schema* cSchema = c->outSchema;
	fun.GrowFromParseTree(parseTree, *cSchema);
	Attribute resultAtts[MAX_ATTS];
	resultAtts[0].name = "sum";
	resultAtts[0].myType = fun.resultType();
	int numAtts = 1;
	for (; gAtts; gAtts = gAtts->next, numAtts++) {
		resultAtts[numAtts].name = gAtts->name;
		resultAtts[numAtts].myType = c->outSchema->FindType(gAtts->name);
	}
	return new Schema("", numAtts, resultAtts);
}

WriteNode::WriteNode(FILE* out, QueryNode* c) :
	UnaryNode("WriteOut", c->outSchema, c, NULL), outFile(out) {}

void QueryNode::print(size_t level) {
	printOp(level);
	printAnnot(level);
	printPipe(level);
	printChildren(level);
}

void QueryNode::printOp(size_t level) {
	cout << (string(3 * (level), ' ') + "-> ") << opName << ": ";
}

void LeafNode::printPipe(size_t level) {
	cout << (string(3 * (level + 1), ' ') + "* ") << "Output pipe: " << pout << endl;
}

void UnaryNode::printPipe(size_t level) {
	cout << (string(3 * (level + 1), ' ') + "* ") << "Output pipe: " << pout << endl;
	cout << (string(3 * (level + 1), ' ') + "* ") << "Input pipe: " << pin << endl;
}

void BinaryNode::printPipe(size_t level) {
	cout << (string(3 * (level + 1), ' ') + "* ") << "Output pipe: " << pout << endl;
	cout << (string(3 * (level + 1), ' ') + "* ") << "Input pipe: " << pleft << ", " << pright << endl;
}

void LeafNode::printOp(size_t level) {
	cout << (string(3 * (level), ' ') + "-> ") << "Select from " << relNames[0] << ": ";
}

void LeafNode::printAnnot(size_t level) {
	selOp.Print();
}

void ProjectNode::printAnnot(size_t level) {
	cout << keepMe[0];
	for (size_t i = 1; i < numAttsOut; ++i) cout << ',' << keepMe[i];
	cout << endl;
}

void JoinNode::printAnnot(size_t level) {
	selOp.Print();
	cout << (string(3 * (level + 1), ' ') + "* ") << "Estimate = " << estimate << ", Cost = " << cost << endl;
}

void SumNode::printAnnot(size_t level) {
	cout << (string(3 * (level + 1), ' ') + "* ") << "Function: "; (const_cast<Function*>(&f))->Print();
}

void GroupByNode::printAnnot(size_t level) {
	cout << (string(3 * (level + 1), ' ') + "* ") << "OrderMaker: "; (const_cast<OrderMaker*>(&grpOrder))->Print();
	cout << (string(3 * (level + 1), ' ') + "* ") << "Function: "; (const_cast<Function*>(&f))->Print();
}

void WriteNode::printAnnot(size_t level) {
	cout << (string(3 * (level + 1), ' ') + "* ") << "Output to " << outFile << endl;
}
