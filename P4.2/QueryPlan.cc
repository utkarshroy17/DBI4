#include "QueryPlan.h"

extern char *dbfile_dir;
extern char *tpch_dir;
extern char *catalog_path;

extern FuncOperator* finalFunction;
extern TableList* tables;
extern AndList* boolean;
extern NameList* groupingAtts;
extern NameList* attsToSelect;
extern int distinctAtts;
extern int distinctFunc;

#define makeNode(pushed, recycler, nodeType, newNode, params)           \
  AndList* pushed;                                                      \
  nodeType* newNode = new nodeType params;                              \
  concatList(recycler, pushed);

QueryPlan::QueryPlan(Statistics *st) : s(st), andList(NULL) {
	makeLeafs();
	makeJoin();
	makeSum();
	makeProj();
	makeDist();
	makeWrite();

	print();
}

void QueryPlan::print() {
	root->print();
}

void QueryPlan::makeLeafs() {

	for (TableList *table = tables; table; table = table->next) {
		
		s->CopyRel(table->tableName, table->aliasAs);
		AndList *target;
		makeNode(target, used, LeafNode, newLeaf, (boolean, pushed, table->tableName, table->aliasAs, s));
		
	}
}