#pragma once
#include "AST.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cmath>
using namespace std;

void printAST(ASTNode* node, int depth = 0);
string astToString(ASTNode* node);
double evaluatePostfix(const vector<Token>& postfix);
string formatDouble(double val);