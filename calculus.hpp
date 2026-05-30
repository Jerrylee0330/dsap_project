#pragma once
#include "AST.hpp"

// =======================================================
// 樹狀結構操作與記憶體管理
// =======================================================
ASTNode *copyTree(ASTNode *node);
void deleteTree(ASTNode *node);
bool isSameTree(ASTNode *a, ASTNode *b);

// =======================================================
// 微積分核心引擎與化簡器
// =======================================================
ASTNode *derivative(ASTNode *node);
ASTNode *integrate(ASTNode *node, int depth);
ASTNode *simplify(ASTNode *node);

// =======================================================
// 積分輔助子系統
// =======================================================
ASTNode *tableIntegral(ASTNode *node);
ASTNode *linearityIntegral(ASTNode *node);

// =======================================================
// 分數與格式化處理
// =======================================================
ASTNode *postProcessFractions(ASTNode *node);
ASTNode *doubleToFractionAST(double val);

// =======================================================
// 代數特徵辨識
// =======================================================
bool isLinearX(ASTNode *node, double &a);