
#include <iostream>
#include "ParseTree.h"
//#include "Statistics.h"
#include "QueryPlan.h"

using namespace std;

const char *dbfile_dir = ""; // dir where binary heap files should be stored
const char *tpch_dir = "./"; // dir where dbgen tpch files (extension *.tbl) can be found
const char *catalog_path = "catalog"; // full path of the catalog file

extern "C" {
	int yyparse(void);   // defined in y.tab.c
}

int main () {

	cout << " Enter CNF predicate (when done press ctrl-D):\n\t";
	
	if (yyparse() != 0) {
		std::cout << "Can't parse your CNF.\n";
		exit(1);
	}

	char *fileName = "Statistics.txt";
	Statistics s;

	s.Read(fileName);

	QueryPlan plan(&s);
	return 0;
	
}


