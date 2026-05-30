#include "parser.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>
bool needsImplicitMultiplication(TokenType prevType)
{
    return (prevType == TokenType::Number ||
            prevType == TokenType::Variable ||
            prevType == TokenType::Constant ||
            prevType == TokenType::RightParen);
}

// =======================================================
// 🌟 Token 預處理器 (完美適配 TokenType::LeftParen 版)
// =======================================================

vector<Token> tokenize(const string &input)
{
    vector<Token> tokens;

    for (int i = 0; i < input.length(); i++)
    {
        char c = input[i];
        if (c == ' ')
            continue;
        if (c == '+')
            tokens.push_back({TokenType::Operator, "+"});
        if (c == '-')
            tokens.push_back({TokenType::Operator, "-"});
        if (c == '*')
            tokens.push_back({TokenType::Operator, "*"});
        if (c == '/')
            tokens.push_back({TokenType::Operator, "/"});
        if (c == '^')
            tokens.push_back({TokenType::Operator, "^"});
        if (c == '(')
            tokens.push_back({TokenType::LeftParen, "("});
        if (c == ')')
            tokens.push_back({TokenType::RightParen, ")"});

        if (isalpha(c))
        {
            string name = "";

            // 1. 貪婪讀取：把連續的英文字母全部吃進來組成單字
            while (i < input.length() && isalpha(input[i]))
            {
                name += input[i];
                i++;
            }
            i--; // 退回一格，抵銷外層 for 迴圈即將執行的 i++

            if (!tokens.empty() && needsImplicitMultiplication(tokens.back().type))
            {
                tokens.push_back({TokenType::Operator, "*", MathFunc::None});
            }

            // 3. 建立靜態hash table
            static const std::unordered_map<string, MathFunc> funcMap = {
                {"sin", MathFunc::sin},
                {"cos", MathFunc::cos},
                {"tan", MathFunc::tan},
                {"cot", MathFunc::cot},
                {"sec", MathFunc::sec},
                {"csc", MathFunc::csc},
                {"ln", MathFunc::ln},
                {"log", MathFunc::log},
                {"arcsin", MathFunc::arcsin},
                {"arccos", MathFunc::arccos},
                {"arctan", MathFunc::arctan},
                {"arccot", MathFunc::arccot},
                {"arcsec", MathFunc::arcsec},
                {"arccsc", MathFunc::arccsc},
                {"abs", MathFunc::abs},
            };

            // 雖然我們暫時不用到 double，但保留這個 map 以後要做 evaluate() 數值計算時非常方便！
            static const std::unordered_map<string, double> constMap = {
                {"pi", 3.14159265358979323846},
                {"e", 2.71828182845904523536}};

            // 4. 字典查表 (O(1) 極速尋找)
            auto itFunc = funcMap.find(name);
            if (itFunc != funcMap.end())
            {
                // 命中函數字典！(例如 sin)
                tokens.push_back({TokenType::Function, name, itFunc->second});
            }
            else
            {
                // 如果不是函數，再查查看是不是常數？
                auto itConst = constMap.find(name);
                if (itConst != constMap.end())
                {
                    // ==========================================
                    // 🌟 核心修正點：命中常數字典！
                    // 不要再把它轉換成小數了，直接賦予 Constant 身分，並保留原本的字母 (name)！
                    // ==========================================
                    tokens.push_back({TokenType::Constant, name, MathFunc::None});
                }
                else
                {
                    // 都不是！那它就是個普通的變數 (例如 x, y)
                    tokens.push_back({TokenType::Variable, name, MathFunc::None});
                }
            }
        }

        if (isdigit(c))
        {
            string numStr = "";
            while (i < input.length() && (isdigit(input[i]) || input[i] == '.'))
            {
                numStr += input[i];
                i++;
            }

            tokens.push_back({TokenType::Number, numStr});
            i--;
        }
    }

    return tokens;
}

int getPrecedence(string op)
{
    if (op == "+" || op == "-")
        return 1;
    if (op == "*" || op == "/")
        return 2;
    if (op == "^")
        return 3;
    else
        return 100;
}

vector<Token> infixToPostfix(const vector<Token> &tokens)
{
    vector<Token> output; // 這個是預計要回傳的後序表達式
    stack<Token> opStack;
    for (int i = 0; i < tokens.size(); i++)
    {
        Token t = tokens[i];

        // 1. 數字和變數直接輸出
        if (t.type == TokenType::Number || t.type == TokenType::Variable || t.type == TokenType::Constant)
        {
            output.push_back(t);
        }

        //  2. 遇到函數，跟左括號一樣先推入 Stack 等待
        else if (t.type == TokenType::Function)
        {
            opStack.push(t);
        }

        // 3. 處理運算子
        else if (t.type == TokenType::Operator)
        {
            while (!opStack.empty())
            {
                Token topOp = opStack.top();
                if (topOp.type == TokenType::Operator && getPrecedence(t.value) <= getPrecedence(topOp.value))
                {
                    output.push_back(topOp);
                    opStack.pop();
                }
                else
                    break;
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
            while (!opStack.empty() && opStack.top().type != TokenType::LeftParen)
            {
                output.push_back(opStack.top());
                opStack.pop();
            }

            if (!opStack.empty() && opStack.top().type == TokenType::LeftParen)
            {
                opStack.pop();
            }
            //  關鍵新增：括號處理完後，如果前面包著函數 (例如 sin)，也把它彈到 output
            if (!opStack.empty() && opStack.top().type == TokenType::Function)
            {
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

ASTNode *buildAST(const vector<Token> &postfix)
{
    stack<ASTNode *> st;

    for (int i = 0; i < postfix.size(); i++)
    {
        Token t = postfix[i];

        if (t.type == TokenType::Number || t.type == TokenType::Variable || t.type == TokenType::Constant)
        {
            ASTNode *new_AST = new ASTNode(t);
            st.push(new_AST);
        }

        else if (t.type == TokenType::Function)
        {
            ASTNode *funcNode = new ASTNode(t);

            // 函數只有一個小孩 (例如 sin 裡面的 x)，把它掛在右邊
            ASTNode *child = st.top();
            st.pop();

            funcNode->right = child;
            funcNode->left = nullptr; // 左邊留空

            st.push(funcNode);
        }

        else if (t.type == TokenType::Operator)
        {
            ASTNode *op_Node = new ASTNode(t);

            ASTNode *rightchild = st.top();
            st.pop();

            ASTNode *leftchild = st.top();
            st.pop();

            op_Node->left = leftchild;
            op_Node->right = rightchild;

            st.push(op_Node);
        }
    }

    // 迴圈結束後，Stack 剩下的唯一一個元素就是整棵樹的 Root
    return st.top();
}

std::string doubleToFraction(double val, double tol = 1e-5)
{
    if (std::abs(val) < tol)
        return "0";

    // 如果本來就是整數，直接回傳
    if (std::abs(val - std::round(val)) < tol)
    {
        return std::to_string((int)std::round(val));
    }

    int sign = (val < 0) ? -1 : 1;
    val = std::abs(val);

    // 暴力尋找分母 (最大分母設為 10000，微積分的題目絕對夠用)
    for (int d = 1; d <= 10000; ++d)
    {
        double num = val * d;
        // 如果乘上分母後非常接近整數，代表我們找到分數了！
        if (std::abs(num - std::round(num)) < tol)
        {
            int n = (int)std::round(num);
            return (sign < 0 ? "-" : "") + std::to_string(n) + "/" + std::to_string(d);
        }
    }

    // 🛡️ 兜底機制：如果真的找不到漂亮的分數 (例如無理數)，就印出乾淨的小數
    std::string res = std::to_string(val * sign);
    res.erase(res.find_last_not_of('0') + 1, std::string::npos); // 砍掉尾巴多餘的 0
    if (res.back() == '.')
        res.pop_back(); // 如果砍完 0 剩下小數點，也砍掉
    return res;
}

string treeToString(ASTNode *node)
{
    if (node == nullptr)
        return "";

    // ==========================================
    // 情況 A：處理變數與常數
    // ==========================================
    if (node->token.type == TokenType::Variable || node->token.type == TokenType::Constant)
    {
        return node->token.value;
    }

    // 處理數字 (小數轉分數)
    if (node->token.type == TokenType::Number)
    {
        return doubleToFraction(std::stod(node->token.value));
    }

    // ==========================================
    // 情況 B：函數處理
    // ==========================================
    if (node->token.type == TokenType::Function)
    {
        if (node->token.value == "abs")
            return "|" + treeToString(node->right) + "|";
        return node->token.value + "(" + treeToString(node->right) + ")";
    }

    // ==========================================
    // 情況 C：運算子處理
    // ==========================================
    if (node->token.type == TokenType::Operator)
    {
        string op = node->token.value;

        // 🌟 根號攔截邏輯：把 A^0.5 轉換成比較好看的 sqrt(A)
        if (op == "^" && node->right && node->right->token.type == TokenType::Number)
        {
            double p = std::stod(node->right->token.value);
            if (std::abs(p - 0.5) < 1e-9)
            {
                return "sqrt(" + treeToString(node->left) + ")";
            }
        }

        // 🌟 乘法優化邏輯 (總司令的改進版)
        if (op == "*")
        {
            double coeff = 1.0;
            vector<string> vars;
            vector<string> funcs;

            std::function<void(ASTNode *)> collectFactors = [&](ASTNode *n)
            {
                if (!n)
                    return;
                if (n->token.type == TokenType::Number)
                {
                    coeff *= stod(n->token.value);
                    return;
                }
                if (n->token.type == TokenType::Variable)
                {
                    vars.push_back(n->token.value);
                    return;
                }
                if (n->token.type == TokenType::Operator && n->token.value == "*")
                {
                    collectFactors(n->left);
                    collectFactors(n->right);
                    return;
                }
                string termStr = treeToString(n);
                // 遇到加減除法被包在乘法裡，強制加括號
                if (n->token.type == TokenType::Operator && (n->token.value == "+" || n->token.value == "-" || n->token.value == "/"))
                {
                    termStr = "(" + termStr + ")";
                }
                funcs.push_back(termStr);
            };

            collectFactors(node);
            if (abs(coeff) < 1e-9)
                return "0";

            string result = "";
            bool hasOtherTerms = !vars.empty() || !funcs.empty();

            // 處理常數係數
            if (abs(coeff - 1.0) < 1e-9 && hasOtherTerms)
            { /* 係數為 1 且有其他東西，省略不印 */
            }
            else if (abs(coeff + 1.0) < 1e-9 && hasOtherTerms)
            {
                result += "-";
            }
            else
            {
                result += doubleToFraction(coeff);
                if (hasOtherTerms)
                    result += " * ";
            }

            // 處理變數與函數
            for (const string &v : vars)
                result += v;
            for (size_t i = 0; i < funcs.size(); ++i)
            {
                if (i > 0 || !vars.empty())
                    result += " * ";
                result += funcs[i];
            }
            return result;
        }

        // 🌟 其他運算子 ( +, -, /, ^ ) 的括號優先級邏輯
        string leftStr = treeToString(node->left);
        string rightStr = treeToString(node->right);

        // 處理單目運算符 (例如最前面的負號 -x)
        if (node->left == nullptr)
        {
            if (node->right && node->right->token.type == TokenType::Operator)
            {
                return op + "(" + rightStr + ")";
            }
            return op + rightStr;
        }

        // 優先級判斷小幫手：數字越大優先級越高
        auto getPrec = [](const string &o)
        {
            if (o == "+" || o == "-")
                return 1;
            if (o == "/" || o == "*")
                return 2;
            if (o == "^")
                return 3;
            return 4;
        };

        int myPrec = getPrec(op);

        // 左邊需不需要括號？(如果左邊的運算子優先級「小於」自己，就要包起來)
        // 例如： (A + B) ^ C  ->  ^ 是 3，+ 是 1，所以左邊要包
        bool wrapLeft = false;
        if (node->left && node->left->token.type == TokenType::Operator)
        {
            if (getPrec(node->left->token.value) < myPrec)
                wrapLeft = true;
        }

        // 右邊需不需要括號？(如果右邊優先級「小於等於」自己，通常要包起來確保安全)
        // 例如： A - (B + C)  或  A / (B * C)  或  A ^ (B ^ C)
        bool wrapRight = false;
        if (node->right && node->right->token.type == TokenType::Operator)
        {
            if (getPrec(node->right->token.value) <= myPrec)
                wrapRight = true;
        }

        if (wrapLeft)
            leftStr = "(" + leftStr + ")";
        if (wrapRight)
            rightStr = "(" + rightStr + ")";

        return leftStr + " " + op + " " + rightStr;
    }

    return "";
}

string formatDouble(double val)
{
    std::ostringstream out;
    out << std::setprecision(15) << std::noshowpoint << val;
    return out.str();
}