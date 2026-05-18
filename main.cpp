#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include "AST.hpp"
#include "parser.hpp"
#include "calculus.hpp" 
#include "utils.hpp"

using namespace std;

// =======================================================
// 🌟 防護罩：把黏在一起的函數和變數拆開 (例如 "sinx" -> "sin x")
// =======================================================
string formatInputString(string s) {
    vector<string> funcs = {
        "arcsin", "arccos", "arctan", "arccot", "arcsec", "arccsc",
        "sin", "cos", "tan", "cot", "sec", "csc", "ln", "log"
    };

    for (const string& f : funcs) {
        size_t pos = 0;
        while ((pos = s.find(f, pos)) != string::npos) {
            size_t nextIdx = pos + f.length();
            // 如果函數名稱後面緊接著字母 (例如 'x')，就強制在中間塞一個空白！
            if (nextIdx < s.length() && isalpha(s[nextIdx])) {
                s.insert(nextIdx, " ");
            }
            pos += f.length(); 
        }
    }
    return s;
}

// =======================================================
// 主程式
// =======================================================
int main() {
    system("chcp 65001 > nul");
    cout << "================================================\n";
    cout << "          C++ Calculus Engine Started!          \n";
    cout << "================================================\n\n";

    string inputExpr;
    
    // 加上迴圈，讓計算機可以一直算下去
    while (true) {
        cout << "Enter a function of x (or type 'exit' to quit): ";
        cout.flush(); 
        
        getline(cin, inputExpr);
        
        if (inputExpr == "exit") break;
        if (inputExpr.empty()) continue; // 防呆：如果只按了 Enter 就跳過

        cout << "\n[Debug] You entered: " << inputExpr << endl;

        // --- 設立檢查點，追蹤引擎執行進度 ---
        
        cout << "[Debug] Step 0: Formatting String (e.g. sinx -> sin x)..." << endl;
        inputExpr = formatInputString(inputExpr);
        cout << "[Debug] -> Formatted to: " << inputExpr << endl;

        cout << "[Debug] Step 1: Tokenizing (Lexer)..." << endl;
        vector<Token> raw_tokens = tokenize(inputExpr); 

        cout << "[Debug] Step 1.5: Preprocessing Tokens (Adding implicit '*')..." << endl;
        vector<Token> processed_tokens = preprocessTokens(raw_tokens);
        
        cout << "[Debug] Step 2: Infix to Postfix (Shunting Yard)..." << endl;
        vector<Token> postfix = infixToPostfix(processed_tokens);
        
        cout << "[Debug] Step 3: Building AST..." << endl;
        ASTNode* root = buildAST(postfix);

        if (root == nullptr) {
            cout << "\n❌ Parse failed! AST is null. Please check your syntax." << endl;
            cout << "------------------------------------------------\n\n";
            continue; // AST 建立失敗，跳回迴圈開頭重新輸入
        }

        cout << "\n------------------------------------------------\n";
        cout << "[Original] f(x) = " << treeToString(root) << endl;

        cout << "[Debug] Step 4: Derivative..." << endl;
        ASTNode* diffResult = derivative(root);
        if (diffResult) {
            diffResult = simplify(diffResult); 
            cout << "[Derivative] f'(x) = " << treeToString(diffResult) << endl;
        }

        cout << "[Debug] Step 5: Integral..." << endl;
        
        cout << "[Debug] 5.1: Copying AST tree..." << endl;
        ASTNode* rootCopy = copyTree(root); 

        cout << "[Debug] 5.2: Entering integrate() engine..." << endl;
        ASTNode* intResult = integrate(rootCopy, 0);
        
        cout << "[Debug] 5.3: Integration done! Entering simplify()..." << endl;
        intResult = simplify(intResult);
        
        if (intResult != nullptr) {
            cout << "[Debug] 5.4: Simplification done! Preparing to print..." << endl;
            cout << "[Integral] Integral of f(x) dx = " << treeToString(intResult) << " + C" << endl;
        } else {
            cout << "[Integral] [Warning] integrate() returned nullptr" << endl;
        }

        cout << "[Debug] Step 6: Cleaning up memory..." << endl;
        deleteTree(root);       // 刪除原本的輸入樹
        deleteTree(diffResult); // 刪除微分解答樹
        deleteTree(intResult);  // 刪除積分解答樹
        
        cout << "================================================\n\n";
    }

    cout << "Calculus Engine successfully terminated. Goodbye!" << endl;
    return 0;
}