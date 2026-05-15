#pragma once
#include "AST.hpp"

ASTNode* copyTree(ASTNode* node);
ASTNode* derivative(ASTNode* node);
ASTNode* simplify(ASTNode* node);
ASTNode* integrate(ASTNode* node, int depth);
ASTNode* tableIntegral(ASTNode* node);
ASTNode* linearityIntegral(ASTNode* node);

bool isLinearX(ASTNode* node, double& a);
bool isJustX(ASTNode* node);
bool isSameTree(ASTNode* a, ASTNode* b);