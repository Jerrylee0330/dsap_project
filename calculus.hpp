#pragma once
#include "AST.hpp"

ASTNode* copyTree(ASTNode* node);
ASTNode* derivative(ASTNode* node);
ASTNode* simplify(ASTNode* node);