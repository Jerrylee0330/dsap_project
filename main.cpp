#include <iostream>
#include <string>
#include <vector>
#include "AST.hpp"
#include "calculus.hpp"
#include "parser.hpp"
#include "utils.hpp"

using namespace std;

int main() {
    system("chcp 65001 > nul");
    cout << "================================================\n";
    cout << "          C++ Calculus Engine Started!          \n";
    cout << "================================================\n\n";

    string inputExpr;
    cout << "Enter a function of x (e.g., 2 * x): ";
    cout.flush(); // ⚠️ 關鍵修復：強迫程式立刻把上面的字印到螢幕上，不要藏在緩衝區！
    
    getline(cin, inputExpr);

    cout << "\n[Debug] You entered: " << inputExpr << endl;

    // --- 設立檢查點，抓出到底是誰當機 ---
    
    cout << "[Debug] Step 1: Tokenizing (Lexer)..." << endl;
    // ⚠️ 如果你的切字串函數不叫 tokenize，請換成你的函數名稱
    vector<Token> tokens = tokenize(inputExpr); 
    
    cout << "[Debug] Step 2: Infix to Postfix (Shunting Yard)..." << endl;
    vector<Token> postfix = infixToPostfix(tokens);
    
    cout << "[Debug] Step 3: Building AST..." << endl;
    ASTNode* root = buildAST(postfix);

    if (root == nullptr) {
        cout << "\n❌ Parse failed! AST is null." << endl;
        return 1;
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
    ASTNode* intResult = integrate(rootCopy,0);
    intResult = simplify(intResult);
    
    cout << "[Debug] 5.3: Integration done! Entering simplify()..." << endl;
    if (intResult != nullptr) {
        cout << "[Debug] 5.4: Simplification bypassed! Preparing to print..." << endl;
        
        // 為了避免亂碼，我們暫時把 ∫ 改成單純的英文字 Integral
        cout << "[Integral] Integral of f(x) dx = " << treeToString(intResult) << " + C" << endl;
        
    } else {
        cout << "[Integral] [Warning] integrate() returned nullptr" << endl;
    }

    return 0;
}