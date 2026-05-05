#include "AST.hpp"
#include "parser.hpp"
#include "calculus.hpp"
#include "utils.hpp"
#include <cstdlib>
using namespace std;

int main() 
{
    system("chcp 65001 > nul");

    string input;
    getline(cin,input);
    vector<Token> tokens = tokenize(input);
    vector<Token> postfix = infixToPostfix(tokens);

    for(int i = 0; i < postfix.size(); i++) {
        cout << postfix[i].value << " ";
    }
    cout << endl;

    ASTNode* root = buildAST(postfix);
    printAST(root);

    ASTNode* diffRoot = derivative(root);
    ASTNode* simplifiedRoot = simplify(diffRoot);
    
    cout << "\n=== Differentiated Tree f'(x) ===" << endl;
    printAST(simplifiedRoot);

    string resultStr = treeToString(simplifiedRoot);
    cout << "\n=== Final Result ===" << endl;
    cout << resultStr << endl;

    return 0;
}