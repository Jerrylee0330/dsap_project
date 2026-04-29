#include "parser.hpp"
#include "utils.hpp"

void printAST(ASTNode* node, int depth)
{
    if (node == nullptr) return;

    // 先印右邊的小孩 (畫在畫面的上方)
    printAST(node->right, depth + 1);

    // 根據深度印出空白縮排
    for (int i = 0; i < depth; i++) {
        cout << "    ";
    }
    
    // 印出這個節點的內容
    cout << node->token.value << endl;

    // 再印左邊的小孩 (畫在畫面的下方)
    printAST(node->left, depth + 1);
}

string astToString(ASTNode* node)
{
    if (node == nullptr) return "";

    //如果是數字或變數，直接回傳它的字串
    if (node->token.type == TokenType::Number || node->token.type == TokenType::Variable) 
        return node->token.value;

    //如果是運算子，就把它的小孩抓出來，組合在一起

    string op = node -> token.value;
    int currentPrec = getPrecedence(op);

    string leftStr = astToString(node->left);
    string rightStr = astToString(node->right);

    bool leftNeedsParens = (node->left && node->left->token.type == TokenType::Operator && getPrecedence(node->left->token.value) < currentPrec);
    bool rightNeedsParens = (node->right && node->right->token.type == TokenType::Operator && getPrecedence(node->right->token.value) <= currentPrec);

    if (op == "*") 
    {
        // 如果左邊是數字，且右邊是「變數」、「次方」、或「有括號的東西」，就可以省略乘號！
        if (node->left && node->left->token.type == TokenType::Number) 
        {
            if (node->right && (node->right->token.type == TokenType::Variable || 
                                node->right->token.value == "^" || 
                                rightNeedsParens)) 
            {
                return leftStr + rightStr; // 完美結合：變成 2x, 2x^2, 或 2(x+1)
            }
        }
        // 其他情況還是要印出乘號 (例如 2 * 2x^1 不然會連在一起變成 22x^1)
        return leftStr + " * " + rightStr;
    }

    // 5. 次方符號不加空白，加減乘除加空白比較好看
    if (op == "^") 
    {
        if (node->right && node->right->token.value == "1") 
        {
            return leftStr; 
        }
        
        // 如果不是 1 次方，就乖乖把它用 ^ 組合起來 (例如 x^2)
        return leftStr + "^" + rightStr;
    }

    return leftStr + " " + op + " " + rightStr;
}

double evaluatePostfix(const vector<Token>& postfix)
{
    stack<double> calStack;
    for(int i = 0; i < postfix.size(); i++)
    {
        Token t = postfix[i];
        if(t.type == TokenType::Number) 
        {
            double val = stod(t.value);
            calStack.push(val);
        }//如果是數字就直接丟進去

        else if(t.type == TokenType::Operator)
        {
            double right_val = calStack.top();//先讀右邊的數字
            calStack.pop();
            double left_val = calStack.top();//在讀左邊的數字
            calStack.pop();

            double result = 0;
            if (t.value == "+") result = left_val + right_val;
            else if (t.value == "-") result = left_val - right_val;
            else if (t.value == "*") result = left_val * right_val;
            else if (t.value == "/") result = left_val / right_val;
            else if (t.value == "^") result = pow(left_val,right_val);
            //計算
            
            calStack.push(result); //把新的結果丟回去
        }//如果是運算子就處理左右的數字
    }
    return calStack.top();
}

string formatDouble(double val)
{
    ostringstream out;
    out << val;
    return out.str();
}