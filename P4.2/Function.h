#include "Record.h"
#include "ParseTree.h"

enum AritmeticOp {
	PushInt, PushDouble, ToDouble, IntUnaryMinus, IntMinus, IntPlus,
	IntDiv, IntMul, DblUnaryMinus, DblMinus, DblPlus, DblDiv, DblMul
};

struct Arithmetic {
	AritmeticOp *opList;
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

	void GrowFromParseTree(struct FuncOperator *parseTree, Schema &mySchema);

	Type BuildRecursively(struct FuncOperator *parseTree, Schema &mySchema);

	void print();

	Type Apply(Record &toMe, int &intResult, double &doubleResult);

	template <class T>
	T Apply(Record& toMe) {
		int intResult; double doubleResult;
		Apply(toMe, intResult, doubleResult);
		return returnsInt ? intResult : doubleResult;
	}

	Type resultType() { 
		return returnsInt ? Int : Double; 
	}
};