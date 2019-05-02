#include "Statistics.h"
#include <map>
#include <string.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdio.h>
#include <set>
#include <math.h>
#define DEP

Statistics::Statistics()
{
	calledFromApply = false;
	isApply = false;
}
Statistics::Statistics(Statistics &copyMe)
{
	this->relMap = copyMe.relMap;
	this->attrMap = copyMe.attrMap;
}
Statistics::~Statistics()
{
}

void Statistics::AddRel(char *relName, int numTuples)
{
	string rel(relName);
	map<string, int>::iterator it = relMap.find(rel);

	if (it == relMap.end()) {
		relMap.insert(pair<string, int>(rel, numTuples));
	}
	else {
		it->second = numTuples;
	}

}
void Statistics::AddAtt(char *relName, char *attName, int numDistincts)
{
	string rel(relName);
	string att(attName);
	
	if (numDistincts == -1) {
		attrMap[relName][attName] = relMap[relName];
	}
	else
		attrMap[relName][attName] = numDistincts;
}
void Statistics::CopyRel(char *oldName, char *newName)
{
	string oldRel(oldName);
	string newRel(newName);

	relMap[newRel] = relMap[oldRel];
	
	map<string, int> oldAttrMap = attrMap[oldRel];

	for (map<string, int>::iterator it = oldAttrMap.begin(); it != oldAttrMap.end(); ++it) {

		string newAtt = newRel + "." + it->first;
		attrMap[newRel][newAtt] = it->second;
	}
}
	
void Statistics::Read(char *fromWhere)
{
	string file(fromWhere);
			

	int relCountInRel, relCountInAttr;

	/*if (!inFile)
		return;*/

	ifstream inFile;
	inFile.open(file.c_str(), ios::in);

	string line;
	inFile >> line;
	relCountInRel = atoi(line.c_str());

	relMap.clear();

	for (int i = 0; i < relCountInRel; i++)
	{
		inFile >> line;

		size_t splitPos = line.find_first_of("#");
		string first = line.substr(0, splitPos);
		string sec = line.substr(splitPos + 1);

		relMap[first] = atoi(sec.c_str());
	}

	inFile >> line;

	relCountInAttr = atoi(line.c_str());

	attrMap.clear();

	string relName, attrName, numDistincts;

	while (!inFile.eof()) {
			
		attrMap[relName][attrName] = atoi(numDistincts.c_str());
		inFile >> relName >> attrName >> numDistincts;
	}

	inFile.close();
	
}
void Statistics::Write(char *fromWhere)
{
	string file(fromWhere);
	remove(fromWhere);

	ofstream outFile;
	outFile.open(file.c_str(), ios::out);

	outFile << relMap.size() << "\n";

	for (map<string,int>::iterator it = relMap.begin(); it != relMap.end(); it++)
	{	
		const char *first = it->first.c_str();
		outFile << first << "#" << it->second << "\n";
	}

	outFile << attrMap.size() << "\n";

	for (map<string, map<string, int>>::iterator it1 = attrMap.begin(); it1 != attrMap.end(); ++it1)
	{		
		//map<string, int> attr = it1->second;
		/*outFile << attr.size() << "\n";*/

		for (map<string, int>::iterator it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			const char *first = it1->first.c_str();
			const char *sec = it2->first.c_str();			
			outFile << first << " " << sec << " " << it2->second << "\n";
		}
	}

	outFile.close();
}

void  Statistics::Apply(struct AndList *parseTree, char *relNames[], int numToJoin)
{
	calledFromApply = true;
	isApply = true;
	Estimate(parseTree, relNames, numToJoin);
	calledFromApply = false;
	isApply = false;
}
double Statistics::Estimate(struct AndList *parseTree, char **relNames, int numToJoin)
{

	double resultEstimate = 0.0;

	struct AndList *currentAnd;
	struct OrList *currentOr;

	currentAnd = parseTree;

	string leftRel;
	string rightRel;

	string leftAttr;
	string rightAttr;

	string joinLeftRel, joinRightRel;

	bool isJoin = false;
	bool isJoinPerformed = false;

	bool isDep = false;
	bool done = false;
	string prev;

	double resAndFactor = 1.0;
	double resOrFactor = 1.0;

	map<string, int> relOpMap;

	while (currentAnd != NULL) {

		currentOr = currentAnd->left;
		resOrFactor = 1.0;

		while (currentOr != NULL) {

			isJoin = false;
			ComparisonOp *curCompOp = currentOr->left;

			if (curCompOp->left->code != NAME) {
				cout << "Not possible" << endl;
				return 0;
			}

			leftAttr = curCompOp->left->value;

#ifdef DEP
			if (strcmp(leftAttr.c_str(), prev.c_str()) == 0)
				isDep = true;

			prev = leftAttr;
#endif //DEP

			for (map<string, map<string, int>>::iterator it = attrMap.begin(); it != attrMap.end(); it++) {
				
				if (attrMap[it->first].count(leftAttr) > 0) {
					leftRel = it->first;
					break;
				}
			}
			
			if (curCompOp->right->code == NAME) {
				isJoin = true;
				isJoinPerformed = true;
				rightAttr = curCompOp->right->value;

				for (map<string, map<string, int>>::iterator it = attrMap.begin(); it != attrMap.end(); it++) {

					if (attrMap[it->first].count(rightAttr) > 0) {
						rightRel = it->first;
						break;
					}
				}				
			}

			if (isJoin == true) {

				double leftCount = attrMap[leftRel][curCompOp->left->value];
				double rightCount = attrMap[rightAttr][curCompOp->right->value];

				if (curCompOp->code == EQUALS) {
					resOrFactor *= (1.0 - (1.0 / max(leftCount, rightCount)));
				}

				joinLeftRel = leftRel;
				joinRightRel = rightRel;
			}
			else {
#ifdef DEP
				if (isDep) {

					if (!done) {
						resOrFactor = 1.0 - resOrFactor;
						done = true;
					}

					if (curCompOp->code == GREATER_THAN || curCompOp->code == LESS_THAN) {
						resOrFactor += (1.0 / 3.0);
						relOpMap[curCompOp->left->value] = curCompOp->code;
					}
					if (curCompOp->code == EQUALS) {
						resOrFactor += (1.0 / (attrMap[leftRel][curCompOp->left->value]));
						relOpMap[curCompOp->left->value] = curCompOp->code;
					}
				}
				else {

					if (curCompOp->code == GREATER_THAN || curCompOp->code == LESS_THAN) {
						resOrFactor *= (2.0 / 3.0);
						relOpMap[curCompOp->left->value] = curCompOp->code;
					}

					if (curCompOp->code == EQUALS) {
						resOrFactor *= (1.0 - (1.0 / attrMap[leftRel][curCompOp->left->value]));
						relOpMap[curCompOp->left->value] = curCompOp->code;
					}
				}
#else
				if (curCompOp->code == GREATER_THAN || curCompOp->code == LESS_THAN) {
					resOrFactor *= (2.0 / 3.0);
					relOpMap[curCompOp->left->value] = curCompOp->code;
				}

				if (curCompOp->code == EQUALS) {
					resOrFactor *= (1.0 - (1.0 / attrMap[leftRel][curCompOp->left->value]));
					relOpMap[curCompOp->left->value] = curCompOp->code;
				}
#endif //DEP
			}

			currentOr = currentOr->rightOr;
		}

#ifdef DEP
		if (!isDep)
			resOrFactor = 1.0 - resOrFactor;
#else
		resOrFactor = 1.0 - resOrFactor;
#endif // DEP

		isDep = false;
		done = false;
		resAndFactor *= resOrFactor;
		currentAnd = currentAnd->rightAnd;
	}

	double rightTupCount = relMap[rightRel];

	if (isJoinPerformed == true) {
		double leftTupCount = relMap[joinLeftRel];
		resultEstimate = leftTupCount * rightTupCount * resAndFactor;
	}
	else {
		double leftTupCount = relMap[leftRel];
		resultEstimate = leftTupCount * resAndFactor;
	}

	for (map<string, int>::iterator it1 = relOpMap.begin(); it1 != relOpMap.end(); it1++) {
		cout << it1->first << "->" << it1->second << endl;
	}

	if (isApply) {

		set<string> joinSet;
		if (isJoinPerformed) {
			
			for (map<string, int>::iterator it1 = relOpMap.begin(); it1 != relOpMap.end(); it1++) {

				for (int i = 0; i < relMap.size(); i++) {

					if(i >= sizeof(relNames) / 2)
						continue;
					int count = attrMap[relNames[i]].count(it1->first);
					
					if (count == 0)
						continue;
					else if (count == 1) {

						for (map<string, int>::iterator it2 = attrMap[relNames[i]].begin(); it2 != attrMap[relNames[i]].end(); it2++) {
							if (it1->second == LESS_THAN || it1->second == GREATER_THAN) {
								attrMap[joinLeftRel + "_" + joinRightRel][it2->first] = (int)round((double)it2->second / 3.0);
							}
							else if (it1->second == EQUALS) {
								if (it1->first == it2->first) {
									attrMap[joinLeftRel + "_" + joinRightRel][it2->first] = 1;
								}
								else {
									attrMap[joinLeftRel + "_" + joinRightRel][it2->first] = min((int)round(resultEstimate), it2->second);
								}
							}							
						}
						break;
					}
					else {
						for (map<string, int>::iterator it2 = attrMap[relNames[i]].begin(); it2 != attrMap[relNames[i]].end(); it2++) {
							if (it1->second == EQUALS) {
								if (it1->first == it2->first) {
									attrMap[joinLeftRel + "_" + joinRightRel][it2->first] = count;
								}
								else
									attrMap[joinLeftRel + "_" + joinRightRel][it2->first] = min((int) round(resultEstimate), it2->second);
							}
						}
						break;
					}
					joinSet.insert(relNames[i]);
				}
			}

			if (joinSet.count(joinLeftRel) == 0) {
				for (map<string, int>::iterator it = attrMap[joinLeftRel].begin(); it != attrMap[joinLeftRel].end(); it++) {
					attrMap[joinLeftRel + "_" + joinRightRel][it->first] = it->second;
				}
			}
			if (joinSet.count(joinRightRel) == 0) {
				for (map<string, int>::iterator it = attrMap[joinRightRel].begin(); it != attrMap[joinRightRel].end(); it++) {
					attrMap[joinLeftRel + "_" + joinRightRel][it->first] = it->second;
				}
			}

			relMap[joinLeftRel + "_" + joinRightRel] = round(resultEstimate);
			relMap.erase(joinLeftRel);
			relMap.erase(joinRightRel);

			attrMap.erase(joinLeftRel);
			attrMap.erase(joinRightRel);
		}			
	}
	return resultEstimate;
}

