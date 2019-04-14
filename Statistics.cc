#include "Statistics.h"
#include <map>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <iostream>

Statistics::Statistics()
{
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

	relMap[oldRel] = relMap[newRel];
	
	map<string, int> oldAttrMap = attrMap[oldRel];

	for (map<string, int>::iterator it = oldAttrMap.begin(); it != oldAttrMap.end(); ++it) {

		attrMap[newRel][it->first] = it->second;
	}
}
	
void Statistics::Read(char *fromWhere)
{
	ifstream inFile;

	inFile.open(fromWhere);

	int relCountInRel, relCountInAttr, attrCount;

	if (inFile) {
		inFile >> relCountInRel;
		string line;
		vector<string> token;
		for (int i = 0; i < relCountInRel; i++)
		{
			inFile >> line;		
			stringstream input(line);
			string temp;
			while (getline(input, temp, '.')) {
				token.push_back(temp);
			}
			relMap.insert(pair<string, int>(token[0], stoi(token[1])));
			token.clear();
		}
		inFile >> relCountInAttr;
		string attrLine;
		for (int i = 0; i < relCountInAttr; i++) 
		{
			inFile >> line;
			inFile >> attrCount;
			for (int j = 0; j < attrCount; j++) {
				inFile >> attrLine;
				stringstream input(attrLine);
				string temp;
				while (getline(input, temp, '.')) {
					token.push_back(temp);
				}
				attrMap[line][token[0]] = stoi(token[1]);
				token.clear();
			}
		}
	}

	for (map<string, int>::iterator it = relMap.begin(); it != relMap.end(); it++)
		cout << it->first << "->" << it->second << endl;
	for (map<string, map<string, int>>::iterator it = attrMap.begin(); it != attrMap.end(); it++) {
		cout << it->first << endl;
		for (map<string, int>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
			cout << it2->first << "->" << it2->second << endl;
	}
}
void Statistics::Write(char *fromWhere)
{
	remove(fromWhere);

	ofstream outFile;
	outFile.open(fromWhere);

	outFile << relMap.size() << "\n";

	for (map<string,int>::iterator it = relMap.begin(); it != relMap.end(); it++)
	{	
		outFile << it->first << "." << it->second << "\n";
	}

	outFile << attrMap.size() << "\n";

	for (map<string, map<string, int>>::iterator it1 = attrMap.begin(); it1 != attrMap.end(); it1++)
	{
		outFile << it1->first << "\n";
		map<string, int> attr = it1->second;

		outFile << attr.size() << "\n";

		for (map<string, int>::iterator it2 = attr.begin(); it2 != attr.end(); it2++)
		{
			outFile << it2->first << "." << it2->second << "\n";
		}
	}

	outFile.close();
}

void  Statistics::Apply(struct AndList *parseTree, char *relNames[], int numToJoin)
{
}
double Statistics::Estimate(struct AndList *parseTree, char **relNames, int numToJoin)
{
}

