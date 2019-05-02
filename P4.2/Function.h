#ifndef FUNCTION_H_
#define FUNCTION_H_

#include "Record.h"
#include "ParseTree.h"

#define MAX_DEPTH 100


enum ArithOp {
	PushInt, PushDouble, ToDouble, ToDouble2Down,
	IntUnaryMinus, IntMinus, IntPlus, IntDivide, IntMultiply,
	DblUnaryMinus, DblMinus, DblPlus, DblDivide, DblMultiply
};

struct Arithmetic {

	ArithOp myOp;
	int recInput;
	void *litInput;
};

class Function {

private:

	Arithmetic *opList;
	int numOps;

	int returnsInt;

public:

	Function();

	// this grows the specified function from a parse tree and converts
	// it into an accumulator-based computation over the attributes in
	// a record with the given schema; the record "literal" is produced
	// by the GrowFromParseTree method
	void GrowFromParseTree(struct FuncOperator *parseTree, Schema &mySchema);

	Type RecursivelyBuild(struct FuncOperator *parseTree, Schema &mySchema);

	// prints out the function to the screen
	void Print();

	Type Apply(Record &toMe, int &intResult, double &doubleResult);

	template <class T>
	T Apply(Record& toMe) {
		int intResult; double doubleResult;
		Apply(toMe, intResult, doubleResult);
		return returnsInt ? intResult : doubleResult;
	}

	Type resultType() const { return returnsInt ? Int : Double; }
};

#endif
