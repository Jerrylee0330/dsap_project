#include "calculus.hpp"
#include "utils.hpp"

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

    node->left = simplify(node->left);
    node->right = simplify(node->right);

    if (node -> token.type == TokenType::Number || node -> token.type == TokenType::Variable) 
        return node;
    
    string op = node -> token.value;
    if (node -> left && node -> right && 
        node -> left -> token.type == TokenType::Number && 
        node -> right -> token.type == TokenType::Number) 
    {
        
        double l_val = stod(node -> left -> token.value);
        double r_val = stod(node -> right -> token.value);
        double result = 0;
        
        if (op == "+") result = l_val + r_val;
        else if (op == "-") result = l_val - r_val;
        else if (op == "*") result = l_val * r_val;
        else if (op == "/") result = l_val / r_val;
        else if (op == "^") result = pow(l_val, r_val);

        return new ASTNode({TokenType::Number, formatDouble(result)});
    }

    if(op == "*")
    {
        if( (node->left && node->left->token.value == "0") || 
            (node->right && node->right->token.value == "0") )
            return new ASTNode({TokenType::Number, "0"});
        
        if(node -> left && node -> left -> token.value == "1") return node -> right;
        if(node -> right && node -> right -> token.value == "1") return node -> left;
    }

    if (op == "+") 
    {
        if (node->left && node->left->token.value == "0") return node->right;
        if (node->right && node->right->token.value == "0") return node->left;
    }

    if (op == "-") 
    {
        if (node->right && node->right->token.value == "0") return node->left;
    }

    if (op == "^") 
    {
        if (node->right && node->right->token.value == "1") return node->left;
    }

    return node;
}