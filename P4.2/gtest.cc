//#include "y.tab.h"
#include <iostream>
#include <stdlib.h>
#include "Statistics.h"
#include "ParseTree.h"
#include <math.h>
#include <gtest/gtest.h>

extern "C" struct YY_BUFFER_STATE *yy_scan_string(const char*);
extern "C" int yyparse(void);
struct AndList *final;

char *fileName = "Statistics1.txt";

Statistics s, s1;
char *relName[] = { "supplier","partsupp" };
char *cnf = "(s_suppkey = ps_suppkey)";
double result;

TEST(DBFileGoogleTest0, q0) {

	bool isError = false;

	try {
		s.AddRel(relName[0], 10000);
		s.AddAtt(relName[0], "s_suppkey", 10000);

		s.AddRel(relName[1], 800000);
		s.AddAtt(relName[1], "ps_suppkey", 10000);
	}
	catch (...) {
		isError = true;
	}
	
	EXPECT_EQ(isError, false);
}

TEST(DBFileGoogleTest1, q1) {	

	yy_scan_string(cnf);
	yyparse();
	result = s.Estimate(final, relName, 2);

	cout << "Result = " << result << endl;

	EXPECT_EQ(result, 800000);
}

TEST(DBFileGoogleTest2, q2) {

	bool isError = false;

	try {
		s.Apply(final, relName, 2);
	}
	catch (...) {
		isError = true;
	}
	
	EXPECT_EQ(isError, false);
}

TEST(DBFileGoogleTest3, q3) {

	bool isError = false;

	try {
		s.Write(fileName);
	}
	catch (...) {
		isError = true;
	}

	EXPECT_EQ(isError, false);	
}

TEST(DBFileGoogleTest4, q4) {

	bool isError = false;

	try {
		s1.Read(fileName);
	}
	catch (...) {
		isError = true;
	}

	EXPECT_EQ(isError, false);	
}

TEST(DBFileGoogleTest5, q5) {
	cnf = "(s_suppkey>1000)";
	yy_scan_string(cnf);
	yyparse();
	double dummy = s1.Estimate(final, relName, 2);

	EXPECT_EQ(round(fabs(dummy*3.0 - result)) > 0.1, false);
}



int main(int argc, char *argv[]) {

	::testing::InitGoogleTest(&argc, argv);
	
	return RUN_ALL_TESTS();
}
