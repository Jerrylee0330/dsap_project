#include "calculus.hpp"
#include "utils.hpp"
#include "parser.hpp"
#include <map>
#include <cmath>

// 🌟 幫手 1：多項式雷達 (精準抓出係數與次方)
// 可以看懂 5, x, 3*x, x^2, 4*x^3, 甚至 2*x*3*x^2 這種怪物！
bool parseTerm(ASTNode* node, double& coeff, double& power) {
    if (!node) return false;

    // 1. 純數字 (例如 5 -> 係數 5, 次方 0)
    if (node->token.type == TokenType::Number) {
        coeff = std::stod(node->token.value);
        power = 0.0;
        return true;
    }
    // 2. 純變數 (例如 x -> 係數 1, 次方 1)
    if (node->token.type == TokenType::Variable && node->token.value == "x") {
        coeff = 1.0;
        power = 1.0;
        return true;
    }
    // 3. 次方項 (例如 x^3 -> 係數 1, 次方 3)
    if (node->token.type == TokenType::Operator && node->token.value == "^") {
        if (node->left && node->left->token.value == "x" &&
            node->right && node->right->token.type == TokenType::Number) {
            coeff = 1.0;
            power = std::stod(node->right->token.value);
            return true;
        }
    }
    // 4. 乘法組合 (例如 3*x, 4*x^2, 或是展開產生的一坨 x*x)
    if (node->token.type == TokenType::Operator && node->token.value == "*") {
        double cL = 1, pL = 0, cR = 1, pR = 0;
        // 遞迴解析左右兩邊
        bool okL = parseTerm(node->left, cL, pL);
        bool okR = parseTerm(node->right, cR, pR);

        if (okL && okR) {
            coeff = cL * cR;  // 數字相乘
            power = pL + pR;  // 次方相加 (指數律: x^a * x^b = x^(a+b))
            return true;
        }
    }
    return false; // 看不懂的複雜結構 (例如 sin(x))
}

// 🌟 幫手 2：標準化組裝工廠
// 負責把 coeff 和 power 變成最乾淨的樹狀結構
ASTNode* buildTerm(double coeff, double power) {
    // 係數為 0，整坨直接歸零
    if (coeff == 0.0) return new ASTNode({TokenType::Number, "0", MathFunc::None});
    
    // 次方為 0，變成純常數
    if (power == 0.0) return new ASTNode({TokenType::Number, formatDouble(coeff), MathFunc::None});

    // 處理變數部分 (x 或 x^n)
    ASTNode* varNode = nullptr;
    if (power == 1.0) {
        varNode = new ASTNode({TokenType::Variable, "x", MathFunc::None});
    } else {
        varNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        varNode->left = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        varNode->right = new ASTNode({TokenType::Number, formatDouble(power), MathFunc::None});
    }

    // 如果係數是 1，直接回傳變數 (不需要 1 * x)
    if (coeff == 1.0) return varNode;
    
    // 如果係數是 -1，組裝成 -1 * x (後續印出可以優化成 -x)
    if (coeff == -1.0) {
        ASTNode* negNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        negNode->left = new ASTNode({TokenType::Number, "-1", MathFunc::None});
        negNode->right = varNode;
        return negNode;
    }

    // 一般情況：係數 * 變數 (例如 3 * x^2)
    ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
    mulNode->left = new ASTNode({TokenType::Number, formatDouble(coeff), MathFunc::None});
    mulNode->right = varNode;
    return mulNode;
}

// 輔助函數：判斷一個節點是不是「同類項」 (例如 x, 3*x, 或 x*5)
// 如果是，就把變數名稱存入 varName，係數存入 coeff
bool isLikeTerm(ASTNode* n, string& varName, double& coeff) {
    if (n == nullptr) return false;

    // 情況 1: 單純只有變數 (例如 "x") -> 係數為 1
    if (n->token.type == TokenType::Variable) {
        varName = n->token.value;
        coeff = 1.0;
        return true;
    }

    // 情況 2: 常數 * 變數 (例如 "3 * x")
    if (n->token.type == TokenType::Operator && n->token.value == "*") {
        if (n->left && n->left->token.type == TokenType::Number && 
            n->right && n->right->token.type == TokenType::Variable) {
            coeff = stod(n->left->token.value);
            varName = n->right->token.value;
            return true;
        }
        // 反過來： 變數 * 常數 (例如 "x * 3")
        if (n->right && n->right->token.type == TokenType::Number && 
            n->left && n->left->token.type == TokenType::Variable) {
            coeff = stod(n->right->token.value);
            varName = n->left->token.value;
            return true;
        }
    }
    return false;
}

bool containsVariable(ASTNode* node, const string& varName) 
{
    if (node == nullptr) return false;

    // 1. 如果是變數節點，檢查名稱是否匹配
    if (node->token.type == TokenType::Variable) {
        return node->token.value == varName;
    }

    // 2. 遞迴搜尋左右子樹
    return containsVariable(node->left, varName) || containsVariable(node->right, varName);
}

bool isConstant(ASTNode* node) {
    if (node == nullptr) return true;
    
    // 如果抓到變數 x，就絕對不是常數
    if (node->token.type == TokenType::Variable && node->token.value == "x") {
        return false;
    }
    
    // 遞迴檢查左右兩邊，必須兩邊都是常數才算常數
    return isConstant(node->left) && isConstant(node->right);
}

bool hasVariableX(ASTNode* node) {
    // 1. 走到盡頭了，沒找到
    if (node == nullptr) return false; 

    // 2. 檢查自己 (中)：我是不是變數 x？
    if (node->token.type == TokenType::Variable && node->token.value == "x") {
        return true; 
    }

    // 3. 自己不是，那就去問左邊的小孩 (左) 和右邊的小孩 (右)
    // 只要有一邊回報 true，整棵樹就代表有 x！
    return hasVariableX(node->left) || hasVariableX(node->right);
}

bool isSameTree(ASTNode* a, ASTNode* b) {
    if (!a && !b) return true;
    
    if (!a || !b) return false;
    
    if (a->token.type != b->token.type) return false;

    if (a->token.type == TokenType::Number) {
        if (std::stod(a->token.value) != std::stod(b->token.value)) {
            return false;
        }
    } 
    else {
        if (a->token.value != b->token.value) return false;
    }
    return isSameTree(a->left, b->left) && isSameTree(a->right, b->right);
}

ASTNode* copyTree(ASTNode* node)
{
    if (node == nullptr) return nullptr;
    ASTNode* newNode = new ASTNode(node->token);
    newNode->left = copyTree(node->left);
    newNode->right = copyTree(node->right);
    return newNode;
}

void deleteTree(ASTNode* node) {
    if (node == nullptr) return;

    deleteTree(node->left);
    deleteTree(node->right);
    delete node; 
}

ASTNode* derivative(ASTNode* node)
{
    if (node == nullptr) return nullptr;

    //處理常數微分    
    if (node->token.type == TokenType::Number) 
    {
        return new ASTNode({TokenType::Number, "0"});
    }

    
    if (node->token.type == TokenType::Constant) 
    {
        return new ASTNode({TokenType::Number, "0", MathFunc::None});
    }

    //處理變數微分
    if (node->token.type == TokenType::Variable) 
    {
        if (node->token.value == "x") 
        {
            return new ASTNode({TokenType::Number, "1"});
        } //如果是x變數，微分後為1
        else 
        {
            return new ASTNode({TokenType::Number, "0"});
        } //如果是其他變數，微分後為 0
    }

    if (node->token.type == TokenType::Operator) 
    {
        string op = node->token.value;

        if(op == "^")
        {
            // 處理多項式次方微分: (u^n)' = n * u^(n-1) * u'
            if (node->right && node->right->token.type == TokenType::Number) 
            {
                ASTNode* powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None}); 
                ASTNode* multNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* chainMulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None}); 

                string n_str = node->right->token.value;
                double n_num = stod(n_str);
                double n_num_minus_one = n_num - 1;
                n_str = formatDouble(n_num);
                string n_minus_1_str = formatDouble(n_num_minus_one);

                ASTNode* nNode = new ASTNode({TokenType::Number, n_str, MathFunc::None});
                ASTNode* newPowerNode = new ASTNode({TokenType::Number, n_minus_1_str, MathFunc::None});

                powNode->left = copyTree(node->left);
                powNode->right = newPowerNode;

                multNode->left = nNode;
                multNode->right = powNode;
                chainMulNode->left = multNode; // 左邊放剛剛算好的 n * u^(n-1)
                chainMulNode->right = derivative(node->left); // 右邊放底數的微分 (u')

                return chainMulNode; 
            }

            else if (node->left && node->left->token.type == TokenType::Number) 
            {
                ASTNode* mulNode_1 = new ASTNode({TokenType::Operator, "*"});
                ASTNode* mulNode_2 = new ASTNode({TokenType::Operator, "*"});
                ASTNode* lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln}); 
                
                mulNode_1 -> left = mulNode_2;
                mulNode_1 -> right = derivative(node -> right);
                mulNode_2 -> left = lnNode;
                mulNode_2 -> right = copyTree(node);
                lnNode -> right = copyTree(node -> left);

                return mulNode_1;
            }//a^u' = ln(a)*a^u*u'

            else
            {
                ASTNode* mulNode_1 = new ASTNode({TokenType::Operator, "*"});
                ASTNode* mulNode_2 = new ASTNode({TokenType::Operator, "*"});
                ASTNode* mulNode_3 = new ASTNode({TokenType::Operator, "*"});
                ASTNode* addNode = new ASTNode({TokenType::Operator, "+"});
                ASTNode* powNode = new ASTNode({TokenType::Operator, "^"});
                ASTNode* divNode = new ASTNode({TokenType::Operator, "/"});
                ASTNode* lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});

                mulNode_1 -> left = powNode;
                mulNode_1 -> right = addNode;
                powNode -> left = copyTree(node -> left);
                powNode -> right = copyTree(node -> right);
                addNode -> left = mulNode_2;
                addNode -> right = mulNode_3;
                mulNode_2 -> left = derivative(node -> right);
                mulNode_2 -> right = lnNode;
                lnNode -> right = copyTree(node -> left);
                mulNode_3 -> left = copyTree(node -> right);
                mulNode_3 -> right = divNode;
                divNode -> left = derivative(node -> left);
                divNode -> right = copyTree(node -> left);

                return mulNode_1;
            }
            
        }

        // 【加法法則】 (f + g)' = f' + g'
        if (op == "+") 
        {
            ASTNode* leftDiff = derivative(node->left);
            ASTNode* rightDiff = derivative(node->right);

            ASTNode* result = new ASTNode({TokenType::Operator, "+"});
            result -> left = leftDiff;
            result -> right = rightDiff;
            return result;
        }

        if(op == "-")
        {
            ASTNode* leftDiff = derivative(node->left);
            ASTNode* rightDiff = derivative(node->right);

            ASTNode* result = new ASTNode({TokenType::Operator, "-"});
            result -> left = leftDiff;
            result -> right = rightDiff;
            return result;
        }

        if(op == "*")
        {
            ASTNode* leftDiff = derivative(node->left);
            ASTNode* rightDiff = derivative(node->right);

            ASTNode* mulNode_left = new ASTNode({TokenType::Operator, "*"});
            ASTNode* mulNode_right = new ASTNode({TokenType::Operator, "*"});

            mulNode_left -> left = leftDiff;
            mulNode_left -> right = copyTree(node -> right);
            mulNode_right ->left = copyTree(node -> left);
            mulNode_right -> right = rightDiff;

            ASTNode* result = new ASTNode({TokenType::Operator, "+"});
            result -> left = mulNode_left;
            result -> right = mulNode_right;

            return result;
        }

        if(op == "/")
        {
            ASTNode* leftDiff = derivative(node->left);
            ASTNode* rightDiff = derivative(node->right);

            ASTNode* mulNode_left = new ASTNode({TokenType::Operator, "*"});
            ASTNode* mulNode_right = new ASTNode({TokenType::Operator, "*"});

            mulNode_left -> left = leftDiff;
            mulNode_left -> right = copyTree(node -> right);
            mulNode_right ->left = copyTree(node -> left);
            mulNode_right -> right = rightDiff;

            ASTNode* numerator = new ASTNode({TokenType::Operator, "-"});
            numerator -> left = mulNode_left;
            numerator -> right = mulNode_right;

            ASTNode* denominator = new ASTNode({TokenType::Operator, "^"});
            denominator -> left = copyTree(node -> right);
            denominator -> right = new ASTNode({TokenType::Number, "2"});

            ASTNode* result = new ASTNode({TokenType::Operator, "/"});
            result -> left = numerator;
            result -> right = denominator;

            return result;
        }
    }

    if(node->token.type == TokenType::Function)
    {
        ASTNode* innerDiff = derivative(node->right);

        ASTNode* outerDiff = nullptr;

        switch (node->token.funcType) {
            case MathFunc::sin: 
            {
                // sin(u)' = cos(u)
                outerDiff = new ASTNode({TokenType::Function, "cos", MathFunc::cos});
                outerDiff -> right = copyTree(node -> right); // 把原本的 u 複製過來
                break;
            }
            case MathFunc::cos: {
                // cos(u)' = -1 * sin(u)
                ASTNode* minusOne = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                ASTNode* sinNode = new ASTNode({TokenType::Function, "sin", MathFunc::sin});
                sinNode->right = copyTree(node -> right);
                
                outerDiff = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                outerDiff -> left = minusOne;
                outerDiff -> right = sinNode;
                break;
            }
            case MathFunc::tan: 
            {
                // tan(u)' = sec^2(u)
                ASTNode* secNode = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                secNode -> right = copyTree(node -> right);
                ASTNode* two = new ASTNode({TokenType::Number, "2", MathFunc::None});

                outerDiff = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                outerDiff -> left = secNode;
                outerDiff -> right = two;

                break;
            }
            case MathFunc::cot: 
            {
                // cot(u)' = -csc^2(u)
                ASTNode* minusOne = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                ASTNode* cscNode = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                cscNode -> right = copyTree(node -> right);
                ASTNode* two = new ASTNode({TokenType::Number, "2", MathFunc::None});

                outerDiff = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                powNode -> left = cscNode;
                powNode -> right = two;

                outerDiff -> left = minusOne;
                outerDiff -> right = powNode;

                break;
            }
            case MathFunc::sec: 
            {
                // sec(u)' = sec(u) * tan(u)
                ASTNode* secNode = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                secNode -> right = copyTree(node -> right);
                ASTNode* tanNode = new ASTNode({TokenType::Function, "tan", MathFunc::tan});
                tanNode -> right = copyTree(node -> right);

                outerDiff = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                outerDiff -> left = secNode;
                outerDiff -> right = tanNode;

                break;
            }
            case MathFunc::csc: 
            {
                // csc(u)' = -csc(u)*cot(u)
                ASTNode* cscNode = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                cscNode -> right = copyTree(node -> right);
                ASTNode* cotNode = new ASTNode({TokenType::Function, "cot", MathFunc::cot});
                cotNode -> right = copyTree(node -> right);
                ASTNode* minusOne = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                mulNode -> left = cscNode;
                mulNode -> right = cotNode;

                outerDiff = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                outerDiff -> left = minusOne;
                outerDiff -> right = mulNode;

                break;
            }
            case MathFunc::ln: {
                // ln(u)' = 1 / u
                ASTNode* one = new ASTNode({TokenType::Number, "1", MathFunc::None});
                outerDiff = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                outerDiff -> left = one;
                outerDiff -> right = copyTree(node -> right);
                break;
            }
           case MathFunc::log: {
                // log_10(u) 的外部微分 outerDiff = 1 / (u * ln(10))
                ASTNode* divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                ASTNode* one = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* mulDenom = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* ten = new ASTNode({TokenType::Number, "10", MathFunc::None});
                ASTNode* ln10Node = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                
                ln10Node -> right = ten;
                mulDenom -> left = copyTree(node -> right); // 這裡的 u 只需要 copy，不用微分
                mulDenom -> right = ln10Node;
                
                divNode -> left = one;
                divNode -> right = mulDenom;

                outerDiff = divNode;
                break;
            }
            case MathFunc::arcsin: {
                // arcsin(u) 的外部微分 outerDiff = (1 - u^2)^-0.5
                ASTNode* powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                ASTNode* one = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* two = new ASTNode({TokenType::Number, "2", MathFunc::None});
                ASTNode* neg_half_point_five = new ASTNode({TokenType::Number, "-0.5", MathFunc::None});
                
                powNode_1 -> left = subNode;
                powNode_1 -> right = neg_half_point_five;
                subNode -> left = one;
                subNode -> right = powNode_2;
                powNode_2 -> left = copyTree(node -> right);
                powNode_2 -> right = two;

                outerDiff = powNode_1;
                break;
            }
            case MathFunc::arccos: {
                // arccos(u) 的外部微分 outerDiff = -1 * (1 - u^2)^-0.5 (已修正數學邏輯)
                ASTNode* mulOuter = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* minus_one = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                ASTNode* powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                ASTNode* one = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* two = new ASTNode({TokenType::Number, "2", MathFunc::None});
                ASTNode* neg_half_point_five = new ASTNode({TokenType::Number, "-0.5", MathFunc::None});
                
                powNode_1 -> left = subNode;
                powNode_1 -> right = neg_half_point_five;
                subNode -> left = one;
                subNode -> right = powNode_2;
                powNode_2 -> left = copyTree(node -> right);
                powNode_2 -> right = two;

                mulOuter -> left = minus_one;
                mulOuter -> right = powNode_1;

                outerDiff = mulOuter;
                break;
            }
            case MathFunc::arctan: {
                // arctan(u) 的外部微分 outerDiff = 1 / (1 + u^2)
                ASTNode* powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                ASTNode* addNode = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                ASTNode* one_1 = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* one_2 = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* two = new ASTNode({TokenType::Number, "2", MathFunc::None});

                divNode -> left = one_1;
                divNode -> right = addNode;
                addNode -> left = one_2;
                addNode -> right = powNode;
                powNode -> left = copyTree(node -> right);
                powNode -> right = two;

                outerDiff = divNode;
                break;
            }
            case MathFunc::arccot: {
                // arccot(u) 的外部微分 outerDiff = -1 / (1 + u^2)
                ASTNode* powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                ASTNode* addNode = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                ASTNode* minus_one = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                ASTNode* one = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* two = new ASTNode({TokenType::Number, "2", MathFunc::None});

                divNode -> left = minus_one;
                divNode -> right = addNode;
                addNode -> left = one;
                addNode -> right = powNode;
                powNode -> left = copyTree(node -> right);
                powNode -> right = two;

                outerDiff = divNode;
                break;
            }
            case MathFunc::arcsec: {
                // arcsec(u) 的外部微分 outerDiff = 1 / (|u| * (u^2 - 1)^0.5)
                ASTNode* mulNode_2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                ASTNode* subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                ASTNode* one_1 = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* one_2 = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* two = new ASTNode({TokenType::Number, "2", MathFunc::None});
                ASTNode* point_five = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
                ASTNode* absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});

                divNode -> left = one_1;
                divNode -> right = mulNode_2;
                mulNode_2 -> left = absNode;
                mulNode_2 -> right = powNode_1;
                absNode -> right = copyTree(node -> right);
                powNode_1 -> left = subNode;
                powNode_1 -> right = point_five;
                subNode -> left = powNode_2;
                subNode -> right = one_2;
                powNode_2 -> left = copyTree(node -> right);
                powNode_2 -> right = two;

                outerDiff = divNode;
                break;
            }
            case MathFunc::arccsc: {
                // arccsc(u) 的外部微分 outerDiff = -1 / (|u| * (u^2 - 1)^0.5)
                ASTNode* mulNode_2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                ASTNode* subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                ASTNode* minus_one = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                ASTNode* one = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* two = new ASTNode({TokenType::Number, "2", MathFunc::None});
                ASTNode* point_five = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
                ASTNode* absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});

                divNode -> left = minus_one;
                divNode -> right = mulNode_2;
                mulNode_2 -> left = absNode;
                mulNode_2 -> right = powNode_1;
                absNode -> right = copyTree(node->right);
                powNode_1 -> left = subNode;
                powNode_1 -> right = point_five;
                subNode -> left = powNode_2;
                subNode -> right = one;
                powNode_2 -> left = copyTree(node->right);
                powNode_2 -> right = two;

                outerDiff = divNode;
                break;
            }
            default:
                break; // 如果未來有其他函數可以繼續擴充
        }


        // 3. 把它們乘起來： f'(g(x)) * g'(x)
        ASTNode* result = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        result->left = outerDiff;
        result->right = innerDiff;

        return result;
    }

    return nullptr;

}

bool hasPlusMinus(ASTNode* node) {
    if (node == nullptr) return false;
    if (node->token.type == TokenType::Operator && 
       (node->token.value == "+" || node->token.value == "-")) {
        return true;
    }
    return hasPlusMinus(node->left) || hasPlusMinus(node->right);
}

// =======================================================
// 🌟 輔助工具 1：因子攤平收集器 (Factor Collector)
// 負責把連續相乘的樹枝全部打平，變成一個清單
// 例如：((x^2+1)^5 * 2) * x  會被攤平成 -> [(x^2+1)^5, 2, x]
// =======================================================
void collectFactors(ASTNode* node, std::vector<ASTNode*>& factors) {
    if (!node) return;
    
    // 如果遇到乘法節點，就繼續往左右兩邊往下挖
    if (node->token.type == TokenType::Operator && node->token.value == "*") {
        collectFactors(node->left, factors);
        collectFactors(node->right, factors);
    } else {
        // 如果已經不是乘法了（例如遇到 (x^2+1)^5 或 2 或 x），就把它存進清單裡
        factors.push_back(node);
    }
}

// =======================================================
// 🌟 輔助工具 2：因子重新組裝機 (Product Rebuilder)
// 負責把挑剩的零件，重新用乘號組合回一棵標準的 AST 樹
// 例如：清單剩 [2, x] -> 會被組裝成 (2 * x) 的樹結構
// =======================================================
ASTNode* rebuildProduct(const std::vector<ASTNode*>& factors) {
    if (factors.empty()) return nullptr;
    
    // 拿第一個零件當作基底
    ASTNode* root = copyTree(factors[0]);
    
    // 把剩下的零件，一個一個用乘號 "*" 疊加接上去
    for (size_t i = 1; i < factors.size(); ++i) {
        ASTNode* newRoot = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        newRoot->left = root;
        newRoot->right = copyTree(factors[i]);
        root = newRoot; // 更新頂端節點
    }
    return root;
}

ASTNode* expand(ASTNode* node) {
    if (node == nullptr) return nullptr;

    // 1. 先遞迴展開左子樹與右子樹 (Bottom-up)
    ASTNode* leftExp = expand(node->left);
    ASTNode* rightExp = expand(node->right);

    // 建立當前節點的更新版本
    ASTNode* current = new ASTNode(node->token);
    current->left = leftExp;
    current->right = rightExp;

    // ==========================================
    // 🌟 擴充情況 C：處理多項式括號高次方展開 (U^n)
    // ==========================================
    if (current->token.type == TokenType::Operator && current->token.value == "^") {
        if (current->right && current->right->token.type == TokenType::Number) {
            double n = std::stod(current->right->token.value);
            
            // 關鍵防禦：只有當底數包含加減法，且指數是正整數時才展開
            if (n > 0 && floor(n) == n && hasPlusMinus(current->left)) {
                if (n == 1.0) {
                    // (U)^1 -> 直接變成 U
                    ASTNode* keep = current->left;
                    current->left = nullptr; // 斷開連結，防止被連帶 deleteTree 釋放
                    deleteTree(current);     // 清除當前的 ^ 節點與數字 1 節點
                    return keep;
                } else {
                    // (U)^n -> U * (U)^(n-1)
                    ASTNode* U = current->left;
                    
                    // 建立右半邊子樹: (U)^(n-1)
                    ASTNode* newPower = new ASTNode({TokenType::Operator, "^", MathFunc::None}); //
                    newPower->left = copyTree(U); //
                    newPower->right = new ASTNode({TokenType::Number, formatDouble(n - 1.0), MathFunc::None}); //

                    // 建立新的乘法根節點
                    ASTNode* newRoot = new ASTNode({TokenType::Operator, "*", MathFunc::None}); //
                    newRoot->left = copyTree(U); //
                    newRoot->right = newPower;   //

                    deleteTree(current);     // 銷毀原本未展開的舊樹，避免 Memory Leak
                    return expand(newRoot);  // 🌟 丟回 expand！強迫它觸發下面的分配律爆破！
                }
            }
        }
    }

    // 2. 尋找分配律的特徵：當前節點是乘法 "*"
    if (current->token.type == TokenType::Operator && current->token.value == "*") {
        
        // ==========================================
        // 情況 A：左邊是 (A ± B)，右邊是 C  -> (A ± B) * C
        // ==========================================
        if (current->left && current->left->token.type == TokenType::Operator && 
           (current->left->token.value == "+" || current->left->token.value == "-")) 
        {
            ASTNode* A = current->left->left;
            ASTNode* B = current->left->right;
            ASTNode* C = current->right;
            string op = current->left->token.value; // "+" 或 "-"

            // 建立新的根節點 (+ 或 -)
            ASTNode* newRoot = new ASTNode({TokenType::Operator, op, MathFunc::None});
            
            // 建立左半邊: A * C
            ASTNode* mul1 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            mul1->left = copyTree(A);
            mul1->right = copyTree(C); // C 被用了第一次

            // 建立右半邊: B * C
            ASTNode* mul2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            mul2->left = copyTree(B);
            mul2->right = copyTree(C); // C 被用了第二次，必須 copy！

            newRoot->left = mul1;
            newRoot->right = mul2;
            
            // 💡 關鍵遞迴：因為 (A*C) + (B*C) 裡面可能還藏著括號，所以要再丟進 expand 檢查一次
            return expand(newRoot); 
        }

        // ==========================================
        // 情況 B：左邊是 A，右邊是 (B ± C)  -> A * (B ± C)
        // ==========================================
        else if (current->right && current->right->token.type == TokenType::Operator && 
                (current->right->token.value == "+" || current->right->token.value == "-")) 
        {
            ASTNode* A = current->left;
            ASTNode* B = current->right->left;
            ASTNode* C = current->right->right;
            string op = current->right->token.value;

            ASTNode* newRoot = new ASTNode({TokenType::Operator, op, MathFunc::None});
            
            // 左半邊: A * B
            ASTNode* mul1 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            mul1->left = copyTree(A); // A 被用了第一次
            mul1->right = copyTree(B);

            // 右半邊: A * C
            ASTNode* mul2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            mul2->left = copyTree(A); // A 被用了第二次，必須 copy！
            mul2->right = copyTree(C);

            newRoot->left = mul1;
            newRoot->right = mul2;
            return expand(newRoot);
        }
    }

    // 如果不需要展開，就回傳更新後的節點
    return current;
}

ASTNode* integrate(ASTNode* node, int depth = 0);

ASTNode* linearityIntegral(ASTNode* node, int depth) {
    // 防禦機制：線性法則只處理運算子 (+, -, *)
    if (node == nullptr || node->token.type != TokenType::Operator) {
        return nullptr;
    }

    // 規則 1：處理加法 (+) 與減法 (-)  [這裡的邏輯完全不變]
    if (node->token.value == "+" || node->token.value == "-") {
        
        ASTNode* leftIntegral = integrate(node->left, depth);        
        ASTNode* rightIntegral = integrate(node->right, depth); 

        if (leftIntegral != nullptr && rightIntegral != nullptr) {
            ASTNode* addSubNode = new ASTNode({TokenType::Operator, node->token.value, MathFunc::None});
            addSubNode->left = leftIntegral;
            addSubNode->right = rightIntegral;
            return addSubNode;
        }
        return nullptr;
    }

    // 規則 2：處理常數乘法 (*)
    if (node->token.value == "*") {
        // 直接判定：左邊或右邊是不是一個純數字？
        bool leftIsNumber = (node->left && node->left->token.type == TokenType::Number);
        bool rightIsNumber = (node->right && node->right->token.type == TokenType::Number);

        // 情況 A：左邊是數字 (例如 3 * x^2)
        if (leftIsNumber && !rightIsNumber) {
            ASTNode* rightIntegral = integrate(node->right,depth); // 只對右邊積分
            
            if (rightIntegral != nullptr) {
                ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                mulNode->left = copyTree(node->left); // 完美複製數字
                mulNode->right = rightIntegral;
                return mulNode;
            }
        }
        // 情況 B：右邊是數字 (例如 x^2 * 3)
        else if (!leftIsNumber && rightIsNumber) {
            ASTNode* leftIntegral = integrate(node->left,depth); // 只對左邊積分
            
            if (leftIntegral != nullptr) {
                ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                mulNode->left = leftIntegral;
                mulNode->right = copyTree(node->right); // 完美複製數字
                return mulNode;
            }
        }
    }

    // 若不是 +, -, *，或是積分失敗，回傳 nullptr 交給其他法則處理
    return nullptr;
}

bool isJustX(ASTNode* node) {
    return node != nullptr && 
           node->token.type == TokenType::Variable && 
           node->token.value == "x";
}

// 輔助函數：判斷是否為 a*x 或單純的 x，並把常數 a 抓出來
bool isLinearX(ASTNode* node, double& a) {
    if (node == nullptr) return false;

    // 情況 1：單純的 x (相當於 1 * x)
    if (node->token.type == TokenType::Variable && node->token.value == "x") {
        a = 1.0;
        return true;
    }

    // 情況 2：遇到乘法 a * x 或 x * a
    if (node->token.type == TokenType::Operator && node->token.value == "*") {
        bool leftIsNum = (node->left && node->left->token.type == TokenType::Number);
        bool rightIsX = (node->right && node->right->token.type == TokenType::Variable && node->right->token.value == "x");
        
        bool rightIsNum = (node->right && node->right->token.type == TokenType::Number);
        bool leftIsX = (node->left && node->left->token.type == TokenType::Variable && node->left->token.value == "x");

        // 如果是 a * x
        if (leftIsNum && rightIsX) {
            a = stod(node->left->token.value);
            return true;
        }
        // 如果是 x * a
        if (leftIsX && rightIsNum) {
            a = stod(node->right->token.value);
            return true;
        }
    }

    return false; // 不是線性結構 (例如 x^2 或是 sin(x))
}

ASTNode* tableIntegral(ASTNode* node) 
{
    if(!node) return nullptr;

    if (node->token.type == TokenType::Constant) {
    // ∫ e dx = e * x
    ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
    mulNode->left = copyTree(node); // 複製那個 "e"
    mulNode->right = new ASTNode({TokenType::Variable, "x", MathFunc::None});
    return mulNode;
    }

    if (node->token.type == TokenType::Variable && node->token.value == "x") 
    {
        ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode* half = new ASTNode({TokenType::Number, "0.5", MathFunc::None}); // 或寫成 "1/2" 如果你的簡化器支援
        ASTNode* powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        ASTNode* xNode = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        ASTNode* two = new ASTNode({TokenType::Number, "2", MathFunc::None});

        powNode->left = xNode;
        powNode->right = two;
        mulNode->left = half;
        mulNode->right = powNode;
        
        return mulNode;
    }

    if (node->token.type == TokenType::Operator && node->token.value == "^")
    {
        if (node->left->token.type == TokenType::Constant && node->left->token.value == "e" &&
        node->right->token.type == TokenType::Variable && node->right->token.value == "x") {
            return copyTree(node); 
        }

        if(node -> left && node -> left -> token.type == TokenType::Variable && node -> right && node -> right->token.type == TokenType::Number)
        {
            if(node -> right -> token.value == "-1")
            {
                ASTNode* lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                lnNode -> right = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
                lnNode -> right -> right = copyTree(node -> left);
                return lnNode;
            }// ∫1/x dx = ln|x|

            else
            {
                ASTNode* mulNode = new ASTNode({TokenType::Operator, "*"});
                ASTNode* divNode = new ASTNode({TokenType::Operator, "/"});
                ASTNode* powNode = new ASTNode({TokenType::Operator, "^"});
                ASTNode* oneNode = new ASTNode({TokenType::Number, "1"});

                string n_str = node -> right -> token.value;
                double n_num = stod(n_str);
                double n_num_plus_one = n_num + 1;
                n_str = formatDouble(n_num);
                string n_plus_1_str = formatDouble(n_num_plus_one);

                ASTNode* N_plus_one_Node_1 = new ASTNode({TokenType::Number, n_plus_1_str});
                ASTNode* N_plus_one_Node_2 = new ASTNode({TokenType::Number, n_plus_1_str});

                mulNode -> left = divNode;
                mulNode -> right = powNode;
                divNode -> left = oneNode;
                divNode -> right = N_plus_one_Node_1;
                powNode -> left = copyTree(node -> left);
                powNode -> right = N_plus_one_Node_2;

                return mulNode;
            }// ∫x^n dx = (1/(n+1)) * x^(n+1) (n != -1)
        }

        // 處理運算子 ^ 的情況
    if (node->token.type == TokenType::Operator && node->token.value == "^") {
        
        bool leftIsNum = (node->left && node->left->token.type == TokenType::Number);
        
        if (leftIsNum) {
            bool rightIsX = (node->right && node->right->token.type == TokenType::Variable && node->right->token.value == "x");
            bool rightIsLinear = false;
            double b_val = 1.0;
            
            if (node->right && node->right->token.type == TokenType::Operator && node->right->token.value == "*") {
                ASTNode* rLeft = node->right->left;
                ASTNode* rRight = node->right->right;
                
                if (rLeft && rLeft->token.type == TokenType::Number && 
                    rRight && rRight->token.type == TokenType::Variable && rRight->token.value == "x") {
                    rightIsLinear = true;
                    b_val = stod(rLeft->token.value); 
                }
                else if (rRight && rRight->token.type == TokenType::Number && 
                         rLeft && rLeft->token.type == TokenType::Variable && rLeft->token.value == "x") {
                    rightIsLinear = true;
                    b_val = stod(rRight->token.value); 
                }
            }

            if (rightIsX || rightIsLinear) {
                // 建構核心公式: a^(bx) / ln(a)
                ASTNode* divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                ASTNode* lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                
                divNode->left = copyTree(node);       
                lnNode->right = copyTree(node->left); 
                divNode->right = lnNode;              

                if (rightIsLinear && b_val != 1.0 && b_val != 0.0) {
                    double reciprocal = 1.0 / b_val;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None}); 
                    
                    outerMul->left = coefNode;
                    outerMul->right = divNode;
                    return outerMul;
                }

                return divNode;
            }
        }
    }
    }

    if (node->token.type == TokenType::Function) {
        
        double a = 1.0; // 準備用來裝常數倍率
        
        if (!isLinearX(node->right, a)) {
            return nullptr;
        }

        switch (node->token.funcType) {
            case MathFunc::sin: {
                // int sin(x) dx = -1 * cos(x)
                ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* minusOne = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                ASTNode* cosNode = new ASTNode({TokenType::Function, "cos", MathFunc::cos});
                
                cosNode->right = copyTree(node->right);
                mulNode->left = minusOne;
                mulNode->right = cosNode;

                if (a != 1.0) {
                    double reciprocal = 1.0 / a;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                    
                    outerMul->left = coefNode;
                    outerMul->right = mulNode;
                    return outerMul;
                }

                return mulNode;
            }
            case MathFunc::cos: {
                // int cos(x) dx = sin(x)
                ASTNode* sinNode = new ASTNode({TokenType::Function, "sin", MathFunc::sin});
                sinNode->right = copyTree(node->right);
                if (a != 1.0) {
                    double reciprocal = 1.0 / a;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                    
                    outerMul->left = coefNode;
                    outerMul->right = sinNode;
                    return outerMul;
                }
                return sinNode;
            }
            case MathFunc::tan: {
                ASTNode* lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                ASTNode* absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
                ASTNode* secNode = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                lnNode -> right = absNode;
                absNode -> right = secNode;
                secNode -> right = copyTree(node -> right);
                if (a != 1.0) {
                    double reciprocal = 1.0 / a;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                    
                    outerMul->left = coefNode;
                    outerMul->right = lnNode;
                    return outerMul;
                }
                return lnNode;
            }
            case MathFunc::cot: {
                ASTNode* lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                ASTNode* absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
                ASTNode* sinNode = new ASTNode({TokenType::Function, "sin", MathFunc::sin});
                lnNode -> right = absNode;
                absNode -> right = sinNode;
                sinNode -> right = copyTree(node -> right);
                if (a != 1.0) {
                    double reciprocal = 1.0 / a;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                    
                    outerMul->left = coefNode;
                    outerMul->right = lnNode;
                    return outerMul;
                }
                return lnNode;
            }
            case MathFunc::sec: {
                ASTNode* lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                ASTNode* absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
                ASTNode* secNode = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                ASTNode* tanNode = new ASTNode({TokenType::Function, "tan", MathFunc::tan});
                ASTNode* plusNode = new ASTNode({TokenType::Operator, "+"});

                lnNode -> right = absNode;
                absNode -> right = plusNode;
                plusNode -> left = secNode;
                plusNode -> right = tanNode;
                secNode -> right = copyTree(node -> right);
                tanNode -> right = copyTree(node -> right);
                if (a != 1.0) {
                    double reciprocal = 1.0 / a;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                    
                    outerMul->left = coefNode;
                    outerMul->right = lnNode;
                    return outerMul;
                }
                return lnNode;
            }
            case MathFunc::csc: {
                ASTNode* minusOne = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                ASTNode* mulNode = new ASTNode({TokenType::Operator, "*"});
                ASTNode* lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                ASTNode* absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
                ASTNode* cscNode = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                ASTNode* cotNode = new ASTNode({TokenType::Function, "cot", MathFunc::cot});
                ASTNode* plusNode = new ASTNode({TokenType::Operator, "+"});

                mulNode -> left = minusOne;
                mulNode -> right = lnNode;
                lnNode -> right = absNode;
                absNode -> right = plusNode;
                plusNode -> left = cscNode;
                plusNode -> right = cotNode;
                cscNode -> right = copyTree(node -> right);
                cotNode -> right = copyTree(node -> right);

                if (a != 1.0) {
                    double reciprocal = 1.0 / a;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                    
                    outerMul->left = coefNode;
                    outerMul->right = mulNode;
                    return outerMul;
                }
                return mulNode;
            }
            case MathFunc::ln: {
                ASTNode* subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                
                subNode -> left = mulNode;
                subNode -> right = copyTree(node -> right);
                mulNode -> left = copyTree(node -> right);
                mulNode -> right = lnNode;
                lnNode -> right = copyTree(node -> right);

                if (a != 1.0) {
                    double reciprocal = 1.0 / a;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                    
                    outerMul->left = coefNode;
                    outerMul->right = subNode;
                    return outerMul;
                }
                return subNode;
            }
            case MathFunc::log:{
                ASTNode* lnNode_out = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                ASTNode* divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                ASTNode* oneNode = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* tenNode = new ASTNode({TokenType::Number, "10", MathFunc::None});
                ASTNode* subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* mulNode_out = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                
                mulNode_out -> left = divNode;
                mulNode_out -> right = subNode;
                divNode -> left = oneNode;
                divNode -> right = lnNode;
                lnNode -> right = tenNode;
                subNode -> left = mulNode;
                subNode -> right = copyTree(node -> right);
                mulNode -> left = copyTree(node -> right);
                mulNode -> right = lnNode;
                lnNode -> right = copyTree(node -> right);

                if (a != 1.0) {
                    double reciprocal = 1.0 / a;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                    
                    outerMul->left = coefNode;
                    outerMul->right = subNode;
                    return outerMul;
                }
                return subNode;

            }
            case MathFunc::arcsin:{
                ASTNode* plusNode = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                ASTNode* powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                ASTNode* arcsinNode = new ASTNode({TokenType::Function, "arcsin", MathFunc::arcsin});
                ASTNode* oneNode = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* halfNode = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
                ASTNode* twoNode = new ASTNode({TokenType::Number, "2", MathFunc::None});

                plusNode -> left = mulNode;
                plusNode -> right = powNode_1;
                mulNode -> left = copyTree(node -> right);
                mulNode -> right = arcsinNode;
                arcsinNode -> right = copyTree(node -> right);
                powNode_1 -> left = subNode;
                powNode_1 -> right = halfNode;
                subNode -> left = oneNode;
                subNode -> right = powNode_2;
                powNode_2 -> left = copyTree(node -> right);
                powNode_2 -> right = twoNode;

                if (a != 1.0) {
                    double reciprocal = 1.0 / a;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                    
                    outerMul->left = coefNode;
                    outerMul->right = plusNode;
                    return outerMul;
                }

                return plusNode;
            }
            case MathFunc::arccos:{
                ASTNode* subNode_1 = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                ASTNode* powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* subNode_2 = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                ASTNode* arccosNode = new ASTNode({TokenType::Function, "arccos", MathFunc::arccos});
                ASTNode* oneNode = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* halfNode = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
                ASTNode* twoNode = new ASTNode({TokenType::Number, "2", MathFunc::None});

                subNode_1 -> left = mulNode;
                subNode_1 -> right = powNode_1;
                mulNode -> left = copyTree(node -> right);
                mulNode -> right = arccosNode;
                arccosNode -> right = copyTree(node -> right);
                powNode_1 -> left = subNode_2;
                powNode_1 -> right = halfNode;
                subNode_2 -> left = oneNode;
                subNode_2 -> right = powNode_2;
                powNode_2 -> left = copyTree(node -> right);
                powNode_2 -> right = twoNode;

                if (a != 1.0) {
                    double reciprocal = 1.0 / a;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                    
                    outerMul->left = coefNode;
                    outerMul->right = subNode_1;
                    return outerMul;
                }

                return subNode_1;
            }
            case MathFunc::arctan:{
                ASTNode* subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                ASTNode* powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* mulNode_left = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* mulNode_right = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* plusNode= new ASTNode({TokenType::Operator, "+", MathFunc::None});
                ASTNode* arctanNode = new ASTNode({TokenType::Function, "arctan", MathFunc::arctan});
                ASTNode* lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                ASTNode* oneNode = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* halfNode = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
                ASTNode* twoNode = new ASTNode({TokenType::Number, "2", MathFunc::None});

                subNode -> left = mulNode_left;
                subNode -> right = mulNode_right;
                mulNode_left -> left = copyTree(node -> right);
                mulNode_left -> right = arctanNode;
                arctanNode -> right = copyTree(node -> right);
                mulNode_right -> left = halfNode;
                mulNode_right -> right = lnNode;
                lnNode -> right = plusNode;
                plusNode -> left = oneNode;
                plusNode -> right = powNode;
                powNode -> left = copyTree(node -> right);
                powNode -> right = twoNode;

                if (a != 1.0) {
                    double reciprocal = 1.0 / a;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                    
                    outerMul->left = coefNode;
                    outerMul->right = subNode;
                    return outerMul;
                }

                return subNode;
            }
            case MathFunc::arccot:{
                ASTNode* plusNode_out = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                ASTNode* powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* mulNode_left = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* mulNode_right = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* plusNode= new ASTNode({TokenType::Operator, "+", MathFunc::None});
                ASTNode* arccotNode = new ASTNode({TokenType::Function, "arccot", MathFunc::arccot});
                ASTNode* lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                ASTNode* oneNode = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* halfNode = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
                ASTNode* twoNode = new ASTNode({TokenType::Number, "2", MathFunc::None});

                plusNode_out -> left = mulNode_left;
                plusNode_out -> right = mulNode_right;
                mulNode_left -> left = copyTree(node -> right);
                mulNode_left -> right = arccotNode;
                arccotNode -> right = copyTree(node -> right);
                mulNode_right -> left = halfNode;
                mulNode_right -> right = lnNode;
                lnNode -> right = plusNode;
                plusNode -> left = oneNode;
                plusNode -> right = powNode;
                powNode -> left = copyTree(node -> right);
                powNode -> right = twoNode;

                if (a != 1.0) {
                    double reciprocal = 1.0 / a;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                    
                    outerMul->left = coefNode;
                    outerMul->right = plusNode_out;
                    return outerMul;
                }

                return plusNode_out;
            }
            case MathFunc::arcsec:{
                ASTNode* subNode_one = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                ASTNode* subNode_two = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                ASTNode* powNode_one = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* powNode_two = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* plusNode = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* arcsecNode = new ASTNode({TokenType::Function, "arcsec", MathFunc::arcsec});
                ASTNode* lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                ASTNode* absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
                ASTNode* oneNode = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* halfNode = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
                ASTNode* twoNode = new ASTNode({TokenType::Number, "2", MathFunc::None});

                subNode_one -> left = mulNode;
                subNode_one -> right = lnNode;
                mulNode -> left = copyTree(node -> right);
                mulNode -> right = arcsecNode;
                arcsecNode -> right = copyTree(node -> right);
                lnNode -> right = absNode;
                absNode -> right = plusNode;
                plusNode -> left = copyTree(node -> right);
                plusNode -> right = powNode_one;
                powNode_one -> left = subNode_two;
                powNode_one -> right = halfNode;
                subNode_two -> left = powNode_two;
                subNode_two -> right = oneNode;
                powNode_two -> left = copyTree(node -> right);
                powNode_two -> right = twoNode;

                if (a != 1.0) {
                    double reciprocal = 1.0 / a;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                    
                    outerMul->left = coefNode;
                    outerMul->right = subNode_one;
                    return outerMul;
                }
                return subNode_one;
            }
            case MathFunc::arccsc:{
                ASTNode* plusNode_one = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                ASTNode* subNode_two = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                ASTNode* powNode_one = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* powNode_two = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* plusNode = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode* arccscNode = new ASTNode({TokenType::Function, "arccsc", MathFunc::arccsc});
                ASTNode* lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                ASTNode* absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
                ASTNode* oneNode = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode* halfNode = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
                ASTNode* twoNode = new ASTNode({TokenType::Number, "2", MathFunc::None});

                plusNode_one -> left = mulNode;
                plusNode_one -> right = lnNode;
                mulNode -> left = copyTree(node -> right);
                mulNode -> right = arccscNode;
                arccscNode -> right = copyTree(node -> right);
                lnNode -> right = absNode;
                absNode -> right = plusNode;
                plusNode -> left = copyTree(node -> right);
                plusNode -> right = powNode_one;
                powNode_one -> left = subNode_two;
                powNode_one -> right = halfNode;
                subNode_two -> left = powNode_two;
                subNode_two -> right = oneNode;
                powNode_two -> left = copyTree(node -> right);
                powNode_two -> right = twoNode;

                if (a != 1.0) {
                    double reciprocal = 1.0 / a;
                    ASTNode* outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    ASTNode* coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                    
                    outerMul->left = coefNode;
                    outerMul->right = plusNode_one;
                    return outerMul;
                }
                return plusNode_one;
            }
            
            default:
                break; // 如果是查表法無法直接積的函數，跳出 switch
        }
    }
    return nullptr; // 目前先回傳 nullptr，表示沒有找到對應的積分結果
}

// LIATE 法則評分系統 (分數越高越適合當 u)
int getPriority(ASTNode* node) {
    if (node == nullptr) return -1;

    // L (Logarithmic): 對數函數 (最高分，因為超好微、超難積)
    if (node->token.type == TokenType::Function && 
       (node->token.funcType == MathFunc::ln || node->token.funcType == MathFunc::log)) {
        return 5;
    }
    // I (Inverse Trig): 反三角函數
    if (node->token.type == TokenType::Function && 
       (node->token.funcType == MathFunc::arcsin || node->token.funcType == MathFunc::arccos || 
        node->token.funcType == MathFunc::arctan || node->token.funcType == MathFunc::arcsec ||
        node->token.funcType == MathFunc::arccsc || node->token.funcType == MathFunc::arccot)) {
        return 4;
    }
    // A (Algebraic): 代數 (多項式、變數 x、次方)
    if (node->token.type == TokenType::Variable || 
       (node->token.type == TokenType::Operator && node->token.value == "^")) {
        return 3;
    }
    // T (Trigonometric): 三角函數
    if (node->token.type == TokenType::Function && 
       (node->token.funcType == MathFunc::sin || node->token.funcType == MathFunc::cos || 
        node->token.funcType == MathFunc::tan || node->token.funcType == MathFunc::cot ||
        node->token.funcType == MathFunc::sec || node->token.funcType == MathFunc::csc)) {
        return 2;
    }
    // E (Exponential): 指數函數 (例如 e^x) -> 其實在 AST 中這也會被判斷為 A，
    // 但因為我們目前指數也是用 ^，如果遇到底數是常數、指數是 x 的，可以特別給 1 分。
    // (為了簡化，A 的規則通常已經足夠擋下多數情況，這裡保留擴充空間)
    
    // 其他 (例如常數) 最不適合當 u
    return 0;
}

struct ExpTrigMatch {
    // --- 掃描過程追蹤用 ---
    bool hasExp = false;    // 找到 e 的指數了嗎？
    bool hasTrig = false;   // 找到三角函數了嗎？

    // --- 最終結果回報用 ---
    bool isMatched = false; // (如果 hasExp 和 hasTrig 都為 true，這個才會變 true)
    double a = 1.0;         // e^(ax) 的 a，預設為 1
    double b = 1.0;         // sin(bx)/cos(bx) 的 b，預設為 1
    bool isSin = true;      // true 代表抓到 sin，false 代表抓到 cos
};

// 專門用來從 "3*x" 或 "x" 中挖出數字 3 的幫手
double extractCoefficient(ASTNode* node) {
    if (node == nullptr) return 1.0; // 防呆，預設為 1

    // 狀況 1: 只有 "x" (例如 e^x) -> 係數是 1
    if (node->token.type == TokenType::Variable && node->token.value == "x") {
        return 1.0;
    }

    // 狀況 2: 發現乘法 "*" (例如 3*x 或 x*3)
    if (node->token.type == TokenType::Operator && node->token.value == "*") {
        
        // 檢查左邊是不是數字，右邊是不是 x
        if (node->left && node->left->token.type == TokenType::Number &&
            node->right && node->right->token.value == "x") {
            return std::stod(node->left->token.value); // stod: String TO Double
        }
        
        // 檢查右邊是不是數字，左邊是不是 x
        if (node->right && node->right->token.type == TokenType::Number &&
            node->left && node->left->token.value == "x") {
            return std::stod(node->right->token.value);
        }
    }
    
    // 如果長得太複雜（例如 x^2），我們這裡先預設回傳 1 
    // (未來可以擴充更強大的代數化簡)
    return 1.0; 
}

void scanForExpAndTrig(ASTNode* node, ExpTrigMatch& matchResult) {
    if (node == nullptr) return;

    // --- 檢查自己 ---
    // 發現 e^...
    if (node->token.value == "^" && node->left && node->left->token.value == "e") {
        matchResult.hasExp = true; 
        // 🌟 把指數部分 (node->right) 丟給幫手去挖 a！
        matchResult.a = extractCoefficient(node->right); 
    }
    // 發現 sin(...)
    else if (node->token.type == TokenType::Function && node->token.funcType == MathFunc::sin) {
        matchResult.hasTrig = true;
        matchResult.isSin = true;
        // 🌟 把括號裡的東西 (node->right) 丟給幫手去挖 b！
        matchResult.b = extractCoefficient(node->right); 
    }
    // 發現 cos(...)
    else if (node->token.type == TokenType::Function && node->token.funcType == MathFunc::cos) {
        matchResult.hasTrig = true;
        matchResult.isSin = false;
        // 🌟 一樣丟給幫手去挖 b！
        matchResult.b = extractCoefficient(node->right); 
    }

    // --- 繼續遞迴搜查 ---
    scanForExpAndTrig(node->left, matchResult);
    scanForExpAndTrig(node->right, matchResult);
}

ExpTrigMatch matchExpTrigPattern(ASTNode* node) {
    ExpTrigMatch result;
    if (node == nullptr || node->token.value != "*") return result;
    
    scanForExpAndTrig(node, result);

    // 結算：如果 e 和 三角函數 都找到了，就判定為「符合特徵」！
    if (result.hasExp && result.hasTrig) {
        result.isMatched = true;
    }

    return result;
}

ASTNode* buildExpTrigResult(ExpTrigMatch match) 
{
    if(match.isSin)
    {
        ASTNode* divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
        ASTNode* mulNode_1 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode* mulNode_2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode* mulNode_3 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode* mulNode_4 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode* mulNode_5 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode* mulNode_6 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode* plusNode_1 = new ASTNode({TokenType::Operator, "+", MathFunc::None});
        ASTNode* subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
        ASTNode* powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        ASTNode* powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        ASTNode* powNode_3 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        ASTNode* sinNode = new ASTNode({TokenType::Function, "sin", MathFunc::sin});
        ASTNode* cosNode = new ASTNode({TokenType::Function, "cos", MathFunc::cos});
        ASTNode* eNode = new ASTNode({TokenType::Constant, "e", MathFunc::None});
        ASTNode* aNode_1 = new ASTNode({TokenType::Number, formatDouble(match.a) , MathFunc::None});
        ASTNode* aNode_2 = new ASTNode({TokenType::Number, formatDouble(match.a) , MathFunc::None});
        ASTNode* aNode_3 = new ASTNode({TokenType::Number, formatDouble(match.a) , MathFunc::None});
        ASTNode* bNode_1 = new ASTNode({TokenType::Number, formatDouble(match.b) , MathFunc::None});
        ASTNode* bNode_2 = new ASTNode({TokenType::Number, formatDouble(match.b) , MathFunc::None});
        ASTNode* bNode_3 = new ASTNode({TokenType::Number, formatDouble(match.b) , MathFunc::None});
        ASTNode* bNode_4 = new ASTNode({TokenType::Number, formatDouble(match.b) , MathFunc::None});
        ASTNode* twoNode_1 = new ASTNode({TokenType::Number, "2" , MathFunc::None});
        ASTNode* twoNode_2 = new ASTNode({TokenType::Number, "2" , MathFunc::None});
        ASTNode* xNode_1 = new ASTNode({TokenType::Variable, "x" , MathFunc::None});
        ASTNode* xNode_2 = new ASTNode({TokenType::Variable, "x" , MathFunc::None});
        ASTNode* xNode_3 = new ASTNode({TokenType::Variable, "x" , MathFunc::None});

        divNode -> left = mulNode_1;
        divNode -> right = plusNode_1;
        mulNode_1 -> left = powNode_1;
        mulNode_1 -> right = subNode;
        powNode_1 -> left = eNode;
        powNode_1 -> right = mulNode_2;
        mulNode_2 -> left = aNode_1;
        mulNode_2 -> right = xNode_1;
        subNode -> left = mulNode_3;
        subNode -> right = mulNode_4;
        mulNode_3 -> left = aNode_2;
        mulNode_3 -> right = sinNode;
        sinNode -> right = mulNode_5;
        mulNode_5 -> left = bNode_1;
        mulNode_5 -> right = xNode_2;
        mulNode_4 -> left = bNode_2;
        mulNode_4 -> right = cosNode;
        cosNode -> right = mulNode_6;
        mulNode_6 -> left = bNode_3;
        mulNode_6 -> right = xNode_3;
        plusNode_1 -> left = powNode_2;
        plusNode_1 -> right = powNode_3;
        powNode_2 -> left = aNode_3;
        powNode_2 -> right = twoNode_1;
        powNode_3 -> left = bNode_4;
        powNode_3 -> right = twoNode_2;

        return divNode;
    }

    else
    {
        ASTNode* divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
        ASTNode* mulNode_1 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode* mulNode_2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode* mulNode_3 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode* mulNode_4 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode* mulNode_5 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode* mulNode_6 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode* plusNode_1 = new ASTNode({TokenType::Operator, "+", MathFunc::None});
        ASTNode* plusNode_2 = new ASTNode({TokenType::Operator, "+", MathFunc::None});
        ASTNode* powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        ASTNode* powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        ASTNode* powNode_3 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        ASTNode* sinNode = new ASTNode({TokenType::Function, "sin", MathFunc::sin});
        ASTNode* cosNode = new ASTNode({TokenType::Function, "cos", MathFunc::cos});
        ASTNode* eNode = new ASTNode({TokenType::Constant, "e", MathFunc::None});
        ASTNode* aNode_1 = new ASTNode({TokenType::Number, formatDouble(match.a) , MathFunc::None});
        ASTNode* aNode_2 = new ASTNode({TokenType::Number, formatDouble(match.a) , MathFunc::None});
        ASTNode* aNode_3 = new ASTNode({TokenType::Number, formatDouble(match.a) , MathFunc::None});
        ASTNode* bNode_1 = new ASTNode({TokenType::Number, formatDouble(match.b) , MathFunc::None});
        ASTNode* bNode_2 = new ASTNode({TokenType::Number, formatDouble(match.b) , MathFunc::None});
        ASTNode* bNode_3 = new ASTNode({TokenType::Number, formatDouble(match.b) , MathFunc::None});
        ASTNode* bNode_4 = new ASTNode({TokenType::Number, formatDouble(match.b) , MathFunc::None});
        ASTNode* twoNode_1 = new ASTNode({TokenType::Number, "2" , MathFunc::None});
        ASTNode* twoNode_2 = new ASTNode({TokenType::Number, "2" , MathFunc::None});
        ASTNode* xNode_1 = new ASTNode({TokenType::Variable, "x" , MathFunc::None});
        ASTNode* xNode_2 = new ASTNode({TokenType::Variable, "x" , MathFunc::None});
        ASTNode* xNode_3 = new ASTNode({TokenType::Variable, "x" , MathFunc::None});

        divNode -> left = mulNode_1;
        divNode -> right = plusNode_1;
        mulNode_1 -> left = powNode_1;
        mulNode_1 -> right = plusNode_2;
        powNode_1 -> left = eNode;
        powNode_1 -> right = mulNode_2;
        mulNode_2 -> left = aNode_1;
        mulNode_2 -> right = xNode_1;
        plusNode_2 -> left = mulNode_3;
        plusNode_2 -> right = mulNode_4;
        mulNode_3 -> left = aNode_2;
        mulNode_3 -> right = cosNode;
        cosNode -> right = mulNode_5;
        mulNode_5 -> left = bNode_1;
        mulNode_5 -> right = xNode_2;
        mulNode_4 -> left = bNode_2;
        mulNode_4 -> right = sinNode;
        sinNode -> right = mulNode_6;
        mulNode_6 -> left = bNode_3;
        mulNode_6 -> right = xNode_3;
        plusNode_1 -> left = powNode_2;
        plusNode_1 -> right = powNode_3;
        powNode_2 -> left = aNode_3;
        powNode_2 -> right = twoNode_1;
        powNode_3 -> left = bNode_4;
        powNode_3 -> right = twoNode_2;

        return divNode;
    }
}

ASTNode* integrationByParts(ASTNode* node,int depth) 
{
    if (depth > 3) return nullptr;
    if (node == nullptr || node->token.type != TokenType::Operator || node->token.value != "*") return nullptr; 

    int leftScore = getPriority(node->left);
    int rightScore = getPriority(node->right);

    ASTNode *u = nullptr, *dv = nullptr;
    if (leftScore >= rightScore) 
    {
        u = node->left;
        dv = node->right;
    } 
    else 
    {
        u = node->right;
        dv = node->left;
    }

    ASTNode* du = derivative(copyTree(u));
    if (du == nullptr) return nullptr; 

    ASTNode* v = integrate(copyTree(dv), depth + 1);
    if (v == nullptr) return nullptr;

    
    ASTNode* mulNode_left = new ASTNode({TokenType::Operator, "*", MathFunc::None});
    ASTNode* mulNode_right = new ASTNode({TokenType::Operator, "*", MathFunc::None});
    mulNode_left -> left = copyTree(u);
    mulNode_left -> right = copyTree(v);
    mulNode_right -> left = copyTree(v);
    mulNode_right -> right = copyTree(du);

    mulNode_right = simplify(mulNode_right);
    ASTNode* integration_vdu = integrate(mulNode_right, depth + 1);
    deleteTree(mulNode_right);
    if (integration_vdu == nullptr) 
    {
        return nullptr; 
    }

    ASTNode* subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
    subNode -> left = mulNode_left;
    subNode -> right = integration_vdu;
    return subNode;
}

ASTNode* multiplyByConstant(ASTNode* resultNode, double k) {
    if (k == 1.0) return resultNode; // 1倍就不用乘了，直接回傳
    
    ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
    // 記得你有 formatDouble 可以把 double 轉回漂亮的字串
    mulNode->left = new ASTNode({TokenType::Number, formatDouble(k), MathFunc::None});
    mulNode->right = resultNode;
    return mulNode;
}

void extractConstant(ASTNode* node, double& out_const, ASTNode*& out_base) {
    out_const = 1.0;     
    out_base = node;     

    if (!node) return;

    if (node->token.type == TokenType::Operator && node->token.value == "*") {
        if (node->left->token.type == TokenType::Number) {
            out_const = std::stod(node->left->token.value);
            out_base = node->right;
        } 
        else if (node->right->token.type == TokenType::Number) {
            out_const = std::stod(node->right->token.value);
            out_base = node->left;
        }
    } 
    else if (node->token.type == TokenType::Number) {
        out_const = std::stod(node->token.value);
        out_base = nullptr;
    }
}

double getProportionalConstant(ASTNode* remainNode, ASTNode* duNode) {
    double const_remain = 1.0, const_du = 1.0;
    ASTNode *base_remain = nullptr, *base_du = nullptr;

    extractConstant(remainNode, const_remain, base_remain);
    extractConstant(duNode, const_du, base_du);

    // 核心結構相同，則計算比例 k
    if (isSameTree(base_remain, base_du)) {
        if (const_du == 0.0) return 0.0; // 防爆機制：避免除以零
        return const_remain / const_du;
    }

    return 0.0; // 核心結構不同，比對失敗
}

ASTNode* tryGenericUSubstitution(ASTNode* f_g_node, ASTNode* remain_node) {
    if (!f_g_node || !remain_node) return nullptr;

    ASTNode* u = nullptr;
    
    // 步驟 1：萃取內層函數 u
    if (f_g_node->token.value == "^" && f_g_node->left->token.value == "e") {
        u = f_g_node->right; // 狀況 A: e^u
    } 
    else if (f_g_node->token.type == TokenType::Function) {
        u = f_g_node->right; // 狀況 B: sin(u), cos(u)...
    } 
    else if (f_g_node->token.value == "^" && f_g_node->right->token.type == TokenType::Number) {
        u = f_g_node->left;  
    }

    if (u == nullptr) return nullptr; // 抓不到 u，此路不通


    ASTNode* du = derivative(copyTree(u));
    ASTNode* simplified_du = simplify(du);

    double k = getProportionalConstant(remain_node, simplified_du);
    
    deleteTree(simplified_du);

    if (k == 0.0) return nullptr; // 變數變換失敗

    // 步驟 4：完美命中！生成對應的積分答案樹
    ASTNode* integrated_f = nullptr;

    // 類型 A: e^u -> 積分是 e^u
    if (f_g_node->token.value == "^" && f_g_node->left->token.value == "e") {
        integrated_f = copyTree(f_g_node); 
    }
    // 類型 B: cos(u) -> 積分是 sin(u)
    else if (f_g_node->token.funcType == MathFunc::cos) {
        integrated_f = new ASTNode({TokenType::Function, "sin", MathFunc::sin});
        integrated_f->right = copyTree(u); 
    }
    // 類型 C: sin(u) -> 積分是 -cos(u)
    else if (f_g_node->token.funcType == MathFunc::sin) {
        ASTNode* cosNode = new ASTNode({TokenType::Function, "cos", MathFunc::cos});
        cosNode->right = copyTree(u);
        
        integrated_f = new ASTNode({TokenType::Operator, "-", MathFunc::None});
        integrated_f->left = new ASTNode({TokenType::Number, "0", MathFunc::None}); 
        integrated_f->right = cosNode;
    }
    else if (f_g_node->token.value == "^" && f_g_node->right->token.type == TokenType::Number) {
        
        // 取得目前的指數 n
        double n = std::stod(f_g_node->right->token.value);
        
        // 🚨 特例防禦：如果 n 是 -1，也就是 u^(-1) = 1/u，積分結果要是 ln|u|
        if (n == -1.0) {
            ASTNode* absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
            absNode->right = copyTree(u); // 把 u 包進絕對值
            
            integrated_f = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
            integrated_f->right = absNode;
        } 
        // 正常情況：次方加 1，係數除以 (n+1)
        else {
            double new_n = n + 1.0;
            
            // 建立新的次方節點 u^(n+1)
            integrated_f = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            integrated_f->left = copyTree(u);
            integrated_f->right = new ASTNode({TokenType::Number, formatDouble(new_n), MathFunc::None});
            
            // 💡 核心魔法：把公式裡的 1/(n+1) 直接融進常數 k 裡面！
            k = k / new_n; 
        }
    }

    // 步驟 5：如果成功套入公式，乘上係數 k 後回傳
    if (integrated_f != nullptr) {
        return multiplyByConstant(integrated_f, k);
    }

    return nullptr;
}

// ===========================================================================
// 🌟 1. 三角代換專用資料結構與基礎雷達
// ===========================================================================
enum class TrigSubType { None, Sine, Tangent, Secant };

struct TrigSubMatch {
    TrigSubType type = TrigSubType::None;
    double a = 0.0; // 常數 a 的值 (例如 4 - x^2，則 a = 2)
};

// 辨識 a^2 - x^2, x^2 - a^2, a^2 + x^2 結構
TrigSubMatch matchTrigSubPattern(ASTNode* node) {
    TrigSubMatch match;
    if (!node || node->token.type != TokenType::Operator || 
       (node->token.value != "+" && node->token.value != "-")) {
        return match;
    }

    double cL = 0, pL = 0, cR = 0, pR = 0;
    bool okL = parseTerm(node->left, cL, pL);
    bool okR = parseTerm(node->right, cR, pR);

    if (okL && okR) {
        // 情況 1：a^2 - x^2 (例如 4 - x^2) -> Sine 代換
        if (node->token.value == "-" && pL == 0.0 && pR == 2.0 && cR == 1.0) {
            if (cL > 0) {
                match.type = TrigSubType::Sine;
                match.a = sqrt(cL);
                return match;
            }
        }
        // 情況 2：x^2 - a^2 (例如 x^2 - 9) -> Secant 代換
        if (node->token.value == "-" && pL == 2.0 && cL == 1.0 && pR == 0.0) {
            if (cR > 0) {
                match.type = TrigSubType::Secant;
                match.a = sqrt(cR);
                return match;
            }
        }
        // 情況 3：a^2 + x^2 或 x^2 + a^2 -> Tangent 代換
        if (node->token.value == "+") {
            if (pL == 0.0 && pR == 2.0 && cR == 1.0 && cL > 0) {
                match.type = TrigSubType::Tangent;
                match.a = sqrt(cL);
                return match;
            } else if (pL == 2.0 && cL == 1.0 && pR == 0.0 && cR > 0) {
                match.type = TrigSubType::Tangent;
                match.a = sqrt(cR);
                return match;
            }
        }
    }
    return match;
}

// ===========================================================================
// 🌟 2. 三角代換進階核心工具群 (補齊先前缺失的函數)
// ===========================================================================

// 【補齊】全樹掃描器：尋找算式中帶有根號 (^0.5) 的三角代換候選者
TrigSubMatch findTrigSubCandidate(ASTNode* node) {
    if (!node) return TrigSubMatch();
    if (node->token.type == TokenType::Operator && node->token.value == "^" &&
        node->right && node->right->token.type == TokenType::Number && node->right->token.value == "0.5") {
        TrigSubMatch m = matchTrigSubPattern(node->left);
        if (m.type != TrigSubType::None) return m;
    }
    TrigSubMatch mL = findTrigSubCandidate(node->left);
    if (mL.type != TrigSubType::None) return mL;
    return findTrigSubCandidate(node->right);
}

// 零件生成器：建立 a * sin(x) 這種 AST 結構
ASTNode* buildTrigTerm(double a, const string& trigFunc) {
    ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
    mulNode->left = new ASTNode({TokenType::Number, formatDouble(a), MathFunc::None});
    ASTNode* funcNode = new ASTNode({TokenType::Function, trigFunc, MathFunc::None});
    funcNode->right = new ASTNode({TokenType::Variable, "x", MathFunc::None}); 
    mulNode->right = funcNode;
    return mulNode;
}

// 正向代換機：把獨立變數 x 替換成相對應的三角函數
ASTNode* applyTrigSubToTree(ASTNode* node, TrigSubMatch match) {
    if (!node) return nullptr;
    if (node->token.type == TokenType::Variable && node->token.value == "x") {
        delete node;
        if (match.type == TrigSubType::Sine)    return buildTrigTerm(match.a, "sin");
        if (match.type == TrigSubType::Tangent) return buildTrigTerm(match.a, "tan");
        if (match.type == TrigSubType::Secant)  return buildTrigTerm(match.a, "sec");
    }
    node->left = applyTrigSubToTree(node->left, match);
    node->right = applyTrigSubToTree(node->right, match);
    return node;
}

// 【補齊】反向代換機：積分完成後，負責把 θ 變數優雅地轉回變數 x 的幾何世界
ASTNode* backSubstitute(ASTNode* node, TrigSubMatch match) {
    if (!node) return nullptr;

    // 情況 A：看到孤立的變數 x (代表純 θ)，轉換成反三角函數
    if (node->token.type == TokenType::Variable && node->token.value == "x") {
        string invName = (match.type == TrigSubType::Sine) ? "arcsin" : 
                         ((match.type == TrigSubType::Tangent) ? "arctan" : "arcsec");
        
        ASTNode* invFunc = new ASTNode({TokenType::Function, invName, MathFunc::None});
        ASTNode* divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
        divNode->left = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        divNode->right = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
        invFunc->right = divNode;
        delete node;
        return invFunc;
    }

    // 情況 B：看到 sin(x)，直接換回 x / a
    if (node->token.type == TokenType::Function && node->token.value == "sin" && 
        node->right && node->right->token.value == "x") {
        if (match.type == TrigSubType::Sine) {
            ASTNode* divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            divNode->left = new ASTNode({TokenType::Variable, "x", MathFunc::None});
            divNode->right = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
            deleteTree(node);
            return divNode;
        }
    }

    // 情況 C：看到 cos(x)，直接換回 √(a^2 - x^2) / a
    if (node->token.type == TokenType::Function && node->token.value == "cos" && 
        node->right && node->right->token.value == "x") {
        if (match.type == TrigSubType::Sine) {
            ASTNode* divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            ASTNode* powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode* subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            
            subNode->left = new ASTNode({TokenType::Number, formatDouble(match.a * match.a), MathFunc::None});
            ASTNode* x2Node = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            x2Node->left = new ASTNode({TokenType::Variable, "x", MathFunc::None});
            x2Node->right = new ASTNode({TokenType::Number, "2", MathFunc::None});
            subNode->right = x2Node;
            
            powNode->left = subNode;
            powNode->right = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
            divNode->left = powNode;
            divNode->right = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
            
            deleteTree(node);
            return divNode;
        }
    }

    node->left = backSubstitute(node->left, match);
    node->right = backSubstitute(node->right, match);
    return node;
}

// ===========================================================================
// 🌟 輔助工具：字串轉 MathFunc 列舉器 (確保動態生成的節點具備型別安全)
// ===========================================================================
MathFunc strToMathFunc(const string& name) {
    if (name == "sin") return MathFunc::sin;
    if (name == "cos") return MathFunc::cos;
    if (name == "tan") return MathFunc::tan;
    if (name == "sec") return MathFunc::sec;
    if (name == "csc") return MathFunc::csc;
    if (name == "cot") return MathFunc::cot;
    if (name == "ln")  return MathFunc::ln;
    if (name == "log") return MathFunc::log;
    if (name == "arcsin") return MathFunc::arcsin;
    if (name == "arccos") return MathFunc::arccos;
    if (name == "arctan") return MathFunc::arctan;
    return MathFunc::None;
}

// =======================================================
// 🌟 分數積分工具一：判斷是否為純多項式
// =======================================================
bool isPolynomial(ASTNode* node) {
    if (node == nullptr) return true;

    // 遇到變數或數字，絕對是合法的多項式元素
    if (node->token.type == TokenType::Number || node->token.type == TokenType::Variable) 
        return true;

    // 嚴格攔截所有特殊函數 (sin, cos, ln, e 等)
    if (node->token.type == TokenType::Function) 
        return false;

    if (node->token.type == TokenType::Operator) {
        string op = node->token.value;
        
        // 加減乘都可以接受，繼續往下檢查
        if (op == "+" || op == "-" || op == "*") {
            return isPolynomial(node->left) && isPolynomial(node->right);
        }
        
        // 如果是除法 (/)，只有當分母是「純數字」時才算多項式 (例如 x/2)
        if (op == "/") {
            if (node->right && node->right->token.type == TokenType::Number) {
                return isPolynomial(node->left);
            }
            return false;
        }

        // 如果是次方 (^)，次方數必須是「非負整數」 (例如 x^2 可以，x^-1 或 x^0.5 不行)
        if (op == "^") {
            if (node->right && node->right->token.type == TokenType::Number) {
                double power = std::stod(node->right->token.value);
                if (power >= 0 && std::abs(power - std::round(power)) < 1e-9) {
                    return isPolynomial(node->left); // 底數也必須是多項式
                }
            }
            return false;
        }
    }
    return false;
}

// =======================================================
// 🌟 分數積分工具二：將展開後的 AST 轉換為 多項式 Map (次方 -> 係數)
// 注意：傳入的 node 必須是已經經過 expand() 暴力展開後的算式！
// =======================================================
void collectPolyTerms(ASTNode* node, std::map<int, double>& polyMap, double currentSign = 1.0) {
    if (node == nullptr) return;

    if (node->token.type == TokenType::Operator && (node->token.value == "+" || node->token.value == "-")) {
        collectPolyTerms(node->left, polyMap, currentSign);
        
        // 如果是減號，右邊那整坨的符號都要翻轉
        double nextSign = (node->token.value == "-") ? -currentSign : currentSign;
        collectPolyTerms(node->right, polyMap, nextSign);
        return;
    }

    // 當走到最底層的單項 (例如 3*x^2 或 x 或是 5)
    double coeff = 0.0, power = 0.0;
    
    // 呼叫你原本寫好的解析單項工具 (parseTerm)
    if (parseTerm(node, coeff, power)) {
        int p = std::round(power);
        polyMap[p] += (coeff * currentSign); // 將係數累加進對應的次方中
    }
}

map<int, double> astToPolyMap(ASTNode* node) {
    map<int, double> polyMap;
    // 為了安全起見，轉換前先強制對它進行一次暴力展開與化簡
    ASTNode* expanded = expand(copyTree(node));
    ASTNode* simplified = simplify(expanded);
    
    collectPolyTerms(simplified, polyMap, 1.0);
    
    deleteTree(simplified);
    return polyMap;
}

// =======================================================
// 🌟 分數積分工具三：將多項式 Map 重新組裝回 AST 語法樹
// =======================================================
ASTNode* polyMapToAST(const std::map<int, double>& polyMap) {
    if (polyMap.empty()) {
        return new ASTNode({TokenType::Number, "0", MathFunc::None});
    }

    ASTNode* resultNode = nullptr;

    // 這裡我們用反向迭代器 (rbegin)，讓組裝出來的樹從最高次方開始 (例如 3x^2 + 2x + 1)
    for (auto it = polyMap.rbegin(); it != polyMap.rend(); ++it) {
        int power = it->first;
        double coeff = it->second;

        // 如果係數是 0，直接跳過這項
        if (std::abs(coeff) < 1e-9) continue;

        ASTNode* termNode = nullptr;

        // 情況 A：常數項 (power == 0)
        if (power == 0) {
            termNode = new ASTNode({TokenType::Number, formatDouble(std::abs(coeff)), MathFunc::None});
        } 
        // 情況 B：一次項 (power == 1)
        else if (power == 1) {
            ASTNode* varNode = new ASTNode({TokenType::Variable, "x", MathFunc::None});
            if (std::abs(coeff) == 1.0) {
                termNode = varNode; // 單純的 x
            } else {
                termNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                termNode->left = new ASTNode({TokenType::Number, formatDouble(std::abs(coeff)), MathFunc::None});
                termNode->right = varNode;
            }
        } 
        // 情況 C：高次項 (power > 1)
        else {
            ASTNode* varNode = new ASTNode({TokenType::Variable, "x", MathFunc::None});
            ASTNode* powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            powNode->left = varNode;
            powNode->right = new ASTNode({TokenType::Number, std::to_string(power), MathFunc::None});

            if (std::abs(coeff) == 1.0) {
                termNode = powNode; // 單純的 x^n
            } else {
                termNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                termNode->left = new ASTNode({TokenType::Number, formatDouble(std::abs(coeff)), MathFunc::None});
                termNode->right = powNode;
            }
        }

        // 把這一項跟前面的項用 + 或 - 接起來
        if (resultNode == nullptr) {
            if (coeff < 0) {
                // 如果第一項就是負的 (例如 -3x^2)
                ASTNode* negNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                negNode->left = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                negNode->right = termNode;
                resultNode = negNode;
            } else {
                resultNode = termNode;
            }
        } else {
            string op = (coeff < 0) ? "-" : "+";
            ASTNode* comboNode = new ASTNode({TokenType::Operator, op, MathFunc::None});
            comboNode->left = resultNode;
            comboNode->right = termNode;
            resultNode = comboNode;
        }
    }

    if (resultNode == nullptr) return new ASTNode({TokenType::Number, "0", MathFunc::None});
    return resultNode;
}

// 🌟 輔助工具：清除 Map 中因為浮點數運算產生的微小誤差 (例如 1e-15 的係數)
void cleanPolyMap(std::map<int, double>& poly) {
    for (auto it = poly.begin(); it != poly.end(); ) {
        if (std::abs(it->second) < 1e-9) {
            it = poly.erase(it); // 係數太小，直接把這項刪掉
        } else {
            ++it;
        }
    }
}

double evalPoly(const std::map<int, double>& poly, double x) {
    double result = 0.0;
    for (auto const& term : poly) {
        result += term.second * std::pow(x, term.first);
    }
    return result;
}



// =======================================================
// 🌟 分數積分第一戰：多項式長除法 N(x) / D(x) = Q(x) ... R(x)
// =======================================================
void polynomialLongDivision(
    std::map<int, double> N, 
    std::map<int, double> D, 
    std::map<int, double>& Q, 
    std::map<int, double>& R
) {
    Q.clear();
    cleanPolyMap(N);
    cleanPolyMap(D);

    if (D.empty()) return; // 防呆機制：分母不能為 0

    // 當分子還有東西，且分子的最高次數 >= 分母的最高次數時，繼續除！
    while (!N.empty()) {
        int degN = N.rbegin()->first;
        int degD = D.rbegin()->first;

        if (degN < degD) break; // 無法再除，剩下的就是餘式

        double coeffN = N.rbegin()->second;
        double coeffD = D.rbegin()->second;

        // 計算這回合的商 (次數相減，係數相除)
        int degQ = degN - degD;
        double coeffQ = coeffN / coeffD;

        Q[degQ] = coeffQ; // 將算出的單項加入商式 Q(x) 中

        // N(x) = N(x) - (coeffQ * x^degQ) * D(x)
        for (auto const& term : D) {
            int currentDeg = term.first;
            double currentCoeff = term.second;
            N[currentDeg + degQ] -= (currentCoeff * coeffQ);
        }
        
        cleanPolyMap(N); // 🌟 關鍵：清除剛才抵銷掉的最高次項及微小誤差
    }
    
    R = N; // 迴圈結束後，剩下的 N 就是餘式 R(x)
}

bool gaussianElimination(std::vector<std::vector<double>>& augMatrix, std::vector<double>& solution) {
    int n = augMatrix.size();
    if (n == 0) return false;
    int m = augMatrix[0].size();
    
    // 1. 前向消去階段 (Forward Elimination)
    for (int i = 0; i < n; ++i) {
        // 🔒 局部主元交換 (Partial Pivoting)：尋找第 i 欄中絕對值最大的列
        int maxRow = i;
        for (int k = i + 1; k < n; ++k) {
            if (std::abs(augMatrix[k][i]) > std::abs(augMatrix[maxRow][i])) {
                maxRow = k;
            }
        }
        
        // 如果最大主元接近 0，代表此聯立方程式無解或有無限多組解（矩陣奇異）
        if (std::abs(augMatrix[maxRow][i]) < 1e-9) {
            return false;
        }
        
        // 交換目前的列與最大主元列
        if (maxRow != i) {
            std::swap(augMatrix[i], augMatrix[maxRow]);
        }
        
        // 開始將目前主元下方的所有橫列對應位置消去為 0
        for (int k = i + 1; k < n; ++k) {
            double factor = augMatrix[k][i] / augMatrix[i][i];
            for (int j = i; j < m; ++j) {
                augMatrix[k][j] -= factor * augMatrix[i][j];
            }
        }
    }
    
    // 2. 後向回代階段 (Back Substitution)
    solution.assign(n, 0.0);
    for (int i = n - 1; i >= 0; --i) {
        double sum = 0.0;
        for (int j = i + 1; j < n; ++j) {
            sum += augMatrix[i][j] * solution[j];
        }
        solution[i] = (augMatrix[i][n] - sum) / augMatrix[i][i];
        
        // 精確度校正：如果解出來的數值極度接近整數，自動進行四捨五入
        if (std::abs(solution[i] - std::round(solution[i])) < 1e-9) {
            solution[i] = std::round(solution[i]);
        }
    }
    
    return true;
}

double findIntegerRoot(const std::map<int, double>& poly) {
    if (poly.empty()) return NAN;
    
    double constantTerm = poly.count(0) ? poly.at(0) : 0.0;
    if (std::abs(constantTerm) < 1e-9) return 0.0; // 如果沒有常數項，x=0 就是一個根

    int c = std::abs(std::round(constantTerm));
    
    // 測試 1 到 c 的所有因數 (包含正負)
    for (int i = 1; i <= c; ++i) {
        if (c % i == 0) {
            if (std::abs(evalPoly(poly, i)) < 1e-9) return i;
            if (std::abs(evalPoly(poly, -i)) < 1e-9) return -i;
        }
    }
    return NAN; // 找不到整數根
}

// ===========================================================================
// 🌟 終極完全體：微積分核心積分引擎 (Integrate Engine)
// ===========================================================================
ASTNode* integrate(ASTNode* node, int depth)
{
    if (node == nullptr) return nullptr;

    // ==========================================
    // 【防線 1】常數項與特種查表
    // ==========================================
    if (isConstant(node)) {
        ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        mulNode->left = copyTree(node); 
        mulNode->right = new ASTNode({TokenType::Variable, "x", MathFunc::None});         
        return mulNode;
    }

    if (node->token.type == TokenType::Operator && node->token.value == "*") {
        ExpTrigMatch match = matchExpTrigPattern(node);
        if (match.isMatched) return buildExpTrigResult(match); 
    }

    // ==========================================
    // 【防線 2】基本查表與線性法則 (加減法拆解)
    // ==========================================
    ASTNode* result = nullptr;
    if ((result = tableIntegral(node))) return result;
    // 💡 確保 depth 精準傳遞，防止遞迴堆疊溢位！
    if ((result = linearityIntegral(node, depth))) return result; 

    // ==========================================
    // 【防線 2.5】有理函數 (分數) 專用戰略區
    // ==========================================
    if (node->token.type == TokenType::Operator && node->token.value == "/") {
        // 檢查分子分母是否都是「純多項式」
        if (isPolynomial(node->left) && isPolynomial(node->right)) {
            
            auto numMap = astToPolyMap(node->left);
            auto denMap = astToPolyMap(node->right);
            
            int degN = numMap.empty() ? 0 : numMap.rbegin()->first;
            int degD = denMap.empty() ? 0 : denMap.rbegin()->first;
            
            // ⚔️ 分數戰略 A：多項式長除法 (當分子次數 >= 分母次數)
            if (degN >= degD && degD > 0) {
                std::map<int, double> Q, R;
                polynomialLongDivision(numMap, denMap, Q, R);
                
                // 把算出來的商 Q 和 餘數 R 變回 AST 語法樹
                ASTNode* qNode = polyMapToAST(Q);
                ASTNode* rNode = polyMapToAST(R);
                
                // 組裝新算式： Q(x) + R(x) / D(x)
                ASTNode* newDiv = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                newDiv->left = rNode;
                newDiv->right = copyTree(node->right); // 分母照舊
                
                ASTNode* finalRes = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                finalRes->left = qNode;
                finalRes->right = newDiv;
                
                // 🚀 把降維拆解完的算式，丟回引擎重新積分！
                ASTNode* result = integrate(finalRes, depth);
                deleteTree(finalRes);
                
                if (result != nullptr) return result;
            }

            // ⚔️ 分數戰略 B：真分式拆解與轉換 (當分子次數 < 分母次數)
            else if (degN < degD && degD > 0) {
                
                // 🚀 新增技能：部分分式展開 (Partial Fractions) - 針對二次分母
                if (degD == 2) {
                    double a_coeff = denMap.count(2) ? denMap[2] : 0.0;
                    double b_coeff = denMap.count(1) ? denMap[1] : 0.0;
                    double c_coeff = denMap.count(0) ? denMap[0] : 0.0;

                    // 計算判別式 b^2 - 4ac
                    double discriminant = b_coeff * b_coeff - 4 * a_coeff * c_coeff;

                    // 如果判別式 > 0，代表分母有兩個相異實根，啟動部分分式！
                    if (discriminant > 0 && a_coeff != 0.0) {
                        // 算出兩個根 r1, r2
                        double r1 = (-b_coeff + sqrt(discriminant)) / (2 * a_coeff);
                        double r2 = (-b_coeff - sqrt(discriminant)) / (2 * a_coeff);

                        // 使用 Heaviside 掩蓋法算 A 和 B
                        double num_r1 = evalPoly(numMap, r1);
                        double num_r2 = evalPoly(numMap, r2);

                        double A = num_r1 / (a_coeff * (r1 - r2));
                        double B = num_r2 / (a_coeff * (r2 - r1));

                        // 建立 A * (x - r1)^-1
                        ASTNode* term1 = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                        term1->left = new ASTNode({TokenType::Variable, "x", MathFunc::None});
                        term1->right = new ASTNode({TokenType::Number, formatDouble(r1), MathFunc::None});
                        ASTNode* pow1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                        pow1->left = term1; pow1->right = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                        ASTNode* partA = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                        partA->left = new ASTNode({TokenType::Number, formatDouble(A), MathFunc::None});
                        partA->right = pow1;

                        // 建立 B * (x - r2)^-1
                        ASTNode* term2 = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                        term2->left = new ASTNode({TokenType::Variable, "x", MathFunc::None});
                        term2->right = new ASTNode({TokenType::Number, formatDouble(r2), MathFunc::None});
                        ASTNode* pow2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                        pow2->left = term2; pow2->right = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                        ASTNode* partB = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                        partB->left = new ASTNode({TokenType::Number, formatDouble(B), MathFunc::None});
                        partB->right = pow2;

                        // 把兩半用加號接起來： A/(x-r1) + B/(x-r2)
                        ASTNode* finalPF = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                        finalPF->left = partA;
                        finalPF->right = partB;

                        // 丟回引擎重新積分！
                        ASTNode* result = integrate(finalPF, depth);
                        deleteTree(finalPF);
                        if (result != nullptr) return result;
                    }

                    // ... 前面的 Heaviside 掩蓋法 (discriminant > 0) ...
                    
                    // 🌟 新增技能：無實根的二次分母 (Arctan 終極公式)
                    // 處理 ∫ K / (ax^2 + bx + c) dx，當 b^2 - 4ac < 0 且分子為常數時
                    else if (discriminant < 0 && a_coeff != 0.0 && degN == 0) {
                        
                        double K = numMap.count(0) ? numMap[0] : 0.0;
                        double sqrt_neg_delta = std::sqrt(-discriminant);

                        // 計算外面的係數: 2K / √(-Δ)
                        double outer_coeff = (2.0 * K) / sqrt_neg_delta;

                        // 建立 2ax
                        ASTNode* two_a_x = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                        two_a_x->left = new ASTNode({TokenType::Number, formatDouble(2.0 * a_coeff), MathFunc::None});
                        two_a_x->right = new ASTNode({TokenType::Variable, "x", MathFunc::None});

                        // 建立內部加法: 2ax + b
                        ASTNode* inner_add = nullptr;
                        if (std::abs(b_coeff) < 1e-9) {
                            inner_add = two_a_x; // 如果 b 是 0，就只留 2ax
                        } else {
                            string op = (b_coeff < 0) ? "-" : "+";
                            inner_add = new ASTNode({TokenType::Operator, op, MathFunc::None});
                            inner_add->left = two_a_x;
                            inner_add->right = new ASTNode({TokenType::Number, formatDouble(std::abs(b_coeff)), MathFunc::None});
                        }

                        // 建立內部除法: (2ax + b) / √(-Δ)
                        ASTNode* inner_div = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                        inner_div->left = inner_add;
                        inner_div->right = new ASTNode({TokenType::Number, formatDouble(sqrt_neg_delta), MathFunc::None});

                        // 建立 arctan(...) (💡 記得用 strToMathFunc 賦予安全的列舉屬性！)
                        ASTNode* arctanNode = new ASTNode({TokenType::Function, "arctan", strToMathFunc("arctan")});
                        arctanNode->right = inner_div;

                        // 乘上外面的係數，大功告成！
                        ASTNode* finalRes = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                        finalRes->left = new ASTNode({TokenType::Number, formatDouble(outer_coeff), MathFunc::None});
                        finalRes->right = arctanNode;

                        // 這裡不需要遞迴丟回 integrate，因為我們已經直接算出最終的積分結果了！
                        return finalRes;
                    }
                }
                // 🚀 終極技能：萬用高次降維剝離 (Degree >= 3)
                else if (degD >= 3) {
                    // 1. 尋找一個整數根 r
                    double r = findIntegerRoot(denMap);
                    
                    if (!std::isnan(r)) {
                        // 2. 建立除式 (x - r)
                        std::map<int, double> linearFactor;
                        linearFactor[1] = 1.0;
                        linearFactor[0] = -r;
                        
                        // 3. 長除法拆解分母： D(x) / (x - r) = Q(x)
                        std::map<int, double> Q_map, remainderD;
                        polynomialLongDivision(denMap, linearFactor, Q_map, remainderD);
                        
                        // 4. 計算常數 A = N(r) / Q(r)
                        double num_r = evalPoly(numMap, r);
                        double Q_r = evalPoly(Q_map, r);
                        
                        if (std::abs(Q_r) > 1e-9) { // 確保分母不為 0
                            double A_val = num_r / Q_r;
                            
                            // 5. 計算剩下的分子 P(x) = (N(x) - A * Q(x)) / (x - r)
                            std::map<int, double> N_minus_AQ = numMap;
                            for (auto const& term : Q_map) {
                                N_minus_AQ[term.first] -= A_val * term.second;
                            }
                            
                            std::map<int, double> P_map, remainderP;
                            polynomialLongDivision(N_minus_AQ, linearFactor, P_map, remainderP);
                            
                            // ==========================================
                            // 6. 將解出來的零件組裝成 AST 語法樹
                            // ==========================================
                            
                            // 第一項： A * (x - r)^-1
                            ASTNode* term1 = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                            term1->left = new ASTNode({TokenType::Variable, "x", MathFunc::None});
                            term1->right = new ASTNode({TokenType::Number, formatDouble(r), MathFunc::None});
                            ASTNode* pow1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                            pow1->left = term1; pow1->right = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                            ASTNode* partA = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                            partA->left = new ASTNode({TokenType::Number, formatDouble(A_val), MathFunc::None});
                            partA->right = pow1;

                            // 🌟 修正點：第二項 P(x) / Q(x) 必須使用純正的除法 "/" 
                            // 這樣才能在下一輪遞迴中，精準觸發【防線 2.5】！
                            ASTNode* P_AST = polyMapToAST(P_map);
                            ASTNode* Q_AST = polyMapToAST(Q_map);
                            
                            ASTNode* partP_Q = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                            partP_Q->left = P_AST;
                            partP_Q->right = Q_AST;

                            // 合併 A /(x-r) + P(x)/Q(x)
                            ASTNode* finalPF = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                            finalPF->left = partA;
                            finalPF->right = partP_Q;

                            // 🚀 丟回引擎重新積分！
                            ASTNode* result = integrate(finalPF, depth);
                            deleteTree(finalPF);
                            if (result != nullptr) return result;
                        }
                    }
                }

                // 🛡️ 兜底戰略 (Fallback)：如果分母無法分解、或是次數不對
                // 退回原本的 A * B^(-1) 戰術，讓 U-Sub 去碰碰運氣
                ASTNode* powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                powNode->left = copyTree(node->right);
                powNode->right = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                
                ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                mulNode->left = copyTree(node->left);
                mulNode->right = powNode;
                
                ASTNode* result = integrate(mulNode, depth);
                deleteTree(mulNode);
                
                if (result != nullptr) return result;
            }

        }
    }

    // ==========================================
    // 【防線 3】乘法 (*) 與 次方 (^) 專用複合戰略調度中心
    // ==========================================
    if (node->token.type == TokenType::Operator && (node->token.value == "*" || node->token.value == "^")) {
        
        // ⚔️ 戰略 3-A：常數提取 (Constant Extraction) - 僅限乘法
        if (node->token.value == "*") {
            if (isConstant(node->left)) {
                ASTNode* cRes = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                cRes->left = copyTree(node->left);             
                cRes->right = integrate(node->right, depth);   
                if (cRes->right != nullptr) return cRes;
                delete cRes->left; delete cRes;
            }
            else if (isConstant(node->right)) {
                ASTNode* cRes = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                cRes->right = copyTree(node->right);           
                cRes->left = integrate(node->left, depth);     
                if (cRes->left != nullptr) return cRes;
                delete cRes->right; delete cRes;
            }
        }

        // 統一收集因子 (乘法樹打平，如果是 ^ 則只會有一個因子)
        std::vector<ASTNode*> factors;
        collectFactors(node, factors); 

        // ⚔️ 戰略 3-B：變數變換 (U-Substitution - 全局試誤版)
        {
            for (size_t candidateIdx = 0; candidateIdx < factors.size(); ++candidateIdx) {
                ASTNode* f = factors[candidateIdx];
                bool isCandidate = false;
                
                if (f->token.type == TokenType::Function) { isCandidate = true; }
                if (f->token.type == TokenType::Operator && f->token.value == "^") {
                    if ((f->left && f->left->token.value == "e") || 
                        (f->right && f->right->token.type == TokenType::Number)) {
                        isCandidate = true;
                    }
                }
                
                if (isCandidate) {
                    ASTNode* f_g_node = factors[candidateIdx]; 
                    std::vector<ASTNode*> remainFactors;
                    for (size_t i = 0; i < factors.size(); ++i) {
                        if (i != candidateIdx) remainFactors.push_back(factors[i]); 
                    }
                    ASTNode* remain_node = rebuildProduct(remainFactors); 
                    ASTNode* uSubResult = tryGenericUSubstitution(f_g_node, remain_node);
                    deleteTree(remain_node); 
                    
                    if (uSubResult != nullptr) return uSubResult;
                }
            }
        }

        // ⚔️ 戰略 3-C：三角倍角公式 (sin(u) * cos(u) -> 0.5 * sin(2u))
        {
            int sinIdx = -1, cosIdx = -1;
            ASTNode* uNode = nullptr;
            for (size_t i = 0; i < factors.size(); ++i) {
                if (factors[i]->token.type == TokenType::Function && factors[i]->token.value == "sin") {
                    for (size_t j = 0; j < factors.size(); ++j) {
                        if (i != j && factors[j]->token.type == TokenType::Function && factors[j]->token.value == "cos") {
                            if (isSameTree(factors[i]->right, factors[j]->right)) {
                                sinIdx = i; cosIdx = j; uNode = factors[i]->right; break;
                            }
                        }
                    }
                }
                if (sinIdx != -1) break;
            }
            
            if (sinIdx != -1 && cosIdx != -1) {
                ASTNode* twoU = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                twoU->left = new ASTNode({TokenType::Number, "2", MathFunc::None});
                twoU->right = copyTree(uNode);
                
                // 💡 確保動態節點擁有 MathFunc
                ASTNode* sin2u = new ASTNode({TokenType::Function, "sin", MathFunc::sin});
                sin2u->right = twoU;
                
                ASTNode* halfSin = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                halfSin->left = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
                halfSin->right = sin2u;
                
                std::vector<ASTNode*> remainFactors;
                for (size_t i = 0; i < factors.size(); ++i) {
                    if (static_cast<int>(i) != sinIdx && static_cast<int>(i) != cosIdx) {
                        remainFactors.push_back(factors[i]);
                    }
                }
                ASTNode* remainNode = rebuildProduct(remainFactors);
                
                ASTNode* finalCombined = nullptr;
                if (remainNode) {
                    finalCombined = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    finalCombined->left = halfSin;
                    finalCombined->right = remainNode;
                } else {
                    finalCombined = halfSin;
                }
                
                ASTNode* result = integrate(finalCombined, depth);
                if (remainNode) deleteTree(finalCombined); else deleteTree(halfSin);
                if (result != nullptr) return result;
            }
        }

        // ⚔️ 戰略 3-D：奇數次方三角剝離法則 (Odd-Power Trig Substitution)
        {
            int oddTrigIdx = -1;
            int m_odd = 0;
            
            for (size_t i = 0; i < factors.size(); ++i) {
                ASTNode* f = factors[i];
                if (f->token.type == TokenType::Operator && f->token.value == "^") {
                    if (f->left && f->left->token.type == TokenType::Function &&
                       (f->left->token.value == "sin" || f->left->token.value == "cos")) {
                        if (f->right && f->right->token.type == TokenType::Number) {
                            double powerVal = std::stod(f->right->token.value);
                            int m = std::round(powerVal);
                            if (std::abs(powerVal - m) < 1e-9 && m % 2 != 0 && m >= 3) {
                                oddTrigIdx = i; m_odd = m; break; 
                            }
                        }
                    }
                }
            }
            
            if (oddTrigIdx != -1) {
                ASTNode* targetNode = factors[oddTrigIdx];
                string funcName = targetNode->left->token.value;
                string oppFunc = (funcName == "sin") ? "cos" : "sin";
                ASTNode* uNode = targetNode->left->right;
                int k = (m_odd - 1) / 2;
                
                // 💡 確保剝離與轉換出來的節點擁有正確的 MathFunc
                ASTNode* peeledFunc = new ASTNode({TokenType::Function, funcName, strToMathFunc(funcName)});
                peeledFunc->right = copyTree(uNode);
                
                ASTNode* oppFuncSq = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode* oppFuncNode = new ASTNode({TokenType::Function, oppFunc, strToMathFunc(oppFunc)});
                oppFuncNode->right = copyTree(uNode);
                oppFuncSq->left = oppFuncNode;
                oppFuncSq->right = new ASTNode({TokenType::Number, "2", MathFunc::None});
                
                ASTNode* oneMinus = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                oneMinus->left = new ASTNode({TokenType::Number, "1", MathFunc::None});
                oneMinus->right = oppFuncSq;
                
                ASTNode* convertedPart = (k == 1) ? oneMinus : new ASTNode({TokenType::Operator, "^", MathFunc::None});
                if (k != 1) { convertedPart->left = oneMinus; convertedPart->right = new ASTNode({TokenType::Number, std::to_string(k), MathFunc::None}); }
                
                ASTNode* newTarget = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                newTarget->left = convertedPart;
                newTarget->right = peeledFunc;
                
                std::vector<ASTNode*> remainFactors;
                for (size_t i = 0; i < factors.size(); ++i) {
                    if (static_cast<int>(i) != oddTrigIdx) remainFactors.push_back(factors[i]);
                }
                ASTNode* remainNode = rebuildProduct(remainFactors);
                
                ASTNode* finalCombined = nullptr;
                if (remainNode) {
                    finalCombined = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    finalCombined->left = newTarget;
                    finalCombined->right = remainNode;
                } else {
                    finalCombined = newTarget;
                }
                
                // 🚀 強制展開與化簡，打碎所有相乘括號，避免 U-Sub 無法辨識！
                ASTNode* expandedFinal = expand(finalCombined);
                ASTNode* simplifiedFinal = simplify(expandedFinal);
                
                ASTNode* result = integrate(simplifiedFinal, depth);
                
                deleteTree(simplifiedFinal);
                if (remainNode) deleteTree(finalCombined); else deleteTree(newTarget);
                
                if (result != nullptr) return result;
            }
        }

        // ⚔️ 戰略 3-E：偶數次方降次法則 (Even-Power Trig Reduction)
        {
            int evenTrigIdx = -1;
            int m_even = 0;
            
            for (size_t i = 0; i < factors.size(); ++i) {
                ASTNode* f = factors[i];
                if (f->token.type == TokenType::Operator && f->token.value == "^") {
                    if (f->left && f->left->token.type == TokenType::Function &&
                       (f->left->token.value == "sin" || f->left->token.value == "cos")) {
                        if (f->right && f->right->token.type == TokenType::Number) {
                            double powerVal = std::stod(f->right->token.value);
                            int m = std::round(powerVal);
                            if (std::abs(powerVal - m) < 1e-9 && m % 2 == 0 && m >= 2) {
                                evenTrigIdx = i; m_even = m; break; 
                            }
                        }
                    }
                }
            }
            
            if (evenTrigIdx != -1) {
                ASTNode* targetNode = factors[evenTrigIdx];
                string funcName = targetNode->left->token.value;
                ASTNode* uNode = targetNode->left->right;
                int k = m_even / 2;
                
                ASTNode* twoU = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                twoU->left = new ASTNode({TokenType::Number, "2", MathFunc::None});
                twoU->right = copyTree(uNode);
                
                // 💡 確保降次生成的節點擁有正確的 MathFunc
                ASTNode* cos2u = new ASTNode({TokenType::Function, "cos", MathFunc::cos});
                cos2u->right = twoU;
                
                ASTNode* halfCos = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                halfCos->left = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
                halfCos->right = cos2u;
                
                string op = (funcName == "cos") ? "+" : "-";
                ASTNode* baseNode = new ASTNode({TokenType::Operator, op, MathFunc::None});
                baseNode->left = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
                baseNode->right = halfCos;
                
                ASTNode* convertedPart = nullptr;
                if (k == 1) {
                    convertedPart = baseNode;
                } else {
                    convertedPart = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                    convertedPart->left = baseNode;
                    convertedPart->right = new ASTNode({TokenType::Number, std::to_string(k), MathFunc::None});
                }
                
                std::vector<ASTNode*> remainFactors;
                for (size_t i = 0; i < factors.size(); ++i) {
                    if (static_cast<int>(i) != evenTrigIdx) remainFactors.push_back(factors[i]);
                }
                ASTNode* remainNode = rebuildProduct(remainFactors);
                
                ASTNode* finalCombined = nullptr;
                if (remainNode) {
                    finalCombined = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    finalCombined->left = convertedPart;
                    finalCombined->right = remainNode;
                } else {
                    finalCombined = convertedPart;
                }
                
                // 🚀 強制展開與化簡，打碎降次公式產生的括號
                ASTNode* expandedFinal = expand(finalCombined);
                ASTNode* simplifiedFinal = simplify(expandedFinal);
                
                ASTNode* result = integrate(simplifiedFinal, depth);
                
                deleteTree(simplifiedFinal);
                if (remainNode) deleteTree(finalCombined); else deleteTree(convertedPart);
                
                if (result != nullptr) return result;
            }
        }

        // ⚔️ 戰略 3-F：分部積分 (Integration By Parts - 僅限乘法，最後大絕招)
        if (node->token.value == "*") {
            ASTNode* byPartsResult = integrationByParts(node, depth);
            if (byPartsResult != nullptr) return byPartsResult; 
        }
    }

    // ==========================================
    // 【防線 4】頂層降維打擊 (僅在 depth == 0 觸發)
    // ==========================================
    if (depth == 0) {
        
        // 🚀 戰略 E：正統三角代換 (Trigonometric Substitution)
        TrigSubMatch match = findTrigSubCandidate(node);
        if (match.type != TrigSubType::None) {
            ASTNode* workingTree = copyTree(node);
            workingTree = applyTrigSubToTree(workingTree, match);
            
            ASTNode* dxNode = nullptr;
            if (match.type == TrigSubType::Sine) dxNode = buildTrigTerm(match.a, "cos"); 
            
            if (dxNode) {
                ASTNode* combinedTree = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                combinedTree->left = workingTree;
                combinedTree->right = dxNode;
                
                // 強效分母標靶攔截對消
                if (combinedTree->left && combinedTree->left->token.type == TokenType::Operator && combinedTree->left->token.value == "/") {
                    if (isSameTree(combinedTree->left->right, combinedTree->right)) {
                        ASTNode* shortcut = copyTree(combinedTree->left->left);
                        deleteTree(combinedTree);
                        combinedTree = shortcut; 
                    }
                }
                
                ASTNode* simplifiedTree = simplify(combinedTree);
                ASTNode* thetaResult = integrate(simplifiedTree, 1);
                deleteTree(simplifiedTree);
                
                if (thetaResult != nullptr) {
                    ASTNode* finalResult = backSubstitute(thetaResult, match);
                    return simplify(finalResult);
                }
            }
        }
        
        // 🚀 戰略 F：暴力代數展開與化簡 (Algebraic Expansion)
        ASTNode* expandedTree = expand(copyTree(node));
        ASTNode* simplifiedExpanded = simplify(expandedTree); 
        
        if (!isSameTree(node, simplifiedExpanded)) {
            ASTNode* finalResult = integrate(simplifiedExpanded, 1); 
            deleteTree(simplifiedExpanded); 
            if (finalResult != nullptr) return finalResult;
        } else {
            deleteTree(simplifiedExpanded);
        }
    }

    return nullptr; // 所有的招數都用盡了，宣告無法積分
}

ASTNode* simplify(ASTNode* node)
{
    if (node == nullptr) return nullptr;

    // 1. 先遞迴化簡左右子樹 (由下往上化簡)
    node->left = simplify(node->left);
    node->right = simplify(node->right);

    // 基本節點直接回傳
    if (node->token.type == TokenType::Number || node->token.type == TokenType::Variable) 
        return node;
    
    // 函數的常數求值 (例如 ln(e), sin(0))
    if (node->token.type == TokenType::Function && node->right && node->right->token.type == TokenType::Number) {
        double val = stod(node->right->token.value);
        double result = 0;
        string funcName = node->token.value;
        bool computable = false;

        if (funcName == "ln") { result = log(val); computable = true; }
        else if (funcName == "log") { result = log10(val); computable = true; }
        else if (funcName == "sin") { result = sin(val); computable = true; }
        else if (funcName == "cos") { result = cos(val); computable = true; }

        if (computable) {
            if (abs(result - round(result)) < 1e-9) {
                result = round(result);
                deleteTree(node); 
                return new ASTNode({TokenType::Number, formatDouble(result), MathFunc::None});
            }
        }
    }

    if (node->token.type == TokenType::Function) {
        return node;
    }

    // 純數字運算
    string op = node->token.value;
    if (node->left && node->right && 
        node->left->token.type == TokenType::Number && 
        node->right->token.type == TokenType::Number) 
    {
        double l_val = stod(node->left->token.value);
        double r_val = stod(node->right->token.value);
        double result = 0;
        
        if (op == "+") result = l_val + r_val;
        else if (op == "-") result = l_val - r_val;
        else if (op == "*") result = l_val * r_val;
        else if (op == "/") result = l_val / r_val;
        else if (op == "^") result = pow(l_val, r_val);

        deleteTree(node);
        return new ASTNode({TokenType::Number, formatDouble(result), MathFunc::None});
    }

    // =======================================================
    // 🌟 運算符號化簡：乘法 (*) 
    // =======================================================
    if (op == "*") 
    {
        if ((node->left && node->left->token.value == "0") || 
            (node->right && node->right->token.value == "0")) {
            deleteTree(node); 
            return new ASTNode({TokenType::Number, "0", MathFunc::None});
        }
        if (node->left && node->left->token.value == "1") {
            ASTNode* keep = node->right; node->right = nullptr; deleteTree(node); return keep;
        }
        if (node->right && node->right->token.value == "1") {
            ASTNode* keep = node->left; node->left = nullptr; deleteTree(node); return keep;
        }

        // ⚙️ 新齒輪一：分式與乘法結合律 (A / B) * C -> (A * C) / B
        if (node->left && node->left->token.type == TokenType::Operator && node->left->token.value == "/") {
            ASTNode* fraction = node->left; ASTNode* C = node->right;
            ASTNode* newDiv = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            ASTNode* newMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            newMul->left = fraction->left; fraction->left = nullptr;
            newMul->right = C; node->right = nullptr;
            newDiv->left = newMul; newDiv->right = fraction->right; fraction->right = nullptr;
            deleteTree(node); return simplify(newDiv);
        }
        // ⚙️ 新齒輪一反向：C * (A / B) -> (C * A) / B
        if (node->right && node->right->token.type == TokenType::Operator && node->right->token.value == "/") {
            ASTNode* C = node->left; ASTNode* fraction = node->right;
            ASTNode* newDiv = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            ASTNode* newMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            newMul->left = C; node->left = nullptr;
            newMul->right = fraction->left; fraction->left = nullptr;
            newDiv->left = newMul; newDiv->right = fraction->right; fraction->right = nullptr;
            deleteTree(node); return simplify(newDiv);
        }

        // ⚙️ 新齒輪二：乘法串同底數冪與常數大歸併 (CAS Core Polynomial Simplifier)
        std::vector<ASTNode*> factors;
        collectFactors(node, factors);

        double final_coeff = 1.0;
        struct BasePower { ASTNode* base; double power; };
        std::vector<BasePower> merged;

        for (ASTNode* f : factors) {
            if (f->token.type == TokenType::Number) {
                final_coeff *= std::stod(f->token.value);
            } else {
                ASTNode* current_base = f; double current_power = 1.0;
                if (f->token.type == TokenType::Operator && f->token.value == "^" && f->right && f->right->token.type == TokenType::Number) {
                    current_base = f->left; current_power = std::stod(f->right->token.value);
                }
                bool found = false;
                for (auto& mp : merged) {
                    if (isSameTree(mp.base, current_base)) { mp.power += current_power; found = true; break; }
                }
                if (!found) merged.push_back({current_base, current_power});
            }
        }

        // 重新組裝打平後的因子
        std::vector<ASTNode*> new_factors;
        if (final_coeff != 1.0 || merged.empty()) {
            new_factors.push_back(new ASTNode({TokenType::Number, formatDouble(final_coeff), MathFunc::None}));
        }
        for (const auto& mp : merged) {
            if (mp.power == 0.0) continue;
            if (mp.power == 1.0) {
                new_factors.push_back(copyTree(mp.base));
            } else {
                ASTNode* powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                powNode->left = copyTree(mp.base);
                powNode->right = new ASTNode({TokenType::Number, formatDouble(mp.power), MathFunc::None});
                new_factors.push_back(powNode);
            }
        }

        // 如果真的有因子被成功合併了，釋放舊樹，回傳新組裝的標準樹
        if (new_factors.size() < factors.size() || (new_factors.size() == 1 && final_coeff == 0.0)) {
            ASTNode* rebuilt = rebuildProduct(new_factors);
            for (ASTNode* nf : new_factors) deleteTree(nf);
            deleteTree(node); return simplify(rebuilt);
        }
        for (ASTNode* nf : new_factors) deleteTree(nf);
    }

    // =======================================================
    // 🌟 運算符號化簡：除法 (/)
    // =======================================================
    if (op == "/") 
    {
        if (node->right && node->right->token.value == "1") {
            ASTNode* keep = node->left; node->left = nullptr; deleteTree(node); return keep;
        }
        if (node->left && node->left->token.value == "0") {
            deleteTree(node); return new ASTNode({TokenType::Number, "0", MathFunc::None});
        }
        if (isSameTree(node->left, node->right)) {
            deleteTree(node); return new ASTNode({TokenType::Number, "1", MathFunc::None});
        }
    }

    // =======================================================
    // 🌟 運算符號化簡：加法 (+) & 減法 (-)
    // =======================================================
    if (op == "+") 
    {
        if (node->left && node->left->token.value == "0") {
            ASTNode* keep = node->right; node->right = nullptr; deleteTree(node); return keep;
        }
        if (node->right && node->right->token.value == "0") {
            ASTNode* keep = node->left; node->left = nullptr; deleteTree(node); return keep;
        }

        string var1, var2; double c1, c2;
        if (isLikeTerm(node->left, var1, c1) && isLikeTerm(node->right, var2, c2)) {
            if (var1 == var2) { 
                double newCoeff = c1 + c2;
                if (newCoeff == 0) { deleteTree(node); return new ASTNode({TokenType::Number, "0", MathFunc::None}); }
                if (newCoeff == 1) { ASTNode* varNode = new ASTNode({TokenType::Variable, var1, MathFunc::None}); deleteTree(node); return varNode; }
                ASTNode* res = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                res->left = new ASTNode({TokenType::Number, formatDouble(newCoeff), MathFunc::None});
                res->right = new ASTNode({TokenType::Variable, var1, MathFunc::None});
                deleteTree(node); return res;
            }
        }
    }

    if (op == "-") 
    {
        if (node->right && node->right->token.value == "0") {
            ASTNode* keep = node->left; node->left = nullptr; deleteTree(node); return keep;
        }
        if (node->left && node->left->token.value == "0") {
            ASTNode* keep = node->right; node->right = nullptr; deleteTree(node);
            ASTNode* negNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            negNode->left = new ASTNode({TokenType::Number, "-1", MathFunc::None});
            negNode->right = keep; return negNode;
        }

        if (node->left && node->right && node->left->token.type == TokenType::Number &&
            node->right->token.type == TokenType::Operator && node->right->token.value == "*") 
        {
            ASTNode* rLeft = node->right->left; ASTNode* rRight = node->right->right;
            if (rLeft && rLeft->token.type == TokenType::Number && std::stod(node->left->token.value) == std::stod(rLeft->token.value)) {
                if (rRight && rRight->token.type == TokenType::Operator && rRight->token.value == "^" && rRight->right && rRight->right->token.value == "2") {
                    ASTNode* funcNode = rRight->left;
                    if (funcNode && funcNode->token.type == TokenType::Function) {
                        string funcName = funcNode->token.value;
                        if (funcName == "sin" || funcName == "cos") {
                            string newFuncName = (funcName == "sin") ? "cos" : "sin";
                            ASTNode* newTerm = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                            newTerm->left = copyTree(node->left);
                            ASTNode* newPow = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                            ASTNode* newFunc = new ASTNode({TokenType::Function, newFuncName, MathFunc::None});
                            newFunc->right = copyTree(funcNode->right);
                            newPow->left = newFunc; newPow->right = new ASTNode({TokenType::Number, "2", MathFunc::None});
                            newTerm->right = newPow; deleteTree(node); return newTerm;
                        }
                    }
                }
            }
        }

        string var1, var2; double c1, c2;
        if (isLikeTerm(node->left, var1, c1) && isLikeTerm(node->right, var2, c2)) {
            if (var1 == var2) { 
                double newCoeff = c1 - c2;
                if (newCoeff == 0) { deleteTree(node); return new ASTNode({TokenType::Number, "0", MathFunc::None}); }
                if (newCoeff == 1) { ASTNode* varNode = new ASTNode({TokenType::Variable, var1, MathFunc::None}); deleteTree(node); return varNode; }
                ASTNode* res = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                res->left = new ASTNode({TokenType::Number, formatDouble(newCoeff), MathFunc::None});
                res->right = new ASTNode({TokenType::Variable, var1, MathFunc::None});
                deleteTree(node); return res;
            }
        }
    }

    // =======================================================
    // 🌟 運算符號化簡：次方 (^)
    // =======================================================
    if (op == "^") 
    {
        if (node->right && node->right->token.value == "1") {
            ASTNode* keep = node->left; node->left = nullptr; deleteTree(node); return keep;
        }

        if (node->right && node->right->token.value == "0.5") {
            if (node->left && node->left->token.type == TokenType::Operator && node->left->token.value == "*") {
                ASTNode* innerMul = node->left;
                if (innerMul->left && innerMul->left->token.type == TokenType::Number &&
                    innerMul->right && innerMul->right->token.type == TokenType::Operator && innerMul->right->token.value == "^" &&
                    innerMul->right->right && innerMul->right->right->token.value == "2") 
                {
                    double A = std::stod(innerMul->left->token.value);
                    ASTNode* funcNode = innerMul->right->left;
                    if (A >= 0) {
                        ASTNode* res = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                        res->left = new ASTNode({TokenType::Number, formatDouble(sqrt(A)), MathFunc::None});
                        res->right = copyTree(funcNode); deleteTree(node); return res;
                    }
                }
            }
        }

        if (node->left && node->left->token.type == TokenType::Operator && node->left->token.value == "*") {
            ASTNode* innerMul = node->left;
            if (innerMul->left && innerMul->left->token.type == TokenType::Number && node->right && node->right->token.type == TokenType::Number) {
                double base_c = std::stod(innerMul->left->token.value);
                double power = std::stod(node->right->token.value);
                double new_c = std::pow(base_c, power);

                ASTNode* newMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                newMul->left = new ASTNode({TokenType::Number, formatDouble(new_c), MathFunc::None});
                ASTNode* newPow = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                newPow->left = copyTree(innerMul->right); newPow->right = copyTree(node->right);
                newMul->right = newPow; deleteTree(node); return newMul;
            }
        }
    }

    return node;
}