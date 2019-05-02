#include "Statistics.h"
#include <climits>

#include <stdio.h>
#include <cstring>
#include <iostream>
#include <stdlib.h> 
#include <fstream>
Statistics::Statistics() {}

Statistics::Statistics(Statistics &copyMe) {
	for (map<string, attrMap>::iterator it1 = copyMe.relMap.begin(); it1 != copyMe.relMap.end(); ++it1) {
		string str1 = it1->first;
		attrMap attMap;
		attMap.numTuples = it1->second.numTuples;
		attMap.numRel = it1->second.numRel;

		for (map<string, int>::iterator it2 = it1->second.attrs.begin(); it2 != it1->second.attrs.end(); ++it2) {
			string str2 = it2->first;
			int n = it2->second;
			attMap.attrs.insert(pair<string, int>(str2, n));
		}
		relMap.insert(pair<string, attrMap>(str1, attMap));
		attMap.attrs.clear();
	}
}

Statistics::~Statistics() {}

void Statistics::AddRel(char *relName, int numTuples) {
	attrMap newRel;
	newRel.numTuples = numTuples;
	newRel.numRel = 1;
	string str(relName);
	relMap.insert(pair<string, attrMap>(str, newRel));
}

void Statistics::AddAtt(char *relName, char *attName, int numDistincts) {
	string rel(relName);
	string att(attName);
	map<string, attrMap>::iterator it = relMap.find(rel);
	if (numDistincts == -1)
		numDistincts = it->second.numTuples;
	it->second.attrs.insert(pair<string, int>(att, numDistincts));
}

void Statistics::CopyRel(char *oldName, char *newName) {
	string old(oldName);
	string newname(newName);
	map<string, attrMap>::iterator it = relMap.find(old);
	attrMap newRel;
	newRel.numTuples = it->second.numTuples;
	newRel.numRel = it->second.numRel;
	for (map<string, int>::iterator it2 = it->second.attrs.begin(); it2 != it->second.attrs.end(); ++it2) {
		char * newatt = new char[200];
		sprintf(newatt, "%s.%s", newName, it2->first.c_str());
		string temp(newatt);
		newRel.attrs.insert(pair<string, int>(newatt, it2->second));
	}
	relMap.insert(pair<string, attrMap>(newname, newRel));
}

void Statistics::Read(char *fromWhere) {
	//FILE* inFile;
	string file(fromWhere);
	ifstream inFile;
	
	inFile.open(file.c_str(), ios::in);
	
	string line;
	char relchar[200];
	int n;
	inFile >> line;
	while (strcmp(line.c_str(), "end")) {
		if (!strcmp(line.c_str(), "rel")) {
			attrMap attMap;
			attMap.numRel = 1;
			inFile >> line;
			string relstr(line);
			strcpy(relchar, relstr.c_str());
			inFile >> line;
			attMap.numTuples = atoi(line.c_str());
			inFile >> line;
			inFile >> line;

			while (strcmp(line.c_str(), "rel") && strcmp(line.c_str(), "end")) {
				string attstr(line);
				inFile >> line;
				n = atoi(line.c_str());
				attMap.attrs.insert(pair<string, int>(attstr, n));
				inFile >> line;
			}
			relMap.insert(pair<string, attrMap>(relstr, attMap));
		}
	}
	inFile.close();
}

void Statistics::Write(char *fromWhere) {
	//FILE* outFile;
	string file(fromWhere);
	ofstream outFile;
	outFile.open(file.c_str(), ios::out);
	for (map<string, attrMap>::iterator it1 = relMap.begin(); it1 != relMap.end(); ++it1) {
		char * write = new char[it1->first.length() + 1];
		strcpy(write, it1->first.c_str());
		/*fprintf(outFile, "rel\n%s\n", write);*/
		outFile << "rel\n" << write << "\n";
		/*fprintf(outFile, "%d\nattrs\n", it1->second.numTuples);*/
		outFile << "\nattrs\n" << it1->second.numTuples << "\n";
		for (map<string, int>::iterator it2 = it1->second.attrs.begin(); it2 != it1->second.attrs.end(); ++it2) {
			char * att = new char[it2->first.length() + 1];
			strcpy(att, it2->first.c_str());
			//fprintf(outFile, "%s\n%d\n", att, it2->second);
			outFile << "\nattrs\n" << it2->second << "\n";
		}
	}
	
	outFile << "end\n";
	outFile.close();
}

void  Statistics::Apply(struct AndList *parseTree, char *relNames[], int numToJoin) {
	struct AndList * currentAnd = parseTree;
	struct OrList * currentOr;
	while (currentAnd != NULL) {
		if (currentAnd->left != NULL) {
			currentOr = currentAnd->left;
			while (currentOr != NULL) {
				if (currentOr->left->left->code == 3 && currentOr->left->right->code == 3) {//
					map<string, int>::iterator itAtt[2];
					map<string, attrMap>::iterator itRel[2];

					string joinAtt1(currentOr->left->left->value);
					string joinAtt2(currentOr->left->right->value);
					for (map<string, attrMap>::iterator it2 = relMap.begin(); it2 != relMap.end(); ++it2) {
						itAtt[0] = it2->second.attrs.find(joinAtt1);
						if (itAtt[0] != it2->second.attrs.end()) {
							itRel[0] = it2;
							break;
						}
					}
					for (map<string, attrMap>::iterator it2 = relMap.begin(); it2 != relMap.end(); ++it2) {
						itAtt[1] = it2->second.attrs.find(joinAtt2);
						if (itAtt[1] != it2->second.attrs.end()) {
							itRel[1] = it2;
							break;
						}
					}
					attrMap joinedRel;
					char * joinName = new char[200];
					sprintf(joinName, "%s|%s", itRel[0]->first.c_str(), itRel[1]->first.c_str());
					string joinNamestr(joinName);
					joinedRel.numTuples = tempRes;
					joinedRel.numRel = numToJoin;
					for (int i = 0; i < 2; i++) {
						for (map<string, int>::iterator it2 = itRel[i]->second.attrs.begin(); it2 != itRel[i]->second.attrs.end(); ++it2) {
							joinedRel.attrs.insert(*it2);
						}
						relMap.erase(itRel[i]);
					}
					relMap.insert(pair<string, attrMap>(joinNamestr, joinedRel));
				}
				else {
					string seleAtt(currentOr->left->left->value);
					map<string, int>::iterator itAtt;
					map<string, attrMap>::iterator itRel;
					for (map<string, attrMap>::iterator it2 = relMap.begin(); it2 != relMap.end(); ++it2) {
						itAtt = it2->second.attrs.find(seleAtt);
						if (itAtt != it2->second.attrs.end()) {
							itRel = it2;
							break;
						}
					}
					itRel->second.numTuples = tempRes;

				}
				currentOr = currentOr->rightOr;
			}
		}
		currentAnd = currentAnd->rightAnd;
	}
}

double Statistics::Estimate(struct AndList *parseTree, char **relNames, int numToJoin) {
	struct AndList * currentAnd = parseTree;
	struct OrList * currentOr;
	double result = 0.0, fraction = 1.0;
	int state = 0;
	if (currentAnd == NULL) {
		if (numToJoin > 1) return -1;
		
		
		return relMap[relNames[0]].numTuples;
	}
	while (currentAnd != NULL) {
		if (currentAnd->left != NULL) {
			currentOr = currentAnd->left;
			double fractionOr = 0.0;
			map<string, int>::iterator lastAtt;
			while (currentOr != NULL) {
				if (currentOr->left->left->code == 3 && currentOr->left->right->code == 3) {
					map<string, int>::iterator itAtt[2];
					map<string, attrMap>::iterator itRel[2];
					string joinAtt1(currentOr->left->left->value);
					string joinAtt2(currentOr->left->right->value);

					for (map<string, attrMap>::iterator it2 = relMap.begin(); it2 != relMap.end(); ++it2) {
						itAtt[0] = it2->second.attrs.find(joinAtt1);
						if (itAtt[0] != it2->second.attrs.end()) {
							itRel[0] = it2;
							break;
						}
					}
					for (map<string, attrMap>::iterator it2 = relMap.begin(); it2 != relMap.end(); ++it2) {
						itAtt[1] = it2->second.attrs.find(joinAtt2);
						if (itAtt[1] != it2->second.attrs.end()) {
							itRel[1] = it2;
							break;
						}
					}

					double max;
					if (itAtt[0]->second >= itAtt[1]->second)		max = (double)itAtt[0]->second;
					else		max = (double)itAtt[1]->second;
					if (state == 0)
						result = (double)itRel[0]->second.numTuples*(double)itRel[1]->second.numTuples / max;
					else
						result = result * (double)itRel[1]->second.numTuples / max;

					//cout << "max " << max << endl;
					//cout << "join result: " << result << endl;
					state = 1;
				}
				else {
					string seleAtt(currentOr->left->left->value);
					map<string, int>::iterator itAtt;
					map<string, attrMap>::iterator itRel;
					for (map<string, attrMap>::iterator it2 = relMap.begin(); it2 != relMap.end(); ++it2) {
						itAtt = it2->second.attrs.find(seleAtt);
						if (itAtt != it2->second.attrs.end()) {
							itRel = it2;
							break;
						}
					}
					if (result == 0.0)
						result = ((double)itRel->second.numTuples);
					double tempFrac;
					if (currentOr->left->code == 7)
						tempFrac = 1.0 / itAtt->second;
					else
						tempFrac = 1.0 / 3.0;
					if (lastAtt != itAtt)
						fractionOr = tempFrac + fractionOr - (tempFrac*fractionOr);
					else
						fractionOr += tempFrac;
					//cout << "fracOr: " << fractionOr << endl;
					lastAtt = itAtt;//
				}
				currentOr = currentOr->rightOr;
			}
			if (fractionOr != 0.0)
				fraction = fraction * fractionOr;
			//cout << "frac: " << fraction << endl;
		}
		currentAnd = currentAnd->rightAnd;
	}
	result = result * fraction;
	//cout << "result " << result << endl;
	tempRes = result;
	return result;
}

