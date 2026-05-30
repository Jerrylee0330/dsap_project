#pragma once
#include <vector>
#include <stack>
#include <unordered_map>
#include <functional>
#include <string>
#include "AST.hpp"

bool needsImplicitMultiplication(TokenType prevType);
vector<Token> tokenize(const string &input);
int getPrecedence(string op);
vector<Token> infixToPostfix(const vector<Token> &tokens);
ASTNode *buildAST(const vector<Token> &postfix);
string treeToString(ASTNode *node);
string formatDouble(double val);