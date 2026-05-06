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