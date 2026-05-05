#pragma once
#include <string>
using namespace std;

// 定義 Token 的種類
enum class TokenType {
    Number,     // 數字 (例如 12, 3.14)
    Variable,   // 變數 (例如 x)
    Operator,   // 運算子 (例如 +, -, *, /)
    LeftParen,  // 左括號 (
    RightParen,  // 右括號 )
    Function    // 函數
};

enum class MathFunc {
    None,
    sin,
    cos,
    tan,
    cot,
    sec,
    csc,
    log,
    ln,
    arcsin,
    arccos,
    arctan,
    arccot,
    arcsec,
    arccsc,
    abs
};

// 儲存 Token 的結構
struct Token {
    TokenType type;
    string value; 
    MathFunc funcType = MathFunc::None; 
};

//定義 AST(抽象語法樹) 的結構
struct ASTNode {
    Token token;            
    ASTNode* left;          
    ASTNode* right;         

    ASTNode(Token t) {
        token = t;
        left = nullptr;
        right = nullptr;
    }
};