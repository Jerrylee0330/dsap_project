#include "parser.hpp"

bool needsImplicitMultiplication(TokenType prevType)
{
    return (prevType == TokenType::Number || 
            prevType == TokenType::Variable || 
            prevType == TokenType::RightParen);
}

vector<Token> tokenize(const string& input)
{
    vector<Token> tokens;

    for(int i = 0; i < input.length(); i++)
    {
        char c = input[i];
        if(c == ' ') continue;
        if(c == '+') tokens.push_back({TokenType::Operator, "+"});
        if(c == '-') tokens.push_back({TokenType::Operator, "-"});
        if(c == '*') tokens.push_back({TokenType::Operator, "*"});
        if(c == '/') tokens.push_back({TokenType::Operator, "/"});
        if(c == '^') tokens.push_back({TokenType::Operator, "^"});
        if(c == '(') tokens.push_back({TokenType::LeftParen, "("});
        if(c == ')') tokens.push_back({TokenType::RightParen, ")"});
        
        if(isalpha(c))
        {
            string name = "";
            
            // 1. 貪婪讀取：把連續的英文字母全部吃進來組成單字
            while (i < input.length() && isalpha(input[i])) {
                name += input[i];
                i++;
            }
            i--; // 退回一格，抵銷外層 for 迴圈即將執行的 i++

            if (!tokens.empty() && needsImplicitMultiplication(tokens.back().type)) {
                tokens.push_back({TokenType::Operator, "*", MathFunc::None});
            }

            // 3. 建立靜態hash table (只會在程式啟動時初始化一次，極度節省資源)
            static const std::unordered_map<string, MathFunc> funcMap = {
                {"sin", MathFunc::sin},
                {"cos", MathFunc::cos},
                {"tan", MathFunc::tan},
                {"cot", MathFunc::cot},
                {"sec", MathFunc::sec},
                {"csc", MathFunc::csc},
                {"ln",  MathFunc::ln},
            };

            static const std::unordered_map<string, double> constMap = {
                {"pi", 3.14159265358979323846},
                {"e",  2.71828182845904523536}
            };

            // 4. 字典查表 (O(1) 極速尋找)
            auto itFunc = funcMap.find(name);
            if (itFunc != funcMap.end()) {
                // 命中函數字典！(例如 sin)
                tokens.push_back({TokenType::Function, name, itFunc->second});
            } 
            else {
                // 如果不是函數，再查查看是不是常數？
                auto itConst = constMap.find(name);
                if (itConst != constMap.end()) {
                    // 命中常數字典！(例如 pi) 👉 直接把它偽裝成 Number 存進去
                    tokens.push_back({TokenType::Number, to_string(itConst->second), MathFunc::None});
                } 
                else {
                    // 都不是！那它就是個普通的變數 (例如 x, y)
                    tokens.push_back({TokenType::Variable, name, MathFunc::None});
                }
            }
        }
        

        if(isdigit(c))
        {
            string numStr = "";
            while(i < input.length() && (isdigit(input[i]) || input[i] == '.')) 
            {
                numStr += input[i];
                i++;
            }

            tokens.push_back({TokenType::Number, numStr});
            i--; 
        }//數字以遍歷形式處理，用while讀到該字元不是數字/小數點為止
    }
    
    return tokens;
}

int getPrecedence(string op)
{
    if(op == "+" || op == "-") return 1;
    if(op == "*" || op == "/") return 2;
    if(op == "^") return 3;
    else return 0;
}

vector<Token> infixToPostfix(const vector<Token>& tokens)
{
    vector<Token> output; //這個是預計要回傳的後序表達式
    stack<Token> opStack;
    for(int i = 0; i < tokens.size(); i++)
    {
        Token t = tokens[i];
        
        // 1. 數字和變數直接輸出
        if (t.type == TokenType::Number || t.type == TokenType::Variable) {
            output.push_back(t);
        }

        //  2. 遇到函數，跟左括號一樣先推入 Stack 等待
        else if (t.type == TokenType::Function) {
            opStack.push(t);
        }

        // 3. 處理運算子
        else if (t.type == TokenType::Operator) {
            while (!opStack.empty())
            {
                Token topOp = opStack.top();
                if (topOp.type == TokenType::Operator && getPrecedence(t.value) <= getPrecedence(topOp.value)) 
                {
                    output.push_back(topOp);
                    opStack.pop();          
                } 
                else break;
            }
            opStack.push(t);
        }

        // 4. 左括號
        else if (t.type == TokenType::LeftParen) 
        {
            opStack.push(t);
        }

        // 5. 右括號
        else if (t.type == TokenType::RightParen) 
        {
            while (!opStack.empty() && opStack.top().type != TokenType::LeftParen) {
                output.push_back(opStack.top());
                opStack.pop();
            }

            if (!opStack.empty() && opStack.top().type == TokenType::LeftParen) {
                opStack.pop(); 
            }
            //  關鍵新增：括號處理完後，如果前面包著函數 (例如 sin)，也把它彈到 output
            if (!opStack.empty() && opStack.top().type == TokenType::Function) {
                output.push_back(opStack.top());
                opStack.pop();
            }
        }
    }
    while (!opStack.empty()) 
    {
        output.push_back(opStack.top());
        opStack.pop();
    }
    return output;
}

ASTNode* buildAST(const vector<Token>& postfix)
{
    stack<ASTNode*> st;

    for (int i = 0; i < postfix.size(); i++) 
    {
        Token t = postfix[i];

        if (t.type == TokenType::Number || t.type == TokenType::Variable) 
        {
            ASTNode* new_AST= new ASTNode(t);
            st.push(new_AST);
        } 

        else if (t.type == TokenType::Function)
        {
            ASTNode* funcNode = new ASTNode(t);
            
            // 函數只有一個小孩 (例如 sin 裡面的 x)，把它掛在右邊
            ASTNode* child = st.top();
            st.pop();
            
            funcNode->right = child;
            funcNode->left = nullptr; // 左邊留空

            st.push(funcNode);
        }

        else if (t.type == TokenType::Operator) 
        {
            ASTNode* op_Node = new ASTNode(t);

            ASTNode* rightchild = st.top();
            st.pop();

            ASTNode* leftchild = st.top();
            st.pop();

            op_Node->left = leftchild;
            op_Node->right = rightchild;

            st.push(op_Node);
        }
    }
    
    // 迴圈結束後，Stack 剩下的唯一一個元素就是整棵樹的 Root
    return st.top(); 
}