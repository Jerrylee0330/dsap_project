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