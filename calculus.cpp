#include "calculus.hpp"
#include "utils.hpp"

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
    if (a->token.value != b->token.value) return false;
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
            if (node->right && node->right->token.type == TokenType::Number) 
            {
                ASTNode* powNode = new ASTNode({TokenType::Operator, "^"}); 
                ASTNode* multNode = new ASTNode({TokenType::Operator, "*"});

                string n_str = node -> right -> token.value;
                double n_num = stod(n_str);
                double n_num_minus_one = n_num - 1;
                n_str = formatDouble(n_num);

                string n_minus_1_str = formatDouble(n_num_minus_one);
                ASTNode* nNode = new ASTNode({TokenType::Number, n_str});
                ASTNode* newPowerNode = new ASTNode({TokenType::Number, n_minus_1_str});

                powNode -> left = copyTree(node->left);
                powNode -> right = newPowerNode;
            
                multNode -> left = nNode;
                multNode -> right = powNode;

                return multNode;
            }//處理指數微分 x^n 運算

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

ASTNode* integrate(ASTNode* node, int depth = 0);

ASTNode* linearityIntegral(ASTNode* node) {
    // 防禦機制：線性法則只處理運算子 (+, -, *)
    if (node == nullptr || node->token.type != TokenType::Operator) {
        return nullptr;
    }

    // 規則 1：處理加法 (+) 與減法 (-)  [這裡的邏輯完全不變]
    if (node->token.value == "+" || node->token.value == "-") {
        ASTNode* leftIntegral = integrate(node->left,0);
        ASTNode* rightIntegral = integrate(node->right,0);

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
            ASTNode* rightIntegral = integrate(node->right,0); // 只對右邊積分
            
            if (rightIntegral != nullptr) {
                ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                mulNode->left = copyTree(node->left); // 完美複製數字
                mulNode->right = rightIntegral;
                return mulNode;
            }
        }
        // 情況 B：右邊是數字 (例如 x^2 * 3)
        else if (!leftIsNumber && rightIsNumber) {
            ASTNode* leftIntegral = integrate(node->left,0); // 只對左邊積分
            
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
    if (integration_vdu == nullptr) 
    {
        return nullptr; 
    }

    ASTNode* subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
    subNode -> left = mulNode_left;
    subNode -> right = integration_vdu;
    return subNode;
}

ASTNode* integrate(ASTNode* node, int depth)
{
    if (node == nullptr) return nullptr;

    if (isConstant(node)) {
        ASTNode* mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode* xNode = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        
        mulNode->left = copyTree(node); // 把整個常數樹 (例如 log(30)) 放在左邊
        mulNode->right = xNode;         // 右邊乘上 x
        return mulNode;
    }

    if (node->token.type == TokenType::Operator && node->token.value == "*") {
        
        ExpTrigMatch match = matchExpTrigPattern(node);
        if (match.isMatched) {
            return buildExpTrigResult(match); 
        }
    }

    ASTNode* result = nullptr;

    // 1. 嘗試：基本查表法
    if ((result = tableIntegral(node))) return result;

    // 2. 嘗試：線性法則
    if ((result = linearityIntegral(node))) return result;

    // 3. 嘗試：分部積分
    if (node->token.type == TokenType::Operator && node->token.value == "*") {
        // 如果左邊是常數 (例如 -1 * cos(x))
        if (isConstant(node->left)) {
            ASTNode* result = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            result->left = copyTree(node->left);             // 把常數提出來
            result->right = integrate(node->right, depth);   // 剩下的丟進去繼續積
            
            if (result->right != nullptr) return result;
            
            // 💡 如果積分失敗，記得釋放記憶體
            delete result->left; 
            delete result;
            // 這裡不直接 return nullptr，因為如果失敗了，還可以讓下面的邏輯試試看
        }
        // 如果右邊是常數 (例如 cos(x) * 3)
        else if (isConstant(node->right)) {
            ASTNode* result = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            result->right = copyTree(node->right);           // 把常數提出來
            result->left = integrate(node->left, depth);     // 剩下的丟進去繼續積
            
            if (result->left != nullptr) return result;
            
            delete result->right;
            delete result;
        }
    }

    if (node->token.type == TokenType::Operator && node->token.value == "*") {
        ASTNode* byPartsResult = integrationByParts(node,0);
        if (byPartsResult != nullptr) return byPartsResult; // 如果特種部隊解出來了，直接回傳！
    }
    //TODO:變數變換、三角代換

    return nullptr; // 無法積分
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
    
    if (node->token.type == TokenType::Function) {
        return node; 
    }
    
    // 函數的常數求值 (例如 ln(e), sin(0))
    if (node->token.type == TokenType::Function && node->right && node->right->token.type == TokenType::Number) {
        double val = stod(node->right->token.value);
        double result = 0;
        string funcName = node->token.value;
        bool computable = false;

        // 呼叫 C++ 內建數學庫
        if (funcName == "ln") { result = log(val); computable = true; }
        else if (funcName == "log") { result = log10(val); computable = true; }
        else if (funcName == "sin") { result = sin(val); computable = true; }
        else if (funcName == "cos") { result = cos(val); computable = true; }
        // (如果有需要可以繼續擴充)

        if (computable) {
            // 判斷算出來的結果是不是「完美整數」 (容許 1e-9 的浮點數誤差)
            if (abs(result - round(result)) < 1e-9) {
                // 情況 1：是整數！ (例如 ln(e) 算出來是 0.99999999...)
                // 把微小誤差四捨五入掉，變成純數字節點回傳
                result = round(result);
                return new ASTNode({TokenType::Number, formatDouble(result), MathFunc::None});
            }
            // 情況 2：不是整數！ (例如 ln(2) 算出來是 0.693147...)
            // 什麼都不做，直接跳出。讓這個節點原封不動地保留在語法樹上！
        }
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

        return new ASTNode({TokenType::Number, formatDouble(result), MathFunc::None});
    }

    // 運算符號化簡與「同類項合併」
    if (op == "*") 
    {
        if ((node->left && node->left->token.value == "0") || 
            (node->right && node->right->token.value == "0"))
            return new ASTNode({TokenType::Number, "0", MathFunc::None});
        
        if (node->left && node->left->token.value == "1") return node->right;
        if (node->right && node->right->token.value == "1") return node->left;
    }

    if (op == "+") 
    {
        if (node->left && node->left->token.value == "0") return node->right;
        if (node->right && node->right->token.value == "0") return node->left;

        // 同類項合併： c1*x + c2*x = (c1+c2)*x
        string var1, var2;
        double c1, c2;
        if (isLikeTerm(node->left, var1, c1) && isLikeTerm(node->right, var2, c2)) {
            if (var1 == var2) { // 確定變數一樣都是 x
                double newCoeff = c1 + c2;
                if (newCoeff == 0) return new ASTNode({TokenType::Number, "0", MathFunc::None}); // 抵銷為 0
                if (newCoeff == 1) return new ASTNode({TokenType::Variable, var1, MathFunc::None}); // 係數為 1 省略
                
                // 組裝成新的： newCoeff * x
                ASTNode* res = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                res->left = new ASTNode({TokenType::Number, formatDouble(newCoeff), MathFunc::None});
                res->right = new ASTNode({TokenType::Variable, var1, MathFunc::None});
                return res;
            }
        }
    }

    if (op == "-") 
    {
        if (node->right && node->right->token.value == "0") return node->left;

        // 🔥 同類項合併： c1*x - c2*x = (c1-c2)*x
        string var1, var2;
        double c1, c2;
        if (isLikeTerm(node->left, var1, c1) && isLikeTerm(node->right, var2, c2)) {
            if (var1 == var2) { 
                double newCoeff = c1 - c2;
                if (newCoeff == 0) return new ASTNode({TokenType::Number, "0", MathFunc::None}); 
                if (newCoeff == 1) return new ASTNode({TokenType::Variable, var1, MathFunc::None}); 
                
                ASTNode* res = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                res->left = new ASTNode({TokenType::Number, formatDouble(newCoeff), MathFunc::None});
                res->right = new ASTNode({TokenType::Variable, var1, MathFunc::None});
                return res;
            }
        }
    }

    if (op == "^") 
    {
        if (node->right && node->right->token.value == "1") return node->left;
    }

    return node;
}