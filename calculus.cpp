#include "calculus.hpp"
#include "parser.hpp"
#include <map>
#include <cmath>

bool USE_REDUCTION_FORMULA;
int integrate_call_count;

bool isLinearX(ASTNode *node, double &a)
{
    if (node == nullptr)
        return false;
    // 情況 1：單純的 x (相當於 1 * x)
    if (node->token.type == TokenType::Variable && node->token.value == "x")
    {
        a = 1.0;
        return true;
    }
    // 情況 2：遇到乘法 a * x 或 x * a
    if (node->token.type == TokenType::Operator && node->token.value == "*")
    {
        bool leftIsNum = (node->left && node->left->token.type == TokenType::Number);
        bool rightIsX = (node->right && node->right->token.type == TokenType::Variable && node->right->token.value == "x");
        bool rightIsNum = (node->right && node->right->token.type == TokenType::Number);
        bool leftIsX = (node->left && node->left->token.type == TokenType::Variable && node->left->token.value == "x");
        if (leftIsNum && rightIsX)
        {
            a = stod(node->left->token.value);
            return true;
        }
        if (leftIsX && rightIsNum)
        {
            a = stod(node->right->token.value);
            return true;
        }
    }
    return false;
}
// 🌟 幫手 1：多項式雷達 (精準抓出係數與次方)
// 可以看懂 5, x, 3*x, x^2, 4*x^3, 甚至 2*x*3*x^2 這種怪物！
bool parseTerm(ASTNode *node, double &coeff, double &power)
{
    if (!node)
        return false;
    // 1. 純數字 (例如 5 -> 係數 5, 次方 0)
    if (node->token.type == TokenType::Number)
    {
        coeff = std::stod(node->token.value);
        power = 0.0;
        return true;
    }
    // 2. 純變數 (例如 x -> 係數 1, 次方 1)
    if (node->token.type == TokenType::Variable && node->token.value == "x")
    {
        coeff = 1.0;
        power = 1.0;
        return true;
    }
    // 3. 次方項 (例如 x^3 -> 係數 1, 次方 3)
    if (node->token.type == TokenType::Operator && node->token.value == "^")
    {
        if (node->left && node->left->token.value == "x" &&
            node->right && node->right->token.type == TokenType::Number)
        {
            coeff = 1.0;
            power = std::stod(node->right->token.value);
            return true;
        }
    }
    // 4. 乘法組合 (例如 3*x, 4*x^2, 或是展開產生的一坨 x*x)
    if (node->token.type == TokenType::Operator && node->token.value == "*")
    {
        double cL = 1, pL = 0, cR = 1, pR = 0;
        // 遞迴解析左右兩邊
        bool okL = parseTerm(node->left, cL, pL);
        bool okR = parseTerm(node->right, cR, pR);
        if (okL && okR)
        {
            coeff = cL * cR; // 數字相乘
            power = pL + pR; // 次方相加 (指數律: x^a * x^b = x^(a+b))
            return true;
        }
    }
    return false; // 看不懂的複雜結構 (例如 sin(x))
}

bool containsVariable(ASTNode *node, const string &varName)
{
    if (node == nullptr)
        return false;
    // 1. 如果是變數節點，檢查名稱是否匹配
    if (node->token.type == TokenType::Variable)
    {
        return node->token.value == varName;
    }
    // 2. 遞迴搜尋左右子樹
    return containsVariable(node->left, varName) || containsVariable(node->right, varName);
}
bool isConstant(ASTNode *node)
{
    if (node == nullptr)
        return true;
    // 如果抓到變數 x，就絕對不是常數
    if (node->token.type == TokenType::Variable && node->token.value == "x")
    {
        return false;
    }
    // 遞迴檢查左右兩邊，必須兩邊都是常數才算常數
    return isConstant(node->left) && isConstant(node->right);
}
bool isSameTree(ASTNode *a, ASTNode *b)
{
    if (!a && !b)
        return true;
    if (!a || !b)
        return false;
    if (a->token.type != b->token.type)
        return false;
    if (a->token.type == TokenType::Number)
    {
        if (std::stod(a->token.value) != std::stod(b->token.value))
        {
            return false;
        }
    }
    else
    {
        if (a->token.value != b->token.value)
            return false;
    }
    return isSameTree(a->left, b->left) && isSameTree(a->right, b->right);
}
ASTNode *copyTree(ASTNode *node)
{
    if (node == nullptr)
        return nullptr;
    ASTNode *newNode = new ASTNode(node->token);
    newNode->left = copyTree(node->left);
    newNode->right = copyTree(node->right);
    return newNode;
}
void deleteTree(ASTNode *node)
{
    if (node == nullptr)
        return;
    deleteTree(node->left);
    deleteTree(node->right);
    delete node;
}
ASTNode *derivative(ASTNode *node)
{
    if (node == nullptr)
        return nullptr;
    // 處理常數微分
    if (node->token.type == TokenType::Number)
    {
        return new ASTNode({TokenType::Number, "0"});
    }
    if (node->token.type == TokenType::Constant)
    {
        return new ASTNode({TokenType::Number, "0", MathFunc::None});
    }
    // 處理變數微分
    if (node->token.type == TokenType::Variable)
    {
        if (node->token.value == "x")
        {
            return new ASTNode({TokenType::Number, "1"});
        } // 如果是x變數，微分後為1
        else
        {
            return new ASTNode({TokenType::Number, "0"});
        } // 如果是其他變數，微分後為 0
    }
    if (node->token.type == TokenType::Operator)
    {
        string op = node->token.value;
        if (op == "^")
        {
            // 處理多項式次方微分: (u^n)' = n * u^(n-1) * u'
            if (node->right && node->right->token.type == TokenType::Number)
            {
                ASTNode *powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode *multNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *chainMulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                string n_str = node->right->token.value;
                double n_num = stod(n_str);
                double n_num_minus_one = n_num - 1;
                n_str = formatDouble(n_num);
                string n_minus_1_str = formatDouble(n_num_minus_one);
                ASTNode *nNode = new ASTNode({TokenType::Number, n_str, MathFunc::None});
                ASTNode *newPowerNode = new ASTNode({TokenType::Number, n_minus_1_str, MathFunc::None});
                powNode->left = copyTree(node->left);
                powNode->right = newPowerNode;
                multNode->left = nNode;
                multNode->right = powNode;
                chainMulNode->left = multNode;                // 左邊放剛剛算好的 n * u^(n-1)
                chainMulNode->right = derivative(node->left); // 右邊放底數的微分 (u')
                return chainMulNode;
            }
            else if (node->left && node->left->token.type == TokenType::Number)
            {
                ASTNode *mulNode_1 = new ASTNode({TokenType::Operator, "*"});
                ASTNode *mulNode_2 = new ASTNode({TokenType::Operator, "*"});
                ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                mulNode_1->left = mulNode_2;
                mulNode_1->right = derivative(node->right);
                mulNode_2->left = lnNode;
                mulNode_2->right = copyTree(node);
                lnNode->right = copyTree(node->left);
                return mulNode_1;
            } // a^u' = ln(a)*a^u*u'
            else
            {
                ASTNode *mulNode_1 = new ASTNode({TokenType::Operator, "*"});
                ASTNode *mulNode_2 = new ASTNode({TokenType::Operator, "*"});
                ASTNode *mulNode_3 = new ASTNode({TokenType::Operator, "*"});
                ASTNode *addNode = new ASTNode({TokenType::Operator, "+"});
                ASTNode *powNode = new ASTNode({TokenType::Operator, "^"});
                ASTNode *divNode = new ASTNode({TokenType::Operator, "/"});
                ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                mulNode_1->left = powNode;
                mulNode_1->right = addNode;
                powNode->left = copyTree(node->left);
                powNode->right = copyTree(node->right);
                addNode->left = mulNode_2;
                addNode->right = mulNode_3;
                mulNode_2->left = derivative(node->right);
                mulNode_2->right = lnNode;
                lnNode->right = copyTree(node->left);
                mulNode_3->left = copyTree(node->right);
                mulNode_3->right = divNode;
                divNode->left = derivative(node->left);
                divNode->right = copyTree(node->left);
                return mulNode_1;
            }
        }
        // 【加法法則】 (f + g)' = f' + g'
        if (op == "+")
        {
            ASTNode *leftDiff = derivative(node->left);
            ASTNode *rightDiff = derivative(node->right);
            ASTNode *result = new ASTNode({TokenType::Operator, "+"});
            result->left = leftDiff;
            result->right = rightDiff;
            return result;
        }
        if (op == "-")
        {
            ASTNode *leftDiff = derivative(node->left);
            ASTNode *rightDiff = derivative(node->right);
            ASTNode *result = new ASTNode({TokenType::Operator, "-"});
            result->left = leftDiff;
            result->right = rightDiff;
            return result;
        }
        if (op == "*")
        {
            ASTNode *leftDiff = derivative(node->left);
            ASTNode *rightDiff = derivative(node->right);
            ASTNode *mulNode_left = new ASTNode({TokenType::Operator, "*"});
            ASTNode *mulNode_right = new ASTNode({TokenType::Operator, "*"});
            mulNode_left->left = leftDiff;
            mulNode_left->right = copyTree(node->right);
            mulNode_right->left = copyTree(node->left);
            mulNode_right->right = rightDiff;
            ASTNode *result = new ASTNode({TokenType::Operator, "+"});
            result->left = mulNode_left;
            result->right = mulNode_right;
            return result;
        }
        if (op == "/")
        {
            ASTNode *leftDiff = derivative(node->left);
            ASTNode *rightDiff = derivative(node->right);
            ASTNode *mulNode_left = new ASTNode({TokenType::Operator, "*"});
            ASTNode *mulNode_right = new ASTNode({TokenType::Operator, "*"});
            mulNode_left->left = leftDiff;
            mulNode_left->right = copyTree(node->right);
            mulNode_right->left = copyTree(node->left);
            mulNode_right->right = rightDiff;
            ASTNode *numerator = new ASTNode({TokenType::Operator, "-"});
            numerator->left = mulNode_left;
            numerator->right = mulNode_right;
            ASTNode *denominator = new ASTNode({TokenType::Operator, "^"});
            denominator->left = copyTree(node->right);
            denominator->right = new ASTNode({TokenType::Number, "2"});
            ASTNode *result = new ASTNode({TokenType::Operator, "/"});
            result->left = numerator;
            result->right = denominator;
            return result;
        }
    }
    if (node->token.type == TokenType::Function)
    {
        ASTNode *innerDiff = derivative(node->right);
        ASTNode *outerDiff = nullptr;
        switch (node->token.funcType)
        {
        case MathFunc::sin:
        {
            // sin(u)' = cos(u)
            outerDiff = new ASTNode({TokenType::Function, "cos", MathFunc::cos});
            outerDiff->right = copyTree(node->right); // 把原本的 u 複製過來
            break;
        }
        case MathFunc::cos:
        {
            // cos(u)' = -1 * sin(u)
            ASTNode *minusOne = new ASTNode({TokenType::Number, "-1", MathFunc::None});
            ASTNode *sinNode = new ASTNode({TokenType::Function, "sin", MathFunc::sin});
            sinNode->right = copyTree(node->right);
            outerDiff = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            outerDiff->left = minusOne;
            outerDiff->right = sinNode;
            break;
        }
        case MathFunc::tan:
        {
            // tan(u)' = sec^2(u)
            ASTNode *secNode = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
            secNode->right = copyTree(node->right);
            ASTNode *two = new ASTNode({TokenType::Number, "2", MathFunc::None});
            outerDiff = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            outerDiff->left = secNode;
            outerDiff->right = two;
            break;
        }
        case MathFunc::cot:
        {
            // cot(u)' = -csc^2(u)
            ASTNode *minusOne = new ASTNode({TokenType::Number, "-1", MathFunc::None});
            ASTNode *cscNode = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
            cscNode->right = copyTree(node->right);
            ASTNode *two = new ASTNode({TokenType::Number, "2", MathFunc::None});
            outerDiff = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            powNode->left = cscNode;
            powNode->right = two;
            outerDiff->left = minusOne;
            outerDiff->right = powNode;
            break;
        }
        case MathFunc::sec:
        {
            // sec(u)' = sec(u) * tan(u)
            ASTNode *secNode = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
            secNode->right = copyTree(node->right);
            ASTNode *tanNode = new ASTNode({TokenType::Function, "tan", MathFunc::tan});
            tanNode->right = copyTree(node->right);
            outerDiff = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            outerDiff->left = secNode;
            outerDiff->right = tanNode;
            break;
        }
        case MathFunc::csc:
        {
            // csc(u)' = -csc(u)*cot(u)
            ASTNode *cscNode = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
            cscNode->right = copyTree(node->right);
            ASTNode *cotNode = new ASTNode({TokenType::Function, "cot", MathFunc::cot});
            cotNode->right = copyTree(node->right);
            ASTNode *minusOne = new ASTNode({TokenType::Number, "-1", MathFunc::None});
            ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            mulNode->left = cscNode;
            mulNode->right = cotNode;
            outerDiff = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            outerDiff->left = minusOne;
            outerDiff->right = mulNode;
            break;
        }
        case MathFunc::ln:
        {
            // ln(u)' = 1 / u
            ASTNode *one = new ASTNode({TokenType::Number, "1", MathFunc::None});
            outerDiff = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            outerDiff->left = one;
            outerDiff->right = copyTree(node->right);
            break;
        }
        case MathFunc::log:
        {
            // log_10(u) 的外部微分 outerDiff = 1 / (u * ln(10))
            ASTNode *divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            ASTNode *one = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *mulDenom = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *ten = new ASTNode({TokenType::Number, "10", MathFunc::None});
            ASTNode *ln10Node = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
            ln10Node->right = ten;
            mulDenom->left = copyTree(node->right); // 這裡的 u 只需要 copy，不用微分
            mulDenom->right = ln10Node;
            divNode->left = one;
            divNode->right = mulDenom;
            outerDiff = divNode;
            break;
        }
        case MathFunc::arcsin:
        {
            // arcsin(u) 的外部微分 outerDiff = (1 - u^2)^-0.5
            ASTNode *powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            ASTNode *one = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *two = new ASTNode({TokenType::Number, "2", MathFunc::None});
            ASTNode *neg_half_point_five = new ASTNode({TokenType::Number, "-0.5", MathFunc::None});
            powNode_1->left = subNode;
            powNode_1->right = neg_half_point_five;
            subNode->left = one;
            subNode->right = powNode_2;
            powNode_2->left = copyTree(node->right);
            powNode_2->right = two;
            outerDiff = powNode_1;
            break;
        }
        case MathFunc::arccos:
        {
            // arccos(u) 的外部微分 outerDiff = -1 * (1 - u^2)^-0.5 (已修正數學邏輯)
            ASTNode *mulOuter = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *minus_one = new ASTNode({TokenType::Number, "-1", MathFunc::None});
            ASTNode *powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            ASTNode *one = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *two = new ASTNode({TokenType::Number, "2", MathFunc::None});
            ASTNode *neg_half_point_five = new ASTNode({TokenType::Number, "-0.5", MathFunc::None});
            powNode_1->left = subNode;
            powNode_1->right = neg_half_point_five;
            subNode->left = one;
            subNode->right = powNode_2;
            powNode_2->left = copyTree(node->right);
            powNode_2->right = two;
            mulOuter->left = minus_one;
            mulOuter->right = powNode_1;
            outerDiff = mulOuter;
            break;
        }
        case MathFunc::arctan:
        {
            // arctan(u) 的外部微分 outerDiff = 1 / (1 + u^2)
            ASTNode *powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            ASTNode *addNode = new ASTNode({TokenType::Operator, "+", MathFunc::None});
            ASTNode *one_1 = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *one_2 = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *two = new ASTNode({TokenType::Number, "2", MathFunc::None});
            divNode->left = one_1;
            divNode->right = addNode;
            addNode->left = one_2;
            addNode->right = powNode;
            powNode->left = copyTree(node->right);
            powNode->right = two;
            outerDiff = divNode;
            break;
        }
        case MathFunc::arccot:
        {
            // arccot(u) 的外部微分 outerDiff = -1 / (1 + u^2)
            ASTNode *powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            ASTNode *addNode = new ASTNode({TokenType::Operator, "+", MathFunc::None});
            ASTNode *minus_one = new ASTNode({TokenType::Number, "-1", MathFunc::None});
            ASTNode *one = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *two = new ASTNode({TokenType::Number, "2", MathFunc::None});
            divNode->left = minus_one;
            divNode->right = addNode;
            addNode->left = one;
            addNode->right = powNode;
            powNode->left = copyTree(node->right);
            powNode->right = two;
            outerDiff = divNode;
            break;
        }
        case MathFunc::arcsec:
        {
            // arcsec(u) 的外部微分 outerDiff = 1 / (|u| * (u^2 - 1)^0.5)
            ASTNode *mulNode_2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            ASTNode *subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            ASTNode *one_1 = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *one_2 = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *two = new ASTNode({TokenType::Number, "2", MathFunc::None});
            ASTNode *point_five = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
            ASTNode *absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
            divNode->left = one_1;
            divNode->right = mulNode_2;
            mulNode_2->left = absNode;
            mulNode_2->right = powNode_1;
            absNode->right = copyTree(node->right);
            powNode_1->left = subNode;
            powNode_1->right = point_five;
            subNode->left = powNode_2;
            subNode->right = one_2;
            powNode_2->left = copyTree(node->right);
            powNode_2->right = two;
            outerDiff = divNode;
            break;
        }
        case MathFunc::arccsc:
        {
            // arccsc(u) 的外部微分 outerDiff = -1 / (|u| * (u^2 - 1)^0.5)
            ASTNode *mulNode_2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            ASTNode *subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            ASTNode *minus_one = new ASTNode({TokenType::Number, "-1", MathFunc::None});
            ASTNode *one = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *two = new ASTNode({TokenType::Number, "2", MathFunc::None});
            ASTNode *point_five = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
            ASTNode *absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
            divNode->left = minus_one;
            divNode->right = mulNode_2;
            mulNode_2->left = absNode;
            mulNode_2->right = powNode_1;
            absNode->right = copyTree(node->right);
            powNode_1->left = subNode;
            powNode_1->right = point_five;
            subNode->left = powNode_2;
            subNode->right = one;
            powNode_2->left = copyTree(node->right);
            powNode_2->right = two;
            outerDiff = divNode;
            break;
        }
        case MathFunc::abs:
        {
            ASTNode *divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            divNode->left = copyTree(node->right); // 分子放 u
            ASTNode *absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
            absNode->right = copyTree(node->right); // 分母放 |u|
            divNode->right = absNode;
            outerDiff = divNode;
            break;
        }
        default:
            break; // 如果未來有其他函數可以繼續擴充
        }
        if (outerDiff == nullptr)
        {
            deleteTree(innerDiff);
            return nullptr;
        }
        ASTNode *result = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        result->left = outerDiff;
        result->right = innerDiff;
        return result;
    }
    return nullptr;
}
bool hasPlusMinus(ASTNode *node)
{
    if (node == nullptr)
        return false;
    if (node->token.type == TokenType::Operator &&
        (node->token.value == "+" || node->token.value == "-"))
    {
        return true;
    }
    return hasPlusMinus(node->left) || hasPlusMinus(node->right);
}
// =======================================================
// 🌟 輔助工具 1：因子攤平收集器 (Factor Collector)
// 負責把連續相乘的樹枝全部打平，變成一個清單
// 例如：((x^2+1)^5 * 2) * x  會被攤平成 -> [(x^2+1)^5, 2, x]
// =======================================================
void collectFactors(ASTNode *node, std::vector<ASTNode *> &factors)
{
    if (!node)
        return;
    // 如果遇到乘法節點，就繼續往左右兩邊往下挖
    if (node->token.type == TokenType::Operator && node->token.value == "*")
    {
        collectFactors(node->left, factors);
        collectFactors(node->right, factors);
    }
    else
    {
        // 如果已經不是乘法了（例如遇到 (x^2+1)^5 或 2 或 x），就把它存進清單裡
        factors.push_back(node);
    }
}
// =======================================================
// 🌟 輔助工具 2：因子重新組裝機 (Product Rebuilder)
// 負責把挑剩的零件，重新用乘號組合回一棵標準的 AST 樹
// 例如：清單剩 [2, x] -> 會被組裝成 (2 * x) 的樹結構
// =======================================================
ASTNode *rebuildProduct(const std::vector<ASTNode *> &factors)
{
    if (factors.empty())
        return new ASTNode({TokenType::Number, "1", MathFunc::None});
    ASTNode *root = copyTree(factors[0]);
    for (size_t i = 1; i < factors.size(); ++i)
    {
        ASTNode *newRoot = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        newRoot->left = root;
        newRoot->right = copyTree(factors[i]);
        root = newRoot;
    }
    return root;
}
// 🚀 單次掃描展開器 (保留之前的邏輯，但在安全的複製樹上運作)
bool expandOnce(ASTNode *&node)
{
    if (node == nullptr)
        return false;
    if (expandOnce(node->left))
        return true;
    if (expandOnce(node->right))
        return true;
    // 處理次方展開 (U)^n -> U * (U)^(n-1)
    if (node->token.type == TokenType::Operator && node->token.value == "^")
    {
        if (node->right && node->right->token.type == TokenType::Number)
        {
            double n = std::stod(node->right->token.value);
            if (n > 0 && floor(n) == n && hasPlusMinus(node->left))
            {
                if (n == 1.0)
                {
                    ASTNode *keep = node->left;
                    node->left = nullptr;
                    deleteTree(node);
                    node = keep;
                    return true;
                }
                else
                {
                    ASTNode *U = copyTree(node->left);
                    ASTNode *newPower = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                    newPower->left = copyTree(node->left);
                    newPower->right = new ASTNode({TokenType::Number, formatDouble(n - 1.0), MathFunc::None});
                    ASTNode *newRoot = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    newRoot->left = U;
                    newRoot->right = newPower;
                    deleteTree(node);
                    node = newRoot;
                    return true;
                }
            }
        }
    }
    // 處理分配律
    if (node->token.type == TokenType::Operator && node->token.value == "*")
    {
        if (node->left && node->left->token.type == TokenType::Operator &&
            (node->left->token.value == "+" || node->left->token.value == "-"))
        {
            ASTNode *A = node->left->left;
            ASTNode *B = node->left->right;
            ASTNode *C = node->right;
            string op = node->left->token.value;
            ASTNode *newRoot = new ASTNode({TokenType::Operator, op, MathFunc::None});
            ASTNode *mul1 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            mul1->left = copyTree(A);
            mul1->right = copyTree(C);
            ASTNode *mul2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            mul2->left = copyTree(B);
            mul2->right = copyTree(C);
            newRoot->left = mul1;
            newRoot->right = mul2;
            deleteTree(node);
            node = newRoot;
            return true;
        }
        else if (node->right && node->right->token.type == TokenType::Operator &&
                 (node->right->token.value == "+" || node->right->token.value == "-"))
        {
            ASTNode *A = node->left;
            ASTNode *B = node->right->left;
            ASTNode *C = node->right->right;
            string op = node->right->token.value;
            ASTNode *newRoot = new ASTNode({TokenType::Operator, op, MathFunc::None});
            ASTNode *mul1 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            mul1->left = copyTree(A);
            mul1->right = copyTree(B);
            ASTNode *mul2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            mul2->left = copyTree(A);
            mul2->right = copyTree(C);
            newRoot->left = mul1;
            newRoot->right = mul2;
            deleteTree(node);
            node = newRoot;
            return true;
        }
    }
    return false;
}
// 🌟 最終安全外掛版 expand：保護原始樹，回傳新樹！
ASTNode *expand(ASTNode *node)
{
    if (node == nullptr)
        return nullptr;
    // 1. 完全拷貝一棵新樹，絕對不破壞傳進來的原始 AST (致敬你的原版安全設計)
    ASTNode *root = copyTree(node);
    // 2. 對這棵新樹進行迭代展開，避免遞迴 Stack Overflow
    while (expandOnce(root))
    {
        // 只要有展開，就繼續
    }
    return root; // 回傳這棵安全且完全展開的新樹！
}
ASTNode *integrate(ASTNode *node, int depth = 0);

int getPriority(ASTNode *node)
{
    if (node == nullptr)
        return -1;
    // 🛡️ 穿透常數與乘除法
    if (node->token.type == TokenType::Operator && (node->token.value == "*" || node->token.value == "/"))
    {
        return std::max(getPriority(node->left), getPriority(node->right));
    }
    // L: 對數
    if (node->token.type == TokenType::Function &&
        (node->token.funcType == MathFunc::ln || node->token.value == "ln" ||
         node->token.funcType == MathFunc::log || node->token.value == "log"))
    {
        return 5;
    }
    // I: 反三角
    if (node->token.type == TokenType::Function &&
        (node->token.value == "arcsin" || node->token.value == "arccos" || node->token.value == "arctan" ||
         node->token.value == "arcsec" || node->token.value == "arccsc" || node->token.value == "arccot"))
    {
        return 4;
    }
    if (node->token.type == TokenType::Operator && node->token.value == "^")
    {
        // 1. 如果底數是 e，這就是指數函數 (E)，給最低分 1 分！
        if (node->left && (node->left->token.value == "e" || node->left->token.value == "E"))
        {
            return 1;
        }
        // 2. 🛡️ 血統繼承：如果底數是對數(5分)或反三角(4分)，次方必須繼承高分！
        // 這樣 ln(x)^2 才能拿到 5 分，成功觸發「乘 1 救援機制」
        int baseScore = getPriority(node->left);
        if (baseScore >= 4)
        {
            return baseScore;
        }
        // 3. 否則它是一般的變數次方 (例如 x^2)，屬於代數 (A)，給 3 分
        return 3;
    }
    // A: 單純的變數 x
    if (node->token.type == TokenType::Variable)
    {
        return 3;
    }
    // T: 三角函數
    if (node->token.type == TokenType::Function &&
        (node->token.value == "sin" || node->token.value == "cos" || node->token.value == "tan" ||
         node->token.value == "cot" || node->token.value == "sec" || node->token.value == "csc"))
    {
        return 2;
    }
    return 0;
}

ASTNode *integrationByParts(ASTNode *node, int depth)
{
    if (depth > 20)
        return nullptr;
    if (node == nullptr || node->token.type != TokenType::Operator || node->token.value != "*")
        return nullptr;

    int leftScore = getPriority(node->left);
    int rightScore = getPriority(node->right);

    ASTNode *u = nullptr;
    ASTNode *dv = nullptr;
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

    ASTNode *du = derivative(copyTree(u));
    if (du == nullptr)
    {
        return nullptr;
    }

    ASTNode *v = integrate(copyTree(dv), depth + 1);
    if (v == nullptr)
    {
        deleteTree(du);
        return nullptr;
    }

    ASTNode *mulNode_left = new ASTNode({TokenType::Operator, "*", MathFunc::None});
    mulNode_left->left = copyTree(u);
    mulNode_left->right = copyTree(v);

    ASTNode *mulNode_right = new ASTNode({TokenType::Operator, "*", MathFunc::None});
    mulNode_right->left = copyTree(v);
    mulNode_right->right = copyTree(du);

    mulNode_right = simplify(mulNode_right);

    ASTNode *integration_vdu = integrate(mulNode_right, depth + 1);
    if (integration_vdu == nullptr)
    {
        deleteTree(mulNode_left);
        deleteTree(du);
        deleteTree(v);
        deleteTree(mulNode_right);
        return nullptr;
    }

    ASTNode *subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
    subNode->left = mulNode_left;
    subNode->right = integration_vdu;

    deleteTree(du);
    deleteTree(v);
    return subNode;
}

ASTNode *linearityIntegral(ASTNode *node, int depth)
{
    if (node == nullptr || node->token.type != TokenType::Operator)
    {
        return nullptr;
    }
    if (node->token.value == "+" || node->token.value == "-")
    {
        // ⚡ 修正： depth + 1
        ASTNode *leftIntegral = integrate(node->left, depth + 1);
        ASTNode *rightIntegral = integrate(node->right, depth + 1);
        if (leftIntegral != nullptr && rightIntegral != nullptr)
        {
            ASTNode *addSubNode = new ASTNode({TokenType::Operator, node->token.value, MathFunc::None});
            addSubNode->left = leftIntegral;
            addSubNode->right = rightIntegral;
            return addSubNode;
        }
        return nullptr;
    }
    if (node->token.value == "*")
    {
        bool leftIsNumber = (node->left && node->left->token.type == TokenType::Number);
        bool rightIsNumber = (node->right && node->right->token.type == TokenType::Number);
        if (leftIsNumber && !rightIsNumber)
        {
            // ⚡ 修正： depth + 1
            ASTNode *rightIntegral = integrate(node->right, depth + 1);
            if (rightIntegral != nullptr)
            {
                ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                mulNode->left = copyTree(node->left);
                mulNode->right = rightIntegral;
                return mulNode;
            }
        }
        else if (!leftIsNumber && rightIsNumber)
        {
            // ⚡ 修正： depth + 1
            ASTNode *leftIntegral = integrate(node->left, depth + 1);
            if (leftIntegral != nullptr)
            {
                ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                mulNode->left = leftIntegral;
                mulNode->right = copyTree(node->right);
                return mulNode;
            }
        }
    }
    return nullptr;
}
ASTNode *tableIntegral(ASTNode *node)
{
    if (!node)
        return nullptr;
    if (node->token.type == TokenType::Constant)
    {
        // ∫ e dx = e * x
        ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        mulNode->left = copyTree(node); // 複製那個 "e"
        mulNode->right = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        return mulNode;
    }
    if (node->token.type == TokenType::Variable && node->token.value == "x")
    {
        ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode *half = new ASTNode({TokenType::Number, "0.5", MathFunc::None}); // 或寫成 "1/2" 如果你的簡化器支援
        ASTNode *powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        ASTNode *xNode = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        ASTNode *two = new ASTNode({TokenType::Number, "2", MathFunc::None});
        powNode->left = xNode;
        powNode->right = two;
        mulNode->left = half;
        mulNode->right = powNode;
        return mulNode;
    }
    if (node->token.type == TokenType::Operator && node->token.value == "^")
    {
        if (node->left && (node->left->token.value == "e" || node->left->token.value == "E") &&
            node->right && node->right->token.value == "x")
        {
            return copyTree(node);
        }

        if (node->left && node->left->token.type == TokenType::Variable && node->right && node->right->token.type == TokenType::Number)
        {
            if (node->right->token.value == "-1")
            {
                ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                lnNode->right = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
                lnNode->right->right = copyTree(node->left);
                return lnNode;
            } // ∫1/x dx = ln|x|
            else
            {
                ASTNode *mulNode = new ASTNode({TokenType::Operator, "*"});
                ASTNode *divNode = new ASTNode({TokenType::Operator, "/"});
                ASTNode *powNode = new ASTNode({TokenType::Operator, "^"});
                ASTNode *oneNode = new ASTNode({TokenType::Number, "1"});
                string n_str = node->right->token.value;
                double n_num = stod(n_str);
                double n_num_plus_one = n_num + 1;
                n_str = formatDouble(n_num);
                string n_plus_1_str = formatDouble(n_num_plus_one);
                ASTNode *N_plus_one_Node_1 = new ASTNode({TokenType::Number, n_plus_1_str});
                ASTNode *N_plus_one_Node_2 = new ASTNode({TokenType::Number, n_plus_1_str});
                mulNode->left = divNode;
                mulNode->right = powNode;
                divNode->left = oneNode;
                divNode->right = N_plus_one_Node_1;
                powNode->left = copyTree(node->left);
                powNode->right = N_plus_one_Node_2;
                return mulNode;
            } // ∫x^n dx = (1/(n+1)) * x^(n+1) (n != -1)
        }
        // 處理運算子 ^ 的情況
        if (node->token.type == TokenType::Operator && node->token.value == "^")
        {
            bool leftIsNum = (node->left && node->left->token.type == TokenType::Number);
            if (leftIsNum)
            {
                bool rightIsX = (node->right && node->right->token.type == TokenType::Variable && node->right->token.value == "x");
                bool rightIsLinear = false;
                double b_val = 1.0;
                if (node->right && node->right->token.type == TokenType::Operator && node->right->token.value == "*")
                {
                    ASTNode *rLeft = node->right->left;
                    ASTNode *rRight = node->right->right;
                    if (rLeft && rLeft->token.type == TokenType::Number &&
                        rRight && rRight->token.type == TokenType::Variable && rRight->token.value == "x")
                    {
                        rightIsLinear = true;
                        b_val = stod(rLeft->token.value);
                    }
                    else if (rRight && rRight->token.type == TokenType::Number &&
                             rLeft && rLeft->token.type == TokenType::Variable && rLeft->token.value == "x")
                    {
                        rightIsLinear = true;
                        b_val = stod(rRight->token.value);
                    }
                }
                if (rightIsX || rightIsLinear)
                {
                    // 建構核心公式: a^(bx) / ln(a)
                    ASTNode *divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                    ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
                    divNode->left = copyTree(node);
                    lnNode->right = copyTree(node->left);
                    divNode->right = lnNode;
                    if (rightIsLinear && b_val != 1.0 && b_val != 0.0)
                    {
                        double reciprocal = 1.0 / b_val;
                        ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                        ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                        outerMul->left = coefNode;
                        outerMul->right = divNode;
                        return outerMul;
                    }
                    return divNode;
                }
            }
        }
    }
    if (node->token.type == TokenType::Function)
    {
        double a = 1.0; // 準備用來裝常數倍率
        if (!isLinearX(node->right, a))
        {
            return nullptr;
        }
        switch (node->token.funcType)
        {
        case MathFunc::sin:
        {
            // int sin(x) dx = -1 * cos(x)
            ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *minusOne = new ASTNode({TokenType::Number, "-1", MathFunc::None});
            ASTNode *cosNode = new ASTNode({TokenType::Function, "cos", MathFunc::cos});
            cosNode->right = copyTree(node->right);
            mulNode->left = minusOne;
            mulNode->right = cosNode;
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = mulNode;
                return outerMul;
            }
            return mulNode;
        }
        case MathFunc::cos:
        {
            // int cos(x) dx = sin(x)
            ASTNode *sinNode = new ASTNode({TokenType::Function, "sin", MathFunc::sin});
            sinNode->right = copyTree(node->right);
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = sinNode;
                return outerMul;
            }
            return sinNode;
        }
        case MathFunc::tan:
        {
            ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
            ASTNode *absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
            ASTNode *secNode = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
            lnNode->right = absNode;
            absNode->right = secNode;
            secNode->right = copyTree(node->right);
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = lnNode;
                return outerMul;
            }
            return lnNode;
        }
        case MathFunc::cot:
        {
            ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
            ASTNode *absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
            ASTNode *sinNode = new ASTNode({TokenType::Function, "sin", MathFunc::sin});
            lnNode->right = absNode;
            absNode->right = sinNode;
            sinNode->right = copyTree(node->right);
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = lnNode;
                return outerMul;
            }
            return lnNode;
        }
        case MathFunc::sec:
        {
            ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
            ASTNode *absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
            ASTNode *secNode = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
            ASTNode *tanNode = new ASTNode({TokenType::Function, "tan", MathFunc::tan});
            ASTNode *plusNode = new ASTNode({TokenType::Operator, "+"});
            lnNode->right = absNode;
            absNode->right = plusNode;
            plusNode->left = secNode;
            plusNode->right = tanNode;
            secNode->right = copyTree(node->right);
            tanNode->right = copyTree(node->right);
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = lnNode;
                return outerMul;
            }
            return lnNode;
        }
        case MathFunc::csc:
        {
            ASTNode *minusOne = new ASTNode({TokenType::Number, "-1", MathFunc::None});
            ASTNode *mulNode = new ASTNode({TokenType::Operator, "*"});
            ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
            ASTNode *absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
            ASTNode *cscNode = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
            ASTNode *cotNode = new ASTNode({TokenType::Function, "cot", MathFunc::cot});
            ASTNode *plusNode = new ASTNode({TokenType::Operator, "+"});
            mulNode->left = minusOne;
            mulNode->right = lnNode;
            lnNode->right = absNode;
            absNode->right = plusNode;
            plusNode->left = cscNode;
            plusNode->right = cotNode;
            cscNode->right = copyTree(node->right);
            cotNode->right = copyTree(node->right);
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = mulNode;
                return outerMul;
            }
            return mulNode;
        }
        case MathFunc::ln:
        {
            ASTNode *subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
            subNode->left = mulNode;
            subNode->right = copyTree(node->right);
            mulNode->left = copyTree(node->right);
            mulNode->right = lnNode;
            lnNode->right = copyTree(node->right);
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = subNode;
                return outerMul;
            }
            return subNode;
        }
        case MathFunc::log:
        {
            ASTNode *lnNode_out = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
            ASTNode *divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            ASTNode *oneNode = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *tenNode = new ASTNode({TokenType::Number, "10", MathFunc::None});
            ASTNode *subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *mulNode_out = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
            mulNode_out->left = divNode;
            mulNode_out->right = subNode;
            divNode->left = oneNode;
            divNode->right = lnNode;
            lnNode->right = tenNode;
            subNode->left = mulNode;
            subNode->right = copyTree(node->right);
            mulNode->left = copyTree(node->right);
            mulNode->right = lnNode;
            lnNode->right = copyTree(node->right);
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = subNode;
                return outerMul;
            }
            return subNode;
        }
        case MathFunc::arcsin:
        {
            ASTNode *plusNode = new ASTNode({TokenType::Operator, "+", MathFunc::None});
            ASTNode *powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            ASTNode *arcsinNode = new ASTNode({TokenType::Function, "arcsin", MathFunc::arcsin});
            ASTNode *oneNode = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *halfNode = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
            ASTNode *twoNode = new ASTNode({TokenType::Number, "2", MathFunc::None});
            plusNode->left = mulNode;
            plusNode->right = powNode_1;
            mulNode->left = copyTree(node->right);
            mulNode->right = arcsinNode;
            arcsinNode->right = copyTree(node->right);
            powNode_1->left = subNode;
            powNode_1->right = halfNode;
            subNode->left = oneNode;
            subNode->right = powNode_2;
            powNode_2->left = copyTree(node->right);
            powNode_2->right = twoNode;
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = plusNode;
                return outerMul;
            }
            return plusNode;
        }
        case MathFunc::arccos:
        {
            ASTNode *subNode_1 = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            ASTNode *powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *subNode_2 = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            ASTNode *arccosNode = new ASTNode({TokenType::Function, "arccos", MathFunc::arccos});
            ASTNode *oneNode = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *halfNode = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
            ASTNode *twoNode = new ASTNode({TokenType::Number, "2", MathFunc::None});
            subNode_1->left = mulNode;
            subNode_1->right = powNode_1;
            mulNode->left = copyTree(node->right);
            mulNode->right = arccosNode;
            arccosNode->right = copyTree(node->right);
            powNode_1->left = subNode_2;
            powNode_1->right = halfNode;
            subNode_2->left = oneNode;
            subNode_2->right = powNode_2;
            powNode_2->left = copyTree(node->right);
            powNode_2->right = twoNode;
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = subNode_1;
                return outerMul;
            }
            return subNode_1;
        }
        case MathFunc::arctan:
        {
            ASTNode *subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            ASTNode *powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *mulNode_left = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *mulNode_right = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *plusNode = new ASTNode({TokenType::Operator, "+", MathFunc::None});
            ASTNode *arctanNode = new ASTNode({TokenType::Function, "arctan", MathFunc::arctan});
            ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
            ASTNode *oneNode = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *halfNode = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
            ASTNode *twoNode = new ASTNode({TokenType::Number, "2", MathFunc::None});
            subNode->left = mulNode_left;
            subNode->right = mulNode_right;
            mulNode_left->left = copyTree(node->right);
            mulNode_left->right = arctanNode;
            arctanNode->right = copyTree(node->right);
            mulNode_right->left = halfNode;
            mulNode_right->right = lnNode;
            lnNode->right = plusNode;
            plusNode->left = oneNode;
            plusNode->right = powNode;
            powNode->left = copyTree(node->right);
            powNode->right = twoNode;
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = subNode;
                return outerMul;
            }
            return subNode;
        }
        case MathFunc::arccot:
        {
            ASTNode *plusNode_out = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            ASTNode *powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *mulNode_left = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *mulNode_right = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *plusNode = new ASTNode({TokenType::Operator, "+", MathFunc::None});
            ASTNode *arccotNode = new ASTNode({TokenType::Function, "arccot", MathFunc::arccot});
            ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
            ASTNode *oneNode = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *halfNode = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
            ASTNode *twoNode = new ASTNode({TokenType::Number, "2", MathFunc::None});
            plusNode_out->left = mulNode_left;
            plusNode_out->right = mulNode_right;
            mulNode_left->left = copyTree(node->right);
            mulNode_left->right = arccotNode;
            arccotNode->right = copyTree(node->right);
            mulNode_right->left = halfNode;
            mulNode_right->right = lnNode;
            lnNode->right = plusNode;
            plusNode->left = oneNode;
            plusNode->right = powNode;
            powNode->left = copyTree(node->right);
            powNode->right = twoNode;
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = plusNode_out;
                return outerMul;
            }
            return plusNode_out;
        }
        case MathFunc::arcsec:
        {
            ASTNode *subNode_one = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            ASTNode *subNode_two = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            ASTNode *powNode_one = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *powNode_two = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *plusNode = new ASTNode({TokenType::Operator, "+", MathFunc::None});
            ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *arcsecNode = new ASTNode({TokenType::Function, "arcsec", MathFunc::arcsec});
            ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
            ASTNode *absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
            ASTNode *oneNode = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *halfNode = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
            ASTNode *twoNode = new ASTNode({TokenType::Number, "2", MathFunc::None});
            subNode_one->left = mulNode;
            subNode_one->right = lnNode;
            mulNode->left = copyTree(node->right);
            mulNode->right = arcsecNode;
            arcsecNode->right = copyTree(node->right);
            lnNode->right = absNode;
            absNode->right = plusNode;
            plusNode->left = copyTree(node->right);
            plusNode->right = powNode_one;
            powNode_one->left = subNode_two;
            powNode_one->right = halfNode;
            subNode_two->left = powNode_two;
            subNode_two->right = oneNode;
            powNode_two->left = copyTree(node->right);
            powNode_two->right = twoNode;
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = subNode_one;
                return outerMul;
            }
            return subNode_one;
        }
        case MathFunc::arccsc:
        {
            ASTNode *plusNode_one = new ASTNode({TokenType::Operator, "+", MathFunc::None});
            ASTNode *subNode_two = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            ASTNode *powNode_one = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *powNode_two = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *plusNode = new ASTNode({TokenType::Operator, "+", MathFunc::None});
            ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *arccscNode = new ASTNode({TokenType::Function, "arccsc", MathFunc::arccsc});
            ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", MathFunc::ln});
            ASTNode *absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
            ASTNode *oneNode = new ASTNode({TokenType::Number, "1", MathFunc::None});
            ASTNode *halfNode = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
            ASTNode *twoNode = new ASTNode({TokenType::Number, "2", MathFunc::None});
            plusNode_one->left = mulNode;
            plusNode_one->right = lnNode;
            mulNode->left = copyTree(node->right);
            mulNode->right = arccscNode;
            arccscNode->right = copyTree(node->right);
            lnNode->right = absNode;
            absNode->right = plusNode;
            plusNode->left = copyTree(node->right);
            plusNode->right = powNode_one;
            powNode_one->left = subNode_two;
            powNode_one->right = halfNode;
            subNode_two->left = powNode_two;
            subNode_two->right = oneNode;
            powNode_two->left = copyTree(node->right);
            powNode_two->right = twoNode;
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = plusNode_one;
                return outerMul;
            }
            return plusNode_one;
        }
        case MathFunc::abs:
        {
            ASTNode *mulNode1 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *mulNode2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            // 建立 0.5
            ASTNode *halfNode = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
            // 建立 x (或是內部的變數 u)
            ASTNode *uNode = copyTree(node->right);
            // 建立 |x|
            ASTNode *absNode = new ASTNode({TokenType::Function, "abs", MathFunc::abs});
            absNode->right = copyTree(node->right);
            // 組裝: 0.5 * u * |u|
            mulNode2->left = uNode;
            mulNode2->right = absNode;
            mulNode1->left = halfNode;
            mulNode1->right = mulNode2;
            // 如果有常數倍率 (例如 ∫ |2x| dx)，套用你原本寫好的常數補償
            if (a != 1.0)
            {
                double reciprocal = 1.0 / a;
                ASTNode *outerMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *coefNode = new ASTNode({TokenType::Number, formatDouble(reciprocal), MathFunc::None});
                outerMul->left = coefNode;
                outerMul->right = mulNode1;
                return outerMul;
            }
            return mulNode1;
        }
        default:
            break; // 如果是查表法無法直接積的函數，跳出 switch
        }
    }
    return nullptr; // 目前先回傳 nullptr，表示沒有找到對應的積分結果
}
// 🌟 強化版 LIATE 法則評分系統
// ==========================================
// 🌟 修正 LIATE 評分：將 E (指數) 從 A (代數) 中分離！
// ==========================================

struct ExpTrigMatch
{
    // --- 掃描過程追蹤用 ---
    bool hasExp = false;  // 找到 e 的指數了嗎？
    bool hasTrig = false; // 找到三角函數了嗎？
    // --- 最終結果回報用 ---
    bool isMatched = false; // (如果 hasExp 和 hasTrig 都為 true，這個才會變 true)
    double a = 1.0;         // e^(ax) 的 a，預設為 1
    double b = 1.0;         // sin(bx)/cos(bx) 的 b，預設為 1
    bool isSin = true;      // true 代表抓到 sin，false 代表抓到 cos
};
// 專門用來從 "3*x" 或 "x" 中挖出數字 3 的幫手
double extractCoefficient(ASTNode *node)
{
    if (node == nullptr)
        return 1.0; // 防呆，預設為 1
    // 狀況 1: 只有 "x" (例如 e^x) -> 係數是 1
    if (node->token.type == TokenType::Variable && node->token.value == "x")
    {
        return 1.0;
    }
    // 狀況 2: 發現乘法 "*" (例如 3*x 或 x*3)
    if (node->token.type == TokenType::Operator && node->token.value == "*")
    {
        // 檢查左邊是不是數字，右邊是不是 x
        if (node->left && node->left->token.type == TokenType::Number &&
            node->right && node->right->token.value == "x")
        {
            return std::stod(node->left->token.value); // stod: String TO Double
        }
        // 檢查右邊是不是數字，左邊是不是 x
        if (node->right && node->right->token.type == TokenType::Number &&
            node->left && node->left->token.value == "x")
        {
            return std::stod(node->right->token.value);
        }
    }
    // 如果長得太複雜（例如 x^2），我們這裡先預設回傳 1
    // (未來可以擴充更強大的代數化簡)
    return 1.0;
}
ExpTrigMatch matchExpTrigPattern(ASTNode *node)
{
    ExpTrigMatch result;
    // 必須是最頂層的乘法節點
    if (node == nullptr || node->token.value != "*")
        return result;
    ASTNode *leftChild = node->left;
    ASTNode *rightChild = node->right;
    // 嚴格檢查是否為 e^(ax) * sin(bx) 或是 sin(bx) * e^(ax) 的結構
    auto checkExpTrig = [&](ASTNode *expNode, ASTNode *trigNode)
    {
        // 檢查一邊是否為 e^...
        if (expNode->token.value == "^" && expNode->left && expNode->left->token.value == "e")
        {
            // 檢查另一邊是否為「乾淨的」 sin 或 cos (不能有次方包著它！)
            if (trigNode->token.type == TokenType::Function &&
                (trigNode->token.funcType == MathFunc::sin || trigNode->token.funcType == MathFunc::cos))
            {
                result.hasExp = true;
                result.hasTrig = true;
                result.a = extractCoefficient(expNode->right);
                result.b = extractCoefficient(trigNode->right);
                result.isSin = (trigNode->token.funcType == MathFunc::sin);
                result.isMatched = true;
            }
        }
    };
    checkExpTrig(leftChild, rightChild);
    if (!result.isMatched)
        checkExpTrig(rightChild, leftChild);
    return result;
}
ASTNode *buildExpTrigResult(ExpTrigMatch match)
{
    if (match.isSin)
    {
        ASTNode *divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
        ASTNode *mulNode_1 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode *mulNode_2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode *mulNode_3 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode *mulNode_4 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode *mulNode_5 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode *mulNode_6 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode *plusNode_1 = new ASTNode({TokenType::Operator, "+", MathFunc::None});
        ASTNode *subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
        ASTNode *powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        ASTNode *powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        ASTNode *powNode_3 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        ASTNode *sinNode = new ASTNode({TokenType::Function, "sin", MathFunc::sin});
        ASTNode *cosNode = new ASTNode({TokenType::Function, "cos", MathFunc::cos});
        ASTNode *eNode = new ASTNode({TokenType::Constant, "e", MathFunc::None});
        ASTNode *aNode_1 = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
        ASTNode *aNode_2 = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
        ASTNode *aNode_3 = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
        ASTNode *bNode_1 = new ASTNode({TokenType::Number, formatDouble(match.b), MathFunc::None});
        ASTNode *bNode_2 = new ASTNode({TokenType::Number, formatDouble(match.b), MathFunc::None});
        ASTNode *bNode_3 = new ASTNode({TokenType::Number, formatDouble(match.b), MathFunc::None});
        ASTNode *bNode_4 = new ASTNode({TokenType::Number, formatDouble(match.b), MathFunc::None});
        ASTNode *twoNode_1 = new ASTNode({TokenType::Number, "2", MathFunc::None});
        ASTNode *twoNode_2 = new ASTNode({TokenType::Number, "2", MathFunc::None});
        ASTNode *xNode_1 = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        ASTNode *xNode_2 = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        ASTNode *xNode_3 = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        divNode->left = mulNode_1;
        divNode->right = plusNode_1;
        mulNode_1->left = powNode_1;
        mulNode_1->right = subNode;
        powNode_1->left = eNode;
        powNode_1->right = mulNode_2;
        mulNode_2->left = aNode_1;
        mulNode_2->right = xNode_1;
        subNode->left = mulNode_3;
        subNode->right = mulNode_4;
        mulNode_3->left = aNode_2;
        mulNode_3->right = sinNode;
        sinNode->right = mulNode_5;
        mulNode_5->left = bNode_1;
        mulNode_5->right = xNode_2;
        mulNode_4->left = bNode_2;
        mulNode_4->right = cosNode;
        cosNode->right = mulNode_6;
        mulNode_6->left = bNode_3;
        mulNode_6->right = xNode_3;
        plusNode_1->left = powNode_2;
        plusNode_1->right = powNode_3;
        powNode_2->left = aNode_3;
        powNode_2->right = twoNode_1;
        powNode_3->left = bNode_4;
        powNode_3->right = twoNode_2;
        return divNode;
    }
    else
    {
        ASTNode *divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
        ASTNode *mulNode_1 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode *mulNode_2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode *mulNode_3 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode *mulNode_4 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode *mulNode_5 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode *mulNode_6 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        ASTNode *plusNode_1 = new ASTNode({TokenType::Operator, "+", MathFunc::None});
        ASTNode *plusNode_2 = new ASTNode({TokenType::Operator, "+", MathFunc::None});
        ASTNode *powNode_1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        ASTNode *powNode_2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        ASTNode *powNode_3 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        ASTNode *sinNode = new ASTNode({TokenType::Function, "sin", MathFunc::sin});
        ASTNode *cosNode = new ASTNode({TokenType::Function, "cos", MathFunc::cos});
        ASTNode *eNode = new ASTNode({TokenType::Constant, "e", MathFunc::None});
        ASTNode *aNode_1 = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
        ASTNode *aNode_2 = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
        ASTNode *aNode_3 = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
        ASTNode *bNode_1 = new ASTNode({TokenType::Number, formatDouble(match.b), MathFunc::None});
        ASTNode *bNode_2 = new ASTNode({TokenType::Number, formatDouble(match.b), MathFunc::None});
        ASTNode *bNode_3 = new ASTNode({TokenType::Number, formatDouble(match.b), MathFunc::None});
        ASTNode *bNode_4 = new ASTNode({TokenType::Number, formatDouble(match.b), MathFunc::None});
        ASTNode *twoNode_1 = new ASTNode({TokenType::Number, "2", MathFunc::None});
        ASTNode *twoNode_2 = new ASTNode({TokenType::Number, "2", MathFunc::None});
        ASTNode *xNode_1 = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        ASTNode *xNode_2 = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        ASTNode *xNode_3 = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        divNode->left = mulNode_1;
        divNode->right = plusNode_1;
        mulNode_1->left = powNode_1;
        mulNode_1->right = plusNode_2;
        powNode_1->left = eNode;
        powNode_1->right = mulNode_2;
        mulNode_2->left = aNode_1;
        mulNode_2->right = xNode_1;
        plusNode_2->left = mulNode_3;
        plusNode_2->right = mulNode_4;
        mulNode_3->left = aNode_2;
        mulNode_3->right = cosNode;
        cosNode->right = mulNode_5;
        mulNode_5->left = bNode_1;
        mulNode_5->right = xNode_2;
        mulNode_4->left = bNode_2;
        mulNode_4->right = sinNode;
        sinNode->right = mulNode_6;
        mulNode_6->left = bNode_3;
        mulNode_6->right = xNode_3;
        plusNode_1->left = powNode_2;
        plusNode_1->right = powNode_3;
        powNode_2->left = aNode_3;
        powNode_2->right = twoNode_1;
        powNode_3->left = bNode_4;
        powNode_3->right = twoNode_2;
        return divNode;
    }
}
ASTNode *multiplyByConstant(ASTNode *resultNode, double k)
{
    if (k == 1.0)
        return resultNode; // 1倍就不用乘了，直接回傳
    ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
    // 記得你有 formatDouble 可以把 double 轉回漂亮的字串
    mulNode->left = new ASTNode({TokenType::Number, formatDouble(k), MathFunc::None});
    mulNode->right = resultNode;
    return mulNode;
}
void extractConstant(ASTNode *node, double &out_const, ASTNode *&out_base)
{
    out_const = 1.0;
    out_base = node;
    if (!node)
        return;
    if (node->token.type == TokenType::Operator && node->token.value == "*")
    {
        if (node->left->token.type == TokenType::Number)
        {
            out_const = std::stod(node->left->token.value);
            out_base = node->right;
        }
        else if (node->right->token.type == TokenType::Number)
        {
            out_const = std::stod(node->right->token.value);
            out_base = node->left;
        }
    }
    else if (node->token.type == TokenType::Number)
    {
        out_const = std::stod(node->token.value);
        out_base = nullptr;
    }
}

// ================== normalizeRational 實作 ==================
ASTNode *normalizeRational(ASTNode *node)
{
    if (!node || node->token.value != "/")
        return node;

    // 1. 收集分子與分母的因子
    std::vector<ASTNode *> num_factors;
    std::vector<ASTNode *> den_factors;
    collectFactors(node->left, num_factors);
    collectFactors(node->right, den_factors);

    double final_coeff = 1.0;
    struct BasePower
    {
        ASTNode *base;
        double power;
    };
    std::vector<BasePower> merged;

    // 2. 處理分子（係數相乘，次方相加）
    for (ASTNode *f : num_factors)
    {
        if (f->token.type == TokenType::Number)
        {
            final_coeff *= std::stod(f->token.value);
        }
        else
        {
            ASTNode *base = f;
            double pow = 1.0;
            if (f->token.value == "^" && f->right && f->right->token.type == TokenType::Number)
            {
                base = f->left;
                pow = std::stod(f->right->token.value);
            }
            bool found = false;
            for (auto &m : merged)
            {
                if (isSameTree(m.base, base))
                {
                    m.power += pow;
                    found = true;
                    break;
                }
            }
            if (!found)
                merged.push_back({copyTree(base), pow});
        }
    }

    // 3. 處理分母（係數相除，次方相減）
    for (ASTNode *f : den_factors)
    {
        if (f->token.type == TokenType::Number)
        {
            double v = std::stod(f->token.value);
            if (std::abs(v) > 1e-9)
                final_coeff /= v;
        }
        else
        {
            ASTNode *base = f;
            double pow = 1.0;
            if (f->token.value == "^" && f->right && f->right->token.type == TokenType::Number)
            {
                base = f->left;
                pow = std::stod(f->right->token.value);
            }
            bool found = false;
            for (auto &m : merged)
            {
                if (isSameTree(m.base, base))
                {
                    m.power -= pow;
                    found = true;
                    break;
                }
            }
            if (!found)
                merged.push_back({copyTree(base), -pow});
        }
    }

    // 4. 重組分子與分母的因子
    std::vector<ASTNode *> n_list, d_list;
    if (std::abs(final_coeff - 1.0) > 1e-9 || merged.empty())
    {
        n_list.push_back(new ASTNode({TokenType::Number, formatDouble(final_coeff), MathFunc::None}));
    }

    for (auto &m : merged)
    {
        if (std::abs(m.power) < 1e-9)
            continue;
        ASTNode *term = nullptr;
        if (std::abs(m.power - 1.0) < 1e-9)
        {
            term = copyTree(m.base);
        }
        else
        {
            term = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            term->left = copyTree(m.base);
            term->right = new ASTNode({TokenType::Number, formatDouble(std::abs(m.power)), MathFunc::None});
        }
        if (m.power > 0)
            n_list.push_back(term);
        else
            d_list.push_back(term);
    }

    ASTNode *numPart = rebuildProduct(n_list);
    ASTNode *denPart = d_list.empty() ? new ASTNode({TokenType::Number, "1", MathFunc::None}) : rebuildProduct(d_list);

    // 清理暫時的因子列表（避免記憶體洩漏）
    for (ASTNode *f : n_list)
        deleteTree(f);
    for (ASTNode *f : d_list)
        deleteTree(f);
    for (auto &m : merged)
        deleteTree(m.base);

    ASTNode *result = new ASTNode({TokenType::Operator, "/", MathFunc::None});
    result->left = numPart;
    result->right = denPart;
    return result;
}

// ================== 補齊缺失的輔助函數 ==================

// 1. 將 AST 中的 x 替換為變數名稱 "u"（如果可能）
ASTNode *convertToUPoly(ASTNode *R_x, ASTNode *u)
{
    if (!R_x)
        return nullptr;

    // 沒有 x 的節點（常數）直接複製
    if (!containsVariable(R_x, "x"))
    {
        return copyTree(R_x);
    }

    // 如果整個 R_x 結構與 u 完全相同 → 變成變數 "u"
    if (isSameTree(R_x, u))
    {
        return new ASTNode({TokenType::Variable, "u", MathFunc::None});
    }

    // 遞迴處理運算子節點
    if (R_x->token.type == TokenType::Operator)
    {
        ASTNode *new_left = convertToUPoly(R_x->left, u);
        ASTNode *new_right = convertToUPoly(R_x->right, u);
        bool left_ok = (R_x->left == nullptr) || (new_left != nullptr);
        bool right_ok = (R_x->right == nullptr) || (new_right != nullptr);
        if (left_ok && right_ok)
        {
            ASTNode *new_node = new ASTNode(R_x->token);
            new_node->left = new_left;
            new_node->right = new_right;
            return new_node;
        }
        // 轉換失敗，清理
        if (new_left)
            deleteTree(new_left);
        if (new_right)
            deleteTree(new_right);
    }
    return nullptr;
}

// 2. 將 AST 中所有指定變數名稱替換為另一個名稱（單純字串替換，不複製子樹）
void replaceVariable(ASTNode *node, const std::string &oldVar, const std::string &newVar)
{
    if (!node)
        return;
    if (node->token.type == TokenType::Variable && node->token.value == oldVar)
    {
        node->token.value = newVar;
    }
    replaceVariable(node->left, oldVar, newVar);
    replaceVariable(node->right, oldVar, newVar);
}

// 3. 將 AST 中所有 targetVar 變數替換為 uNode 的副本（會複製整個子樹）
ASTNode *substituteVariableBack(ASTNode *node, const std::string &targetVar, ASTNode *uNode)
{
    if (!node)
        return nullptr;

    // 遞迴處理子樹
    node->left = substituteVariableBack(node->left, targetVar, uNode);
    node->right = substituteVariableBack(node->right, targetVar, uNode);

    // 如果當前節點是目標變數，則替換為 uNode 的副本
    if (node->token.type == TokenType::Variable && node->token.value == targetVar)
    {
        delete node;
        return copyTree(uNode);
    }
    return node;
}

ASTNode *performSubstitution(ASTNode *f_g_node, ASTNode *remain_node, ASTNode *&out_u)
{
    if (!f_g_node || !remain_node)
    {
        return nullptr;
    }
    // 1. 找出內層 u
    ASTNode *u = nullptr;
    if (f_g_node->token.value == "^" && f_g_node->left && f_g_node->left->token.value == "e")
    {
        u = f_g_node->right;
    }
    else if (f_g_node->token.type == TokenType::Function)
    {
        u = f_g_node->right;
    }
    else if (f_g_node->token.value == "^" && f_g_node->right && f_g_node->right->token.type == TokenType::Number)
    {
        u = f_g_node->left;
    }
    else
    {
        return nullptr;
    }
    if (!u)
    {
        return nullptr;
    }
    if (u->token.type == TokenType::Variable && u->token.value == "x")
    {
        return nullptr;
    }

    // 2. 計算 du/dx
    ASTNode *du_raw = derivative(copyTree(u));
    if (!du_raw)
    {
        return nullptr;
    }
    ASTNode *du = copyTree(du_raw);
    deleteTree(du_raw);
    if (!du)
    {
        return nullptr;
    }

    // 3. 建立 remain_node / du 並正規化
    ASTNode *div_raw = new ASTNode({TokenType::Operator, "/", MathFunc::None});
    div_raw->left = copyTree(remain_node);
    div_raw->right = copyTree(du);
    ASTNode *norm = normalizeRational(div_raw);
    if (!norm)
    {
        deleteTree(div_raw);
        deleteTree(du);
        return nullptr;
    }
    ASTNode *R_x = simplify(norm);
    deleteTree(div_raw);
    deleteTree(du);
    if (!R_x)
    {
        return nullptr;
    }

    // 4. 嘗試將 R_x 轉換為 u 的多項式
    ASTNode *R_u = convertToUPoly(R_x, u);
    deleteTree(R_x);
    if (!R_u)
    {
        return nullptr;
    }

    // 5. 將所有「u」偽裝成「x」
    replaceVariable(R_u, "u", "x");

    // 6. 建構新的 f(u) 並以 x 為變數
    ASTNode *f_u = nullptr;
    if (f_g_node->token.value == "^" && f_g_node->left && f_g_node->left->token.value == "e")
    {
        f_u = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        f_u->left = new ASTNode({TokenType::Constant, "e", MathFunc::None});
        f_u->right = new ASTNode({TokenType::Variable, "x", MathFunc::None});
    }
    else if (f_g_node->token.type == TokenType::Function)
    {
        f_u = new ASTNode(f_g_node->token);
        f_u->right = new ASTNode({TokenType::Variable, "x", MathFunc::None});
    }
    else if (f_g_node->token.value == "^" && f_g_node->right && f_g_node->right->token.type == TokenType::Number)
    {
        f_u = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        f_u->left = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        f_u->right = copyTree(f_g_node->right);
    }
    else
    {
        deleteTree(R_u);
        return nullptr;
    }

    // 7. 組合新積分式
    ASTNode *newIntegral = new ASTNode({TokenType::Operator, "*", MathFunc::None});
    newIntegral->left = R_u;
    newIntegral->right = f_u;

    // 8. 記錄原本的 u
    out_u = copyTree(u);

    ASTNode *simplified = simplify(newIntegral);
    return simplified;
}

// ===========================================================================
// 🌟 1. 三角代換專用資料結構與基礎雷達
// ===========================================================================
enum class TrigSubType
{
    None,
    Sine,
    Tangent,
    Secant
};

struct TrigSubMatch
{
    TrigSubType type = TrigSubType::None;
    double a = 0.0;
};

// ===========================================================================
// 🌟 1. 辨識 a^2 - x^2, x^2 - a^2, a^2 + x^2 結構
// ===========================================================================
// ===========================================================================
// 🌟 1. 【反干擾升級版】特徵辨識器 (解決 -1 + x^2 的順序盲區)
// ===========================================================================
TrigSubMatch matchTrigSubPattern(ASTNode *node)
{
    TrigSubMatch match;
    if (!node || node->token.type != TokenType::Operator)
        return match;
    if (node->token.value != "+" && node->token.value != "-")
        return match;

    double cL = 0, pL = 0, cR = 0, pR = 0;
    bool okL = parseTerm(node->left, cL, pL);
    bool okR = parseTerm(node->right, cR, pR);

    if (!okL || !okR)
        return match;

    // 🚀 核心升級：統一將 A - B 視為 A + (-B) 處理，徹底破除順序盲區！
    if (node->token.value == "-")
    {
        cR = -cR; // 把減號吸收到右邊的係數裡，一律當作加法判斷
    }

    // 情況 A：左邊是常數 (pL=0)，右邊是平方項 (pR=2)
    if (pL == 0.0 && pR == 2.0)
    {
        if (cR == 1.0 && cL > 0)
        {
            match.type = TrigSubType::Tangent;
            match.a = sqrt(cL);
        } // a^2 + x^2
        else if (cR == 1.0 && cL < 0)
        {
            match.type = TrigSubType::Secant;
            match.a = sqrt(-cL);
        } // -a^2 + x^2
        else if (cR == -1.0 && cL > 0)
        {
            match.type = TrigSubType::Sine;
            match.a = sqrt(cL);
        } // a^2 - x^2
    }
    // 情況 B：左邊是平方項 (pL=2)，右邊是常數 (pR=0)
    else if (pL == 2.0 && pR == 0.0)
    {
        if (cL == 1.0 && cR > 0)
        {
            match.type = TrigSubType::Tangent;
            match.a = sqrt(cR);
        } // x^2 + a^2
        else if (cL == 1.0 && cR < 0)
        {
            match.type = TrigSubType::Secant;
            match.a = sqrt(-cR);
        } // x^2 - a^2
        else if (cL == -1.0 && cR > 0)
        {
            match.type = TrigSubType::Sine;
            match.a = sqrt(cR);
        } // -x^2 + a^2
    }

    return match;
}

// ===========================================================================
// 🌟 2. 【反干擾升級版】全樹掃描器 (解決 -1/2 的分數盲區)
// ===========================================================================
// ===========================================================================
// 🌟 【終極反干擾版】全樹掃描器：解除 0.5 限制，全面鎖定所有小數與負數！
// ===========================================================================
TrigSubMatch findTrigSubCandidate(ASTNode *node)
{
    if (!node)
        return TrigSubMatch();

    if (node->token.type == TokenType::Operator && node->token.value == "^" && node->right)
    {
        double p = 0;
        bool isPowerValid = false;

        // 解析純小數或整數 (例如 1.5, -2)
        if (node->right->token.type == TokenType::Number)
        {
            p = std::stod(node->right->token.value);
            isPowerValid = true;
        }
        // 解析被 Parser 當作除法處理的分數 (例如 3 / 2)
        else if (node->right->token.type == TokenType::Operator && node->right->token.value == "/")
        {
            if (node->right->left && node->right->right &&
                node->right->left->token.type == TokenType::Number &&
                node->right->right->token.type == TokenType::Number)
            {
                p = std::stod(node->right->left->token.value) / std::stod(node->right->right->token.value);
                isPowerValid = true;
            }
        }

        if (isPowerValid)
        {
            // 🚀 雷達終極升級：
            // 利用 std::round 判斷它是不是整數。
            // 只要是「小數」(例如 1.5, 0.5)，或是「負數」(例如 -1, -2)，全面啟動掃描！
            bool isFractional = (std::abs(p - std::round(p)) > 1e-9);

            if (isFractional || p < 0)
            {
                TrigSubMatch m = matchTrigSubPattern(node->left);
                if (m.type != TrigSubType::None)
                    return m;
            }
        }
    }

    TrigSubMatch mL = findTrigSubCandidate(node->left);
    if (mL.type != TrigSubType::None)
        return mL;
    return findTrigSubCandidate(node->right);
}

// ===========================================================================
// 🌟 輔助工具：字串轉 MathFunc
// ===========================================================================
MathFunc strToMathFunc(const string &name)
{
    if (name == "sin")
        return MathFunc::sin;
    if (name == "cos")
        return MathFunc::cos;
    if (name == "tan")
        return MathFunc::tan;
    if (name == "sec")
        return MathFunc::sec;
    if (name == "csc")
        return MathFunc::csc;
    if (name == "cot")
        return MathFunc::cot;
    if (name == "ln")
        return MathFunc::ln;
    if (name == "log")
        return MathFunc::log;
    if (name == "arcsin")
        return MathFunc::arcsin;
    if (name == "arccos")
        return MathFunc::arccos;
    if (name == "arctan")
        return MathFunc::arctan;
    return MathFunc::None;
}

// 零件生成器：建立 a * sin(x) 這種 AST 結構
ASTNode *buildTrigTerm(double a, const string &trigFunc)
{
    ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
    mulNode->left = new ASTNode({TokenType::Number, formatDouble(a), MathFunc::None});
    ASTNode *funcNode = new ASTNode({TokenType::Function, trigFunc, strToMathFunc(trigFunc)});
    funcNode->right = new ASTNode({TokenType::Variable, "x", MathFunc::None});
    mulNode->right = funcNode;
    return mulNode;
}

// ===========================================================================
// 🌟 2. 三角代換進階核心工具群
// ===========================================================================

// 🎯 【強化版】正向代換機：加入「原子級畢氏定理相消」
ASTNode *applyTrigSubToTree(ASTNode *node, TrigSubMatch match)
{
    if (!node)
        return nullptr;

    // 🚀 手術 2：一旦看到原本的 a^2 - x^2，不囉嗦，直接變成 a^2 * cos^2(x)
    TrigSubMatch currentMatch = matchTrigSubPattern(node);
    if (currentMatch.type == match.type && std::abs(currentMatch.a - match.a) < 1e-9)
    {
        double a_sq = match.a * match.a;
        ASTNode *aSqNode = new ASTNode({TokenType::Number, formatDouble(a_sq), MathFunc::None});

        // 判斷要相消成什麼三角函數
        string trigFunc;
        if (match.type == TrigSubType::Sine)
            trigFunc = "cos"; // 1 - sin^2 = cos^2
        else if (match.type == TrigSubType::Tangent)
            trigFunc = "sec"; // 1 + tan^2 = sec^2
        else if (match.type == TrigSubType::Secant)
            trigFunc = "tan"; // sec^2 - 1 = tan^2

        ASTNode *funcNode = new ASTNode({TokenType::Function, trigFunc, strToMathFunc(trigFunc)});
        funcNode->right = new ASTNode({TokenType::Variable, "x", MathFunc::None}); // 注意：用 x 暫代 θ

        ASTNode *trigSq = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        trigSq->left = funcNode;
        trigSq->right = new ASTNode({TokenType::Number, "2", MathFunc::None});

        ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        mulNode->left = aSqNode;
        mulNode->right = trigSq;

        deleteTree(node); // 清除舊的結構
        return mulNode;   // 華麗轉身，直接回傳相消完的結果！
    }

    if (node->token.type == TokenType::Variable && node->token.value == "x")
    {
        delete node; // 只有在沒被相消攔截到的孤立 x，才會做標準的 a*sin(x) 替換
        if (match.type == TrigSubType::Sine)
            return buildTrigTerm(match.a, "sin");
        if (match.type == TrigSubType::Tangent)
            return buildTrigTerm(match.a, "tan");
        if (match.type == TrigSubType::Secant)
            return buildTrigTerm(match.a, "sec");
    }

    node->left = applyTrigSubToTree(node->left, match);
    node->right = applyTrigSubToTree(node->right, match);
    return node;
}

// 【補齊】反向代換機
ASTNode *backSubstitute(ASTNode *node, TrigSubMatch match)
{
    if (!node)
        return nullptr;

    // 情況 A：看到孤立的變數 x (代表純 θ)，轉換成反三角函數 arcsin(x/a) 等
    if (node->token.type == TokenType::Variable && node->token.value == "x")
    {
        string invName = (match.type == TrigSubType::Sine) ? "arcsin" : ((match.type == TrigSubType::Tangent) ? "arctan" : "arcsec");
        ASTNode *invFunc = new ASTNode({TokenType::Function, invName, strToMathFunc(invName)});

        if (std::abs(match.a - 1.0) < 1e-9)
        {
            invFunc->right = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        }
        else
        {
            ASTNode *divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            divNode->left = new ASTNode({TokenType::Variable, "x", MathFunc::None});
            divNode->right = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
            invFunc->right = divNode;
        }
        delete node;
        return invFunc;
    }

    // 情況 B：看到 sin(x)，如果是 Sine 代換，直接換回 x / a
    if (node->token.type == TokenType::Function && node->token.value == "sin" &&
        node->right && node->right->token.value == "x")
    {
        if (match.type == TrigSubType::Sine)
        {
            ASTNode *resNode = nullptr;
            if (std::abs(match.a - 1.0) < 1e-9)
            {
                resNode = new ASTNode({TokenType::Variable, "x", MathFunc::None});
            }
            else
            {
                resNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                resNode->left = new ASTNode({TokenType::Variable, "x", MathFunc::None});
                resNode->right = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
            }
            deleteTree(node);
            return resNode;
        }
    }

    // 情況 C：看到 cos(x)，如果是 Sine 代換，直接換回 √(a^2 - x^2) / a
    if (node->token.type == TokenType::Function && node->token.value == "cos" &&
        node->right && node->right->token.value == "x")
    {
        if (match.type == TrigSubType::Sine)
        {
            ASTNode *divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            ASTNode *powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *subNode = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            subNode->left = new ASTNode({TokenType::Number, formatDouble(match.a * match.a), MathFunc::None});
            ASTNode *x2Node = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            x2Node->left = new ASTNode({TokenType::Variable, "x", MathFunc::None});
            x2Node->right = new ASTNode({TokenType::Number, "2", MathFunc::None});
            subNode->right = x2Node;

            powNode->left = subNode;
            powNode->right = new ASTNode({TokenType::Number, "0.5", MathFunc::None});

            if (std::abs(match.a - 1.0) < 1e-9)
            {
                deleteTree(node);
                delete divNode; // a=1 時不需要除法
                return powNode;
            }
            else
            {
                divNode->left = powNode;
                divNode->right = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
                deleteTree(node);
                return divNode;
            }
        }
    }

    node->left = backSubstitute(node->left, match);
    node->right = backSubstitute(node->right, match);
    return node;
}

// ===========================================================================
// 🌟 三角代換總指揮官：負責發動代換、掛載 dx、遞迴積分、並翻譯回現實世界
// ===========================================================================
ASTNode *tryTrigSubstitution(ASTNode *node, int depth)
{
    // 1. 雷達掃描：尋找適合的三角代換特徵
    TrigSubMatch match = findTrigSubCandidate(node);
    if (match.type == TrigSubType::None)
        return nullptr;

    // 2. 將 x 替換，並啟動原子級相消
    ASTNode *subTree = applyTrigSubToTree(copyTree(node), match);

    // 3. 🎯 根據不同代換，精準生成對應的 dx
    ASTNode *dxNode = nullptr;

    if (match.type == TrigSubType::Sine)
    {
        // Sine 代換： dx = a * cos(x)
        dxNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        dxNode->left = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
        dxNode->right = new ASTNode({TokenType::Function, "cos", MathFunc::cos});
        dxNode->right->right = new ASTNode({TokenType::Variable, "x", MathFunc::None});
    }
    else if (match.type == TrigSubType::Tangent)
    {
        // Tangent 代換： dx = a * sec^2(x)
        ASTNode *secNode = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
        secNode->right = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        ASTNode *secSq = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        secSq->left = secNode;
        secSq->right = new ASTNode({TokenType::Number, "2", MathFunc::None});

        dxNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        dxNode->left = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
        dxNode->right = secSq;
    }
    else if (match.type == TrigSubType::Secant)
    {
        // Secant 代換： dx = a * sec(x) * tan(x)
        ASTNode *secNode = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
        secNode->right = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        ASTNode *tanNode = new ASTNode({TokenType::Function, "tan", MathFunc::tan});
        tanNode->right = new ASTNode({TokenType::Variable, "x", MathFunc::None});

        ASTNode *secTan = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        secTan->left = secNode;
        secTan->right = tanNode;

        dxNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        dxNode->left = new ASTNode({TokenType::Number, formatDouble(match.a), MathFunc::None});
        dxNode->right = secTan;
    }

    // 4. 組合 integrand * dx
    ASTNode *fullIntegral = new ASTNode({TokenType::Operator, "*", MathFunc::None});
    fullIntegral->left = subTree;
    fullIntegral->right = dxNode;

    // 5. 啟動化簡，讓 dx 和分母互相廝殺相消
    fullIntegral = simplify(fullIntegral);

    // 6. 遞迴進入異世界積分
    ASTNode *result = integrate(fullIntegral, depth + 1);

    if (!result)
    {
        deleteTree(fullIntegral);
        return nullptr; // 積分失敗，撤退
    }

    // 7. 積分成功，翻譯回 x 的現實世界
    ASTNode *finalResult = backSubstitute(result, match);
    return simplify(finalResult);
}

// =======================================================
// 🌟 分數積分工具一：判斷是否為純多項式
// =======================================================
bool isPolynomial(ASTNode *node)
{
    if (node == nullptr)
        return true;
    // 遇到變數或數字，絕對是合法的多項式元素
    if (node->token.type == TokenType::Number || node->token.type == TokenType::Variable)
        return true;
    // 嚴格攔截所有特殊函數 (sin, cos, ln, e 等)
    if (node->token.type == TokenType::Function)
        return false;
    if (node->token.type == TokenType::Operator)
    {
        string op = node->token.value;
        // 加減乘都可以接受，繼續往下檢查
        if (op == "+" || op == "-" || op == "*")
        {
            return isPolynomial(node->left) && isPolynomial(node->right);
        }
        // 如果是除法 (/)，只有當分母是「純數字」時才算多項式 (例如 x/2)
        if (op == "/")
        {
            if (node->right && node->right->token.type == TokenType::Number)
            {
                return isPolynomial(node->left);
            }
            return false;
        }
        // 如果是次方 (^)，次方數必須是「非負整數」 (例如 x^2 可以，x^-1 或 x^0.5 不行)
        if (op == "^")
        {
            if (node->right && node->right->token.type == TokenType::Number)
            {
                double power = std::stod(node->right->token.value);
                if (power >= 0 && std::abs(power - std::round(power)) < 1e-9)
                {
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
void collectPolyTerms(ASTNode *node, std::map<int, double> &polyMap, double currentSign = 1.0)
{
    if (node == nullptr)
        return;
    if (node->token.type == TokenType::Operator && (node->token.value == "+" || node->token.value == "-"))
    {
        collectPolyTerms(node->left, polyMap, currentSign);
        // 如果是減號，右邊那整坨的符號都要翻轉
        double nextSign = (node->token.value == "-") ? -currentSign : currentSign;
        collectPolyTerms(node->right, polyMap, nextSign);
        return;
    }
    // 當走到最底層的單項 (例如 3*x^2 或 x 或是 5)
    double coeff = 0.0, power = 0.0;
    // 呼叫你原本寫好的解析單項工具 (parseTerm)
    if (parseTerm(node, coeff, power))
    {
        int p = std::round(power);
        polyMap[p] += (coeff * currentSign); // 將係數累加進對應的次方中
    }
}
map<int, double> astToPolyMap(ASTNode *node)
{
    map<int, double> polyMap;
    // 為了安全起見，轉換前先強制對它進行一次暴力展開與化簡
    ASTNode *expanded = expand(copyTree(node));
    ASTNode *simplified = simplify(expanded);
    collectPolyTerms(simplified, polyMap, 1.0);
    deleteTree(simplified);
    return polyMap;
}
// =======================================================
// 🌟 分數積分工具三：將多項式 Map 重新組裝回 AST 語法樹
// =======================================================
ASTNode *polyMapToAST(const std::map<int, double> &polyMap)
{
    if (polyMap.empty())
    {
        return new ASTNode({TokenType::Number, "0", MathFunc::None});
    }
    ASTNode *resultNode = nullptr;
    // 這裡我們用反向迭代器 (rbegin)，讓組裝出來的樹從最高次方開始 (例如 3x^2 + 2x + 1)
    for (auto it = polyMap.rbegin(); it != polyMap.rend(); ++it)
    {
        int power = it->first;
        double coeff = it->second;
        // 如果係數是 0，直接跳過這項
        if (std::abs(coeff) < 1e-9)
            continue;
        ASTNode *termNode = nullptr;
        // 情況 A：常數項 (power == 0)
        if (power == 0)
        {
            termNode = new ASTNode({TokenType::Number, formatDouble(std::abs(coeff)), MathFunc::None});
        }
        // 情況 B：一次項 (power == 1)
        else if (power == 1)
        {
            ASTNode *varNode = new ASTNode({TokenType::Variable, "x", MathFunc::None});
            if (std::abs(coeff) == 1.0)
            {
                termNode = varNode; // 單純的 x
            }
            else
            {
                termNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                termNode->left = new ASTNode({TokenType::Number, formatDouble(std::abs(coeff)), MathFunc::None});
                termNode->right = varNode;
            }
        }
        // 情況 C：高次項 (power > 1)
        else
        {
            ASTNode *varNode = new ASTNode({TokenType::Variable, "x", MathFunc::None});
            ASTNode *powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            powNode->left = varNode;
            powNode->right = new ASTNode({TokenType::Number, std::to_string(power), MathFunc::None});
            if (std::abs(coeff) == 1.0)
            {
                termNode = powNode; // 單純的 x^n
            }
            else
            {
                termNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                termNode->left = new ASTNode({TokenType::Number, formatDouble(std::abs(coeff)), MathFunc::None});
                termNode->right = powNode;
            }
        }
        // 把這一項跟前面的項用 + 或 - 接起來
        if (resultNode == nullptr)
        {
            if (coeff < 0)
            {
                // 如果第一項就是負的 (例如 -3x^2)
                ASTNode *negNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                negNode->left = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                negNode->right = termNode;
                resultNode = negNode;
            }
            else
            {
                resultNode = termNode;
            }
        }
        else
        {
            string op = (coeff < 0) ? "-" : "+";
            ASTNode *comboNode = new ASTNode({TokenType::Operator, op, MathFunc::None});
            comboNode->left = resultNode;
            comboNode->right = termNode;
            resultNode = comboNode;
        }
    }
    if (resultNode == nullptr)
        return new ASTNode({TokenType::Number, "0", MathFunc::None});
    return resultNode;
}
// 🌟 輔助工具：清除 Map 中因為浮點數運算產生的微小誤差 (例如 1e-15 的係數)
void cleanPolyMap(std::map<int, double> &poly)
{
    for (auto it = poly.begin(); it != poly.end();)
    {
        if (std::abs(it->second) < 1e-9)
        {
            it = poly.erase(it); // 係數太小，直接把這項刪掉
        }
        else
        {
            ++it;
        }
    }
}
double evalPoly(const std::map<int, double> &poly, double x)
{
    double result = 0.0;
    for (auto const &term : poly)
    {
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
    std::map<int, double> &Q,
    std::map<int, double> &R)
{
    Q.clear();
    cleanPolyMap(N);
    cleanPolyMap(D);
    if (D.empty())
        return; // 防呆機制：分母不能為 0
    // 當分子還有東西，且分子的最高次數 >= 分母的最高次數時，繼續除！
    while (!N.empty())
    {
        int degN = N.rbegin()->first;
        int degD = D.rbegin()->first;
        if (degN < degD)
            break; // 無法再除，剩下的就是餘式
        double coeffN = N.rbegin()->second;
        double coeffD = D.rbegin()->second;
        // 計算這回合的商 (次數相減，係數相除)
        int degQ = degN - degD;
        double coeffQ = coeffN / coeffD;
        Q[degQ] = coeffQ; // 將算出的單項加入商式 Q(x) 中
        // N(x) = N(x) - (coeffQ * x^degQ) * D(x)
        for (auto const &term : D)
        {
            int currentDeg = term.first;
            double currentCoeff = term.second;
            N[currentDeg + degQ] -= (currentCoeff * coeffQ);
        }
        cleanPolyMap(N); // 🌟 關鍵：清除剛才抵銷掉的最高次項及微小誤差
    }
    R = N; // 迴圈結束後，剩下的 N 就是餘式 R(x)
}
double findIntegerRoot(const std::map<int, double> &poly)
{
    if (poly.empty())
        return NAN;
    double constantTerm = poly.count(0) ? poly.at(0) : 0.0;
    if (std::abs(constantTerm) < 1e-9)
        return 0.0; // 如果沒有常數項，x=0 就是一個根
    int c = std::abs(std::round(constantTerm));
    // 測試 1 到 c 的所有因數 (包含正負)
    for (int i = 1; i <= c; ++i)
    {
        if (c % i == 0)
        {
            if (std::abs(evalPoly(poly, i)) < 1e-9)
                return i;
            if (std::abs(evalPoly(poly, -i)) < 1e-9)
                return -i;
        }
    }
    return NAN; // 找不到整數根
}

// =======================================================
// 🌟 輔助雷達：多項式係數萃取器 (專門處理 2 次以下多項式)
// =======================================================
void accumulatePoly(ASTNode *node, double multiplier, double &a, double &b, double &c, bool &isPoly)
{
    if (!node || !isPoly)
        return;

    if (node->token.type == TokenType::Number)
    {
        c += multiplier * std::stod(node->token.value);
        return;
    }
    if (node->token.type == TokenType::Variable && node->token.value == "x")
    {
        b += multiplier;
        return;
    }
    if (node->token.type == TokenType::Operator)
    {
        string op = node->token.value;
        if (op == "+")
        {
            accumulatePoly(node->left, multiplier, a, b, c, isPoly);
            accumulatePoly(node->right, multiplier, a, b, c, isPoly);
            return;
        }
        if (op == "-")
        {
            accumulatePoly(node->left, multiplier, a, b, c, isPoly);
            accumulatePoly(node->right, -multiplier, a, b, c, isPoly);
            return;
        }
        if (op == "*")
        {
            double num = 1.0;
            ASTNode *nonNum = nullptr;
            if (node->left && node->left->token.type == TokenType::Number)
            {
                num = std::stod(node->left->token.value);
                nonNum = node->right;
            }
            else if (node->right && node->right->token.type == TokenType::Number)
            {
                num = std::stod(node->right->token.value);
                nonNum = node->left;
            }
            else
            {
                isPoly = false;
                return;
            }
            accumulatePoly(nonNum, multiplier * num, a, b, c, isPoly);
            return;
        }
        if (op == "/")
        {
            if (node->right && node->right->token.type == TokenType::Number)
            {
                double num = std::stod(node->right->token.value);
                accumulatePoly(node->left, multiplier / num, a, b, c, isPoly);
            }
            else
            {
                isPoly = false;
            }
            return;
        }
        if (op == "^")
        {
            if (node->left && node->left->token.type == TokenType::Variable && node->left->token.value == "x" &&
                node->right && node->right->token.type == TokenType::Number)
            {
                double p = std::stod(node->right->token.value);
                if (std::abs(p - 2.0) < 1e-9)
                    a += multiplier;
                else if (std::abs(p - 1.0) < 1e-9)
                    b += multiplier;
                else if (std::abs(p - 0.0) < 1e-9)
                    c += multiplier;
                else
                    isPoly = false; // 超過 2 次方，非目標
            }
            else
            {
                isPoly = false;
            }
            return;
        }
    }
    isPoly = false;
}

bool matchLinear(ASTNode *node, double &p, double &q)
{
    double a = 0;
    p = 0;
    q = 0;
    bool isPoly = true;
    accumulatePoly(node, 1.0, a, p, q, isPoly);
    return isPoly && std::abs(a) < 1e-9;
}

bool matchQuadratic(ASTNode *node, double &a, double &b, double &c)
{
    a = 0;
    b = 0;
    c = 0;
    bool isPoly = true;
    accumulatePoly(node, 1.0, a, b, c, isPoly);
    return isPoly && std::abs(a) > 1e-9;
}

// =======================================================
// 🌟 終極兵工廠：不可約二次式積分公式建構器 (符號根號升級版)
// =======================================================
ASTNode *buildIrreducibleQuadraticIntegral(double p, double q, double a, double b, double c, ASTNode *quadNode)
{
    double D = 4.0 * a * c - b * b; // 判別式已確保 > 0

    // 🛡️ 符號根號製造機
    auto buildSymbolicSqrt = [](double val) -> ASTNode *
    {
        double sq = std::sqrt(val);
        if (std::abs(sq - std::round(sq)) < 1e-9)
        {
            return new ASTNode({TokenType::Number, formatDouble(std::round(sq)), MathFunc::None});
        }
        else
        {
            ASTNode *sqNode = new ASTNode({TokenType::Operator, "sqrt", MathFunc::None});
            sqNode->right = new ASTNode({TokenType::Number, formatDouble(val), MathFunc::None});
            return sqNode;
        }
    };

    // --- 前半段： (p / 2a) * ln|ax^2 + bx + c| ---
    ASTNode *lnPart = nullptr;
    if (std::abs(p) > 1e-9)
    {
        double coeff1 = p / (2.0 * a);
        ASTNode *lnNode = new ASTNode({TokenType::Function, "ln", strToMathFunc("ln")});
        lnNode->right = copyTree(quadNode);

        lnPart = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        lnPart->left = new ASTNode({TokenType::Number, formatDouble(coeff1), MathFunc::None});
        lnPart->right = lnNode;
    }

    // --- 後半段： ((2aq - bp) / (a * sqrt(D))) * arctan((2ax + b) / sqrt(D)) ---
    ASTNode *atanPart = nullptr;
    double numVal = 2.0 * a * q - b * p;

    if (std::abs(numVal) > 1e-9)
    {
        ASTNode *xNode = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        ASTNode *twoAx = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        twoAx->left = new ASTNode({TokenType::Number, formatDouble(2.0 * a), MathFunc::None});
        twoAx->right = xNode;

        ASTNode *innerAdd = nullptr;
        if (std::abs(b) < 1e-9)
        {
            innerAdd = twoAx;
        }
        else
        {
            innerAdd = new ASTNode({TokenType::Operator, "+", MathFunc::None});
            innerAdd->left = twoAx;
            innerAdd->right = new ASTNode({TokenType::Number, formatDouble(b), MathFunc::None});
        }

        ASTNode *divK = new ASTNode({TokenType::Operator, "/", MathFunc::None});
        divK->left = innerAdd;
        divK->right = buildSymbolicSqrt(D);

        ASTNode *atanNode = new ASTNode({TokenType::Function, "arctan", strToMathFunc("arctan")});
        atanNode->right = divK;

        ASTNode *coeff2Node = new ASTNode({TokenType::Operator, "/", MathFunc::None});
        coeff2Node->left = new ASTNode({TokenType::Number, formatDouble(numVal), MathFunc::None});

        if (std::abs(a - 1.0) < 1e-9)
        {
            coeff2Node->right = buildSymbolicSqrt(D);
        }
        else
        {
            ASTNode *denomMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            denomMul->left = new ASTNode({TokenType::Number, formatDouble(a), MathFunc::None});
            denomMul->right = buildSymbolicSqrt(D);
            coeff2Node->right = denomMul;
        }

        atanPart = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        atanPart->left = coeff2Node;
        atanPart->right = atanNode;
    }

    if (lnPart && atanPart)
    {
        ASTNode *plusNode = new ASTNode({TokenType::Operator, "+", MathFunc::None});
        plusNode->left = lnPart;
        plusNode->right = atanPart;
        return plusNode;
    }
    else if (lnPart)
    {
        return lnPart;
    }
    else if (atanPart)
    {
        return atanPart;
    }
    else
    {
        return new ASTNode({TokenType::Number, "0", MathFunc::None});
    }
}

// =======================================================
// ⚔️ 戰術小隊 1：隱形單位元救援 (處理孤立的 ln, arctan 等)
// =======================================================
ASTNode *handleImplicitByParts(ASTNode *node, int depth)
{
    bool needRescue = false;
    // 條件 1：節點本身就是高優先級 (例如純 ln(x) 或 arctan(x))
    if (getPriority(node) >= 4)
    {
        needRescue = true;
    }
    // 條件 2：血統繼承！如果是次方 (例如 ln(x)^2)，只要底數是高優先級，照樣出動救援！
    if (node->token.value == "^" && node->left && getPriority(node->left) >= 4)
    {
        needRescue = true;
    }

    if (needRescue)
    {
        // 強制包裝成 1 * f(x)
        ASTNode *dummyMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        dummyMul->left = new ASTNode({TokenType::Number, "1", MathFunc::None});
        dummyMul->right = copyTree(node);
        // 直接呼叫分部積分大絕招！
        ASTNode *rescueResult = integrationByParts(dummyMul, depth + 1);
        deleteTree(dummyMul); // 用完即丟，確保記憶體安全
        if (rescueResult != nullptr)
            return rescueResult;
    }
    return nullptr;
}

// =======================================================
// ⚔️ 戰術小隊 2：有理函數處理 (不可約二次式、部分分式、長除法)
// =======================================================
ASTNode *handleRationalFunction(ASTNode *node, int depth)
{
    // 1. 攔截不可約二次式 (支援 / 以及 A * B^-1 偽裝)
    ASTNode *numNode = nullptr;
    ASTNode *denNode = nullptr;
    if (node->token.type == TokenType::Operator)
    {
        if (node->token.value == "/")
        {
            numNode = node->left;
            denNode = node->right;
        }
        else if (node->token.value == "*")
        {
            if (node->right && node->right->token.type == TokenType::Operator && node->right->token.value == "^" &&
                node->right->right && node->right->right->token.type == TokenType::Number && std::stod(node->right->right->token.value) == -1.0)
            {
                numNode = node->left;
                denNode = node->right->left;
            }
            else if (node->left && node->left->token.type == TokenType::Operator && node->left->token.value == "^" &&
                     node->left->right && node->left->right->token.type == TokenType::Number && std::stod(node->left->right->token.value) == -1.0)
            {
                numNode = node->right;
                denNode = node->left->left;
            }
        }
    }

    if (numNode && denNode)
    {
        double p = 0.0, q = 0.0;
        double a = 0.0, b = 0.0, c = 0.0;
        if (matchLinear(numNode, p, q) && matchQuadratic(denNode, a, b, c))
        {
            double discriminant = b * b - 4 * a * c;
            if (discriminant < -1e-9)
            {
                ASTNode *result = buildIrreducibleQuadraticIntegral(p, q, a, b, c, denNode);
                return simplify(result);
            }
        }
    }

    // 2. 僅對 "/" 進行分數的加減法拆解、長除法、部分分式
    if (node->token.type == TokenType::Operator && node->token.value == "/")
    {
        if (node->left->token.type == TokenType::Operator &&
            (node->left->token.value == "+" || node->left->token.value == "-"))
        {
            ASTNode *newPlusMinus = new ASTNode({TokenType::Operator, node->left->token.value, MathFunc::None});
            ASTNode *div1 = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            div1->left = copyTree(node->left->left);
            div1->right = copyTree(node->right);
            ASTNode *div2 = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            div2->left = copyTree(node->left->right);
            div2->right = copyTree(node->right);
            newPlusMinus->left = div1;
            newPlusMinus->right = div2;
            ASTNode *result = integrate(newPlusMinus, depth);
            deleteTree(newPlusMinus);
            if (result != nullptr)
                return result;
        }

        if (isPolynomial(node->left) && isPolynomial(node->right))
        {
            auto numMap = astToPolyMap(node->left);
            auto denMap = astToPolyMap(node->right);
            int degN = numMap.empty() ? 0 : numMap.rbegin()->first;
            int degD = denMap.empty() ? 0 : denMap.rbegin()->first;

            if (degN >= degD && degD > 0)
            {
                std::map<int, double> Q, R;
                polynomialLongDivision(numMap, denMap, Q, R);
                ASTNode *qNode = polyMapToAST(Q);
                ASTNode *rNode = polyMapToAST(R);
                ASTNode *newDiv = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                newDiv->left = rNode;
                newDiv->right = copyTree(node->right);
                ASTNode *finalRes = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                finalRes->left = qNode;
                finalRes->right = newDiv;
                ASTNode *result = integrate(finalRes, depth);
                deleteTree(finalRes);
                if (result != nullptr)
                    return result;
            }
            else if (degN < degD && degD > 0)
            {
                if (degD == 2)
                {
                    double a_coeff = denMap.count(2) ? denMap[2] : 0.0;
                    double b_coeff = denMap.count(1) ? denMap[1] : 0.0;
                    double c_coeff = denMap.count(0) ? denMap[0] : 0.0;
                    double discriminant = b_coeff * b_coeff - 4 * a_coeff * c_coeff;
                    if (discriminant > 0 && a_coeff != 0.0)
                    {
                        double r1 = (-b_coeff + sqrt(discriminant)) / (2 * a_coeff);
                        double r2 = (-b_coeff - sqrt(discriminant)) / (2 * a_coeff);
                        double num_r1 = evalPoly(numMap, r1);
                        double num_r2 = evalPoly(numMap, r2);
                        double A = num_r1 / (a_coeff * (r1 - r2));
                        double B = num_r2 / (a_coeff * (r2 - r1));
                        ASTNode *term1 = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                        term1->left = new ASTNode({TokenType::Variable, "x", MathFunc::None});
                        term1->right = new ASTNode({TokenType::Number, formatDouble(r1), MathFunc::None});
                        ASTNode *pow1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                        pow1->left = term1;
                        pow1->right = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                        ASTNode *partA = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                        partA->left = new ASTNode({TokenType::Number, formatDouble(A), MathFunc::None});
                        partA->right = pow1;
                        ASTNode *term2 = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                        term2->left = new ASTNode({TokenType::Variable, "x", MathFunc::None});
                        term2->right = new ASTNode({TokenType::Number, formatDouble(r2), MathFunc::None});
                        ASTNode *pow2 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                        pow2->left = term2;
                        pow2->right = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                        ASTNode *partB = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                        partB->left = new ASTNode({TokenType::Number, formatDouble(B), MathFunc::None});
                        partB->right = pow2;
                        ASTNode *finalPF = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                        finalPF->left = partA;
                        finalPF->right = partB;
                        ASTNode *result = integrate(finalPF, depth);
                        deleteTree(finalPF);
                        if (result != nullptr)
                            return result;
                    }
                    else if (discriminant < 0 && a_coeff != 0.0 && degN == 0)
                    {
                        double K = numMap.count(0) ? numMap[0] : 0.0;
                        double sqrt_neg_delta = std::sqrt(-discriminant);
                        double outer_coeff = (2.0 * K) / sqrt_neg_delta;
                        ASTNode *two_a_x = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                        two_a_x->left = new ASTNode({TokenType::Number, formatDouble(2.0 * a_coeff), MathFunc::None});
                        two_a_x->right = new ASTNode({TokenType::Variable, "x", MathFunc::None});
                        ASTNode *inner_add = nullptr;
                        if (std::abs(b_coeff) < 1e-9)
                        {
                            inner_add = two_a_x;
                        }
                        else
                        {
                            string op = (b_coeff < 0) ? "-" : "+";
                            inner_add = new ASTNode({TokenType::Operator, op, MathFunc::None});
                            inner_add->left = two_a_x;
                            inner_add->right = new ASTNode({TokenType::Number, formatDouble(std::abs(b_coeff)), MathFunc::None});
                        }
                        ASTNode *inner_div = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                        inner_div->left = inner_add;
                        inner_div->right = new ASTNode({TokenType::Number, formatDouble(sqrt_neg_delta), MathFunc::None});
                        ASTNode *arctanNode = new ASTNode({TokenType::Function, "arctan", strToMathFunc("arctan")});
                        arctanNode->right = inner_div;
                        ASTNode *finalRes = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                        finalRes->left = new ASTNode({TokenType::Number, formatDouble(outer_coeff), MathFunc::None});
                        finalRes->right = arctanNode;
                        return finalRes;
                    }
                }
                else if (degD >= 3)
                {
                    double r = findIntegerRoot(denMap);
                    if (!std::isnan(r))
                    {
                        std::map<int, double> linearFactor;
                        linearFactor[1] = 1.0;
                        linearFactor[0] = -r;
                        std::map<int, double> Q_map, remainderD;
                        polynomialLongDivision(denMap, linearFactor, Q_map, remainderD);
                        double num_r = evalPoly(numMap, r);
                        double Q_r = evalPoly(Q_map, r);
                        if (std::abs(Q_r) > 1e-9)
                        {
                            double A_val = num_r / Q_r;
                            std::map<int, double> N_minus_AQ = numMap;
                            for (auto const &term : Q_map)
                                N_minus_AQ[term.first] -= A_val * term.second;
                            std::map<int, double> P_map, remainderP;
                            polynomialLongDivision(N_minus_AQ, linearFactor, P_map, remainderP);
                            ASTNode *term1 = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                            term1->left = new ASTNode({TokenType::Variable, "x", MathFunc::None});
                            term1->right = new ASTNode({TokenType::Number, formatDouble(r), MathFunc::None});
                            ASTNode *pow1 = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                            pow1->left = term1;
                            pow1->right = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                            ASTNode *partA = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                            partA->left = new ASTNode({TokenType::Number, formatDouble(A_val), MathFunc::None});
                            partA->right = pow1;
                            ASTNode *P_AST = polyMapToAST(P_map);
                            ASTNode *Q_AST = polyMapToAST(Q_map);
                            ASTNode *partP_Q = new ASTNode({TokenType::Operator, "/", MathFunc::None});
                            partP_Q->left = P_AST;
                            partP_Q->right = Q_AST;
                            ASTNode *finalPF = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                            finalPF->left = partA;
                            finalPF->right = partP_Q;
                            ASTNode *result = integrate(finalPF, depth);
                            deleteTree(finalPF);
                            if (result != nullptr)
                                return result;
                        }
                    }
                }
            }
        }

        // 3. 兜底轉換：除法無條件轉乘法負一次方，防止死循環
        ASTNode *powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
        powNode->left = copyTree(node->right);
        powNode->right = new ASTNode({TokenType::Number, "-1", MathFunc::None});
        ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        mulNode->left = copyTree(node->left);
        mulNode->right = powNode;
        ASTNode *result = integrate(mulNode, depth);
        deleteTree(mulNode);
        return result;
    }
    return nullptr;
}

// =======================================================
// ⚔️ 戰術小隊 3：變數變換 (U-Substitution)
// =======================================================
ASTNode *handleUSubstitution(ASTNode *node, const std::vector<ASTNode *> &factors, int depth)
{
    for (size_t candidateIdx = 0; candidateIdx < factors.size(); ++candidateIdx)
    {
        ASTNode *f = factors[candidateIdx];
        bool isCandidate = false;

        if (f->token.type == TokenType::Operator && f->token.value == "^")
        {
            if (f->left && f->left->token.value == "e")
            {
                if (!(f->right && f->right->token.type == TokenType::Variable && f->right->token.value == "x"))
                {
                    isCandidate = true;
                }
            }
            else if (f->right && f->right->token.type == TokenType::Number)
            {
                if (!(f->left && f->left->token.type == TokenType::Variable && f->left->token.value == "x"))
                {
                    isCandidate = true;
                }
            }
        }
        if (f->token.type == TokenType::Function)
        {
            isCandidate = true;
        }
        if (f->token.type == TokenType::Operator && f->token.value == "^")
        {
            if (f->left && f->left->token.value == "e")
            {
                isCandidate = true;
            }
            else if (f->right && f->right->token.type == TokenType::Number)
            {
                if (!(f->left && f->left->token.type == TokenType::Variable && f->left->token.value == "x"))
                {
                    isCandidate = true;
                }
            }
        }
        if (!isCandidate)
            continue;

        std::vector<ASTNode *> remainFactors;
        for (size_t i = 0; i < factors.size(); ++i)
        {
            if (i != candidateIdx)
                remainFactors.push_back(copyTree(factors[i]));
        }
        ASTNode *remain_node = rebuildProduct(remainFactors);

        ASTNode *u_node = nullptr;
        ASTNode *transformed = performSubstitution(f, remain_node, u_node);

        if (transformed && !isSameTree(node, transformed))
        {
            ASTNode *intResult = integrate(transformed, depth + 1);
            if (intResult)
            {
                ASTNode *finalResult = substituteVariableBack(intResult, "x", u_node);
                deleteTree(transformed);
                deleteTree(u_node);
                deleteTree(remain_node);
                return finalResult;
            }
            deleteTree(transformed);
            deleteTree(u_node);
        }
        deleteTree(remain_node);
    }
    return nullptr;
}

// =======================================================
// ⚔️ 戰術小隊 4：三角多項式複合積分
// =======================================================
ASTNode *handleTrigPolynomials(ASTNode *node, const std::vector<ASTNode *> &factors, int depth)
{
    // ⚔️ 戰略 3-C：三角倍角公式
    {
        int sinIdx = -1, cosIdx = -1;
        ASTNode *uNode = nullptr;
        for (size_t i = 0; i < factors.size(); ++i)
        {
            if (factors[i]->token.type == TokenType::Function && factors[i]->token.value == "sin")
            {
                for (size_t j = 0; j < factors.size(); ++j)
                {
                    if (i != j && factors[j]->token.type == TokenType::Function && factors[j]->token.value == "cos")
                    {
                        if (isSameTree(factors[i]->right, factors[j]->right))
                        {
                            sinIdx = i;
                            cosIdx = j;
                            uNode = factors[i]->right;
                            break;
                        }
                    }
                }
            }
            if (sinIdx != -1)
                break;
        }
        if (sinIdx != -1 && cosIdx != -1)
        {
            ASTNode *twoU = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            twoU->left = new ASTNode({TokenType::Number, "2", MathFunc::None});
            twoU->right = copyTree(uNode);
            ASTNode *sin2u = new ASTNode({TokenType::Function, "sin", MathFunc::sin});
            sin2u->right = twoU;
            ASTNode *halfSin = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            halfSin->left = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
            halfSin->right = sin2u;
            std::vector<ASTNode *> remainFactors;
            for (size_t i = 0; i < factors.size(); ++i)
            {
                if (static_cast<int>(i) != sinIdx && static_cast<int>(i) != cosIdx)
                {
                    remainFactors.push_back(factors[i]);
                }
            }
            ASTNode *remainNode = rebuildProduct(remainFactors);
            ASTNode *finalCombined = nullptr;
            if (remainNode)
            {
                finalCombined = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                finalCombined->left = halfSin;
                finalCombined->right = remainNode;
            }
            else
            {
                finalCombined = halfSin;
            }
            ASTNode *result = integrate(finalCombined, depth + 1);
            if (remainNode)
                deleteTree(finalCombined);
            else
                deleteTree(halfSin);
            if (result != nullptr)
                return result;
        }
    }
    // ⚔️ 戰略 3-D：奇數次方三角剝離法則
    {
        int oddTrigIdx = -1;
        int m_odd = 0;
        for (size_t i = 0; i < factors.size(); ++i)
        {
            ASTNode *f = factors[i];
            if (f->token.type == TokenType::Operator && f->token.value == "^")
            {
                if (f->left && f->left->token.type == TokenType::Function &&
                    (f->left->token.value == "sin" || f->left->token.value == "cos"))
                {
                    if (f->right && f->right->token.type == TokenType::Number)
                    {
                        double powerVal = std::stod(f->right->token.value);
                        int m = std::round(powerVal);
                        if (std::abs(powerVal - m) < 1e-9 && m % 2 != 0 && m >= 3)
                        {
                            oddTrigIdx = i;
                            m_odd = m;
                            break;
                        }
                    }
                }
            }
        }
        if (oddTrigIdx != -1)
        {
            ASTNode *targetNode = factors[oddTrigIdx];
            string funcName = targetNode->left->token.value;
            string oppFunc = (funcName == "sin") ? "cos" : "sin";
            ASTNode *uNode = targetNode->left->right;
            int k = (m_odd - 1) / 2;
            ASTNode *peeledFunc = new ASTNode({TokenType::Function, funcName, strToMathFunc(funcName)});
            peeledFunc->right = copyTree(uNode);
            ASTNode *oppFuncSq = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            ASTNode *oppFuncNode = new ASTNode({TokenType::Function, oppFunc, strToMathFunc(oppFunc)});
            oppFuncNode->right = copyTree(uNode);
            oppFuncSq->left = oppFuncNode;
            oppFuncSq->right = new ASTNode({TokenType::Number, "2", MathFunc::None});
            ASTNode *oneMinus = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            oneMinus->left = new ASTNode({TokenType::Number, "1", MathFunc::None});
            oneMinus->right = oppFuncSq;
            ASTNode *convertedPart = (k == 1) ? oneMinus : new ASTNode({TokenType::Operator, "^", MathFunc::None});
            if (k != 1)
            {
                convertedPart->left = oneMinus;
                convertedPart->right = new ASTNode({TokenType::Number, std::to_string(k), MathFunc::None});
            }
            ASTNode *newTarget = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            newTarget->left = convertedPart;
            newTarget->right = peeledFunc;
            std::vector<ASTNode *> remainFactors;
            for (size_t i = 0; i < factors.size(); ++i)
            {
                if (static_cast<int>(i) != oddTrigIdx)
                    remainFactors.push_back(factors[i]);
            }
            ASTNode *remainNode = rebuildProduct(remainFactors);
            ASTNode *finalCombined = nullptr;
            if (remainNode)
            {
                finalCombined = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                finalCombined->left = newTarget;
                finalCombined->right = remainNode;
            }
            else
            {
                finalCombined = newTarget;
            }
            ASTNode *expandedFinal = expand(finalCombined);
            ASTNode *simplifiedFinal = simplify(expandedFinal);
            ASTNode *result = integrate(simplifiedFinal, depth);
            deleteTree(simplifiedFinal);
            if (remainNode)
                deleteTree(finalCombined);
            else
                deleteTree(newTarget);
            if (result != nullptr)
                return result;
        }
    }
    // ⚔️ 戰略 3-D.5：雙偶數純化法則
    {
        int sinIdx = -1, cosIdx = -1;
        int m_sin = 0, n_cos = 0;
        for (size_t i = 0; i < factors.size(); ++i)
        {
            ASTNode *f = factors[i];
            if (f->token.value == "^" && f->left && (f->left->token.value == "sin" || f->left->token.value == "cos"))
            {
                if (f->right && f->right->token.type == TokenType::Number)
                {
                    int p = std::round(std::stod(f->right->token.value));
                    if (p % 2 == 0)
                    {
                        if (f->left->token.value == "sin")
                        {
                            sinIdx = i;
                            m_sin = p;
                        }
                        else if (f->left->token.value == "cos")
                        {
                            cosIdx = i;
                            n_cos = p;
                        }
                    }
                }
            }
        }
        if (sinIdx != -1 && cosIdx != -1)
        {
            ASTNode *uNode = factors[sinIdx]->left->right;
            int k = m_sin / 2;
            ASTNode *cosNode = new ASTNode({TokenType::Function, "cos", MathFunc::cos});
            cosNode->right = copyTree(uNode);
            ASTNode *cosSq = new ASTNode({TokenType::Operator, "^", MathFunc::None});
            cosSq->left = cosNode;
            cosSq->right = new ASTNode({TokenType::Number, "2", MathFunc::None});
            ASTNode *oneMinus = new ASTNode({TokenType::Operator, "-", MathFunc::None});
            oneMinus->left = new ASTNode({TokenType::Number, "1", MathFunc::None});
            oneMinus->right = cosSq;
            ASTNode *convertedPart = (k == 1) ? oneMinus : new ASTNode({TokenType::Operator, "^", MathFunc::None});
            if (k != 1)
            {
                convertedPart->left = oneMinus;
                convertedPart->right = new ASTNode({TokenType::Number, std::to_string(k), MathFunc::None});
            }
            std::vector<ASTNode *> remainFactors;
            for (size_t i = 0; i < factors.size(); ++i)
            {
                if (static_cast<int>(i) != sinIdx)
                    remainFactors.push_back(copyTree(factors[i]));
            }
            ASTNode *remainNode = rebuildProduct(remainFactors);
            ASTNode *finalCombined = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            finalCombined->left = convertedPart;
            finalCombined->right = remainNode;
            ASTNode *expandedFinal = expand(finalCombined);
            ASTNode *simplifiedFinal = simplify(expandedFinal);
            ASTNode *result = integrate(simplifiedFinal, depth);
            deleteTree(simplifiedFinal);
            deleteTree(finalCombined);
            if (result != nullptr)
                return result;
        }
    }
    // ⚔️ 戰略 3-E：偶數次方降次法則
    {
        int evenTrigIdx = -1;
        int m_even = 0;
        for (size_t i = 0; i < factors.size(); ++i)
        {
            ASTNode *f = factors[i];
            if (f->token.type == TokenType::Operator && f->token.value == "^")
            {
                if (f->left && f->left->token.type == TokenType::Function &&
                    (f->left->token.value == "sin" || f->left->token.value == "cos"))
                {
                    if (f->right && f->right->token.type == TokenType::Number)
                    {
                        double powerVal = std::stod(f->right->token.value);
                        int m = std::round(powerVal);
                        if (std::abs(powerVal - m) < 1e-9 && m % 2 == 0 && m >= 2)
                        {
                            evenTrigIdx = i;
                            m_even = m;
                            break;
                        }
                    }
                }
            }
        }
        if (evenTrigIdx != -1)
        {
            ASTNode *targetNode = factors[evenTrigIdx];
            string funcName = targetNode->left->token.value;
            ASTNode *uNode = targetNode->left->right;
            int k = m_even / 2;
            ASTNode *twoU = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            twoU->left = new ASTNode({TokenType::Number, "2", MathFunc::None});
            twoU->right = copyTree(uNode);
            ASTNode *cos2u = new ASTNode({TokenType::Function, "cos", MathFunc::cos});
            cos2u->right = twoU;
            ASTNode *halfCos = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            halfCos->left = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
            halfCos->right = cos2u;
            string op = (funcName == "cos") ? "+" : "-";
            ASTNode *baseNode = new ASTNode({TokenType::Operator, op, MathFunc::None});
            baseNode->left = new ASTNode({TokenType::Number, "0.5", MathFunc::None});
            baseNode->right = halfCos;
            ASTNode *convertedPart = nullptr;
            if (k == 1)
            {
                convertedPart = baseNode;
            }
            else
            {
                convertedPart = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                convertedPart->left = baseNode;
                convertedPart->right = new ASTNode({TokenType::Number, std::to_string(k), MathFunc::None});
            }
            std::vector<ASTNode *> remainFactors;
            for (size_t i = 0; i < factors.size(); ++i)
            {
                if (static_cast<int>(i) != evenTrigIdx)
                    remainFactors.push_back(factors[i]);
            }
            ASTNode *remainNode = rebuildProduct(remainFactors);
            ASTNode *finalCombined = nullptr;
            if (remainNode)
            {
                finalCombined = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                finalCombined->left = convertedPart;
                finalCombined->right = remainNode;
            }
            else
            {
                finalCombined = convertedPart;
            }
            ASTNode *expandedFinal = expand(finalCombined);
            ASTNode *simplifiedFinal = simplify(expandedFinal);
            ASTNode *result = integrate(simplifiedFinal, depth);
            deleteTree(simplifiedFinal);
            if (remainNode)
                deleteTree(finalCombined);
            else
                deleteTree(convertedPart);
            if (result != nullptr)
                return result;
        }
    }
    // ⚔️ 戰略 3-F：sec-tan 與 csc-cot 乘積積分法
    {
        int secIdx = -1, tanIdx = -1;
        int cscIdx = -1, cotIdx = -1;
        for (size_t i = 0; i < factors.size(); ++i)
        {
            if (factors[i]->token.type == TokenType::Function)
            {
                if (factors[i]->token.value == "sec")
                    secIdx = i;
                if (factors[i]->token.value == "tan")
                    tanIdx = i;
                if (factors[i]->token.value == "csc")
                    cscIdx = i;
                if (factors[i]->token.value == "cot")
                    cotIdx = i;
            }
        }
        if (secIdx != -1 && tanIdx != -1)
        {
            ASTNode *res = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
            res->right = copyTree(factors[secIdx]->right);
            return res;
        }
        if (cscIdx != -1 && cotIdx != -1)
        {
            ASTNode *negNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            negNode->left = new ASTNode({TokenType::Number, "-1", MathFunc::None});
            ASTNode *cscRes = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
            cscRes->right = copyTree(factors[cscIdx]->right);
            negNode->right = cscRes;
            return negNode;
        }
    }
    // ⚔️ 戰略 3-G：sec-tan 族系終極代數轉換
    {
        int secIdx = -1, tanIdx = -1;
        int m_sec = 0, n_tan = 0;
        double coeff = 1.0;
        std::vector<ASTNode *> otherFactors;
        for (size_t i = 0; i < factors.size(); ++i)
        {
            ASTNode *f = factors[i];
            if (f->token.type == TokenType::Number)
            {
                coeff *= std::stod(f->token.value);
            }
            else if (f->token.value == "^" && f->left && f->left->token.value == "sec")
            {
                secIdx = i;
                m_sec = std::round(std::stod(f->right->token.value));
            }
            else if (f->token.value == "^" && f->left && f->left->token.value == "tan")
            {
                tanIdx = i;
                n_tan = std::round(std::stod(f->right->token.value));
            }
            else if (f->token.type == TokenType::Function && f->token.value == "sec")
            {
                secIdx = i;
                m_sec = 1;
            }
            else if (f->token.type == TokenType::Function && f->token.value == "tan")
            {
                tanIdx = i;
                n_tan = 1;
            }
            else
            {
                otherFactors.push_back(f);
            }
        }
        if (otherFactors.empty() && (secIdx != -1 || tanIdx != -1))
        {
            ASTNode *uNode = nullptr;
            if (secIdx != -1)
                uNode = (factors[secIdx]->token.value == "^") ? factors[secIdx]->left->right : factors[secIdx]->right;
            else
                uNode = (factors[tanIdx]->token.value == "^") ? factors[tanIdx]->left->right : factors[tanIdx]->right;
            if (m_sec >= 4 && m_sec % 2 == 0)
            {
                int k = (m_sec - 2) / 2;
                ASTNode *tanNode = new ASTNode({TokenType::Function, "tan", MathFunc::tan});
                tanNode->right = copyTree(uNode);
                ASTNode *tanSq = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                tanSq->left = tanNode;
                tanSq->right = new ASTNode({TokenType::Number, "2", MathFunc::None});
                ASTNode *tanSqPlusOne = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                tanSqPlusOne->left = tanSq;
                tanSqPlusOne->right = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode *secConverted = (k == 1) ? tanSqPlusOne : new ASTNode({TokenType::Operator, "^", MathFunc::None});
                if (k != 1)
                {
                    secConverted->left = tanSqPlusOne;
                    secConverted->right = new ASTNode({TokenType::Number, std::to_string(k), MathFunc::None});
                }
                ASTNode *secSq = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode *sBase = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                sBase->right = copyTree(uNode);
                secSq->left = sBase;
                secSq->right = new ASTNode({TokenType::Number, "2", MathFunc::None});
                ASTNode *restNodes = secSq;
                if (n_tan > 0)
                {
                    ASTNode *tanOriginal = (n_tan == 1) ? new ASTNode({TokenType::Function, "tan", MathFunc::tan}) : new ASTNode({TokenType::Operator, "^", MathFunc::None});
                    if (n_tan == 1)
                    {
                        tanOriginal->right = copyTree(uNode);
                    }
                    else
                    {
                        ASTNode *tb = new ASTNode({TokenType::Function, "tan", MathFunc::tan});
                        tb->right = copyTree(uNode);
                        tanOriginal->left = tb;
                        tanOriginal->right = new ASTNode({TokenType::Number, std::to_string(n_tan), MathFunc::None});
                    }
                    ASTNode *mul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    mul->left = secSq;
                    mul->right = tanOriginal;
                    restNodes = mul;
                }
                ASTNode *convertedFormula = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                convertedFormula->left = secConverted;
                convertedFormula->right = restNodes;
                if (coeff != 1.0)
                {
                    ASTNode *cNode = new ASTNode({TokenType::Number, formatDouble(coeff), MathFunc::None});
                    ASTNode *r = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    r->left = cNode;
                    r->right = convertedFormula;
                    convertedFormula = r;
                }
                ASTNode *simplifiedFinal = simplify(convertedFormula);
                ASTNode *result = integrate(simplifiedFinal, depth);
                deleteTree(simplifiedFinal);
                if (result)
                    return result;
            }
            else if (n_tan >= 3 && n_tan % 2 != 0)
            {
                int k = (n_tan - 1) / 2;
                ASTNode *secNode = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                secNode->right = copyTree(uNode);
                ASTNode *secSq = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                secSq->left = secNode;
                secSq->right = new ASTNode({TokenType::Number, "2", MathFunc::None});
                ASTNode *secSqMinusOne = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                secSqMinusOne->left = secSq;
                secSqMinusOne->right = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode *tanConverted = (k == 1) ? secSqMinusOne : new ASTNode({TokenType::Operator, "^", MathFunc::None});
                if (k != 1)
                {
                    tanConverted->left = secSqMinusOne;
                    tanConverted->right = new ASTNode({TokenType::Number, std::to_string(k), MathFunc::None});
                }
                ASTNode *duNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *sOne = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                sOne->right = copyTree(uNode);
                ASTNode *tOne = new ASTNode({TokenType::Function, "tan", MathFunc::tan});
                tOne->right = copyTree(uNode);
                duNode->left = sOne;
                duNode->right = tOne;
                ASTNode *restNodes = duNode;
                int remaining_sec = m_sec - 1;
                if (remaining_sec > 0)
                {
                    ASTNode *secRem = (remaining_sec == 1) ? new ASTNode({TokenType::Function, "sec", MathFunc::sec}) : new ASTNode({TokenType::Operator, "^", MathFunc::None});
                    if (remaining_sec == 1)
                    {
                        secRem->right = copyTree(uNode);
                    }
                    else
                    {
                        ASTNode *sb = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                        sb->right = copyTree(uNode);
                        secRem->left = sb;
                        secRem->right = new ASTNode({TokenType::Number, std::to_string(remaining_sec), MathFunc::None});
                    }
                    ASTNode *mul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    mul->left = secRem;
                    mul->right = duNode;
                    restNodes = mul;
                }
                ASTNode *convertedFormula = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                convertedFormula->left = tanConverted;
                convertedFormula->right = restNodes;
                if (coeff != 1.0)
                {
                    ASTNode *cNode = new ASTNode({TokenType::Number, formatDouble(coeff), MathFunc::None});
                    ASTNode *r = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    r->left = cNode;
                    r->right = convertedFormula;
                    convertedFormula = r;
                }
                ASTNode *simplifiedFinal = simplify(convertedFormula);
                ASTNode *result = integrate(simplifiedFinal, depth);
                deleteTree(simplifiedFinal);
                if (result)
                    return result;
            }
            else if (m_sec % 2 != 0 && n_tan >= 2 && n_tan % 2 == 0)
            {
                int k = n_tan / 2;
                ASTNode *secNode = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                secNode->right = copyTree(uNode);
                ASTNode *secSq = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                secSq->left = secNode;
                secSq->right = new ASTNode({TokenType::Number, "2", MathFunc::None});
                ASTNode *secSqMinusOne = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                secSqMinusOne->left = secSq;
                secSqMinusOne->right = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode *tanConverted = (k == 1) ? secSqMinusOne : new ASTNode({TokenType::Operator, "^", MathFunc::None});
                if (k != 1)
                {
                    tanConverted->left = secSqMinusOne;
                    tanConverted->right = new ASTNode({TokenType::Number, std::to_string(k), MathFunc::None});
                }
                ASTNode *restNodes = nullptr;
                if (m_sec == 1)
                {
                    restNodes = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                    restNodes->right = copyTree(uNode);
                }
                else
                {
                    restNodes = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                    ASTNode *sb = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                    sb->right = copyTree(uNode);
                    restNodes->left = sb;
                    restNodes->right = new ASTNode({TokenType::Number, std::to_string(m_sec), MathFunc::None});
                }
                ASTNode *convertedFormula = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                convertedFormula->left = tanConverted;
                convertedFormula->right = restNodes;
                if (coeff != 1.0)
                {
                    ASTNode *cNode = new ASTNode({TokenType::Number, formatDouble(coeff), MathFunc::None});
                    ASTNode *r = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    r->left = cNode;
                    r->right = convertedFormula;
                    convertedFormula = r;
                }
                ASTNode *simplifiedFinal = simplify(convertedFormula);
                ASTNode *result = integrate(simplifiedFinal, depth);
                deleteTree(simplifiedFinal);
                if (result)
                    return result;
            }
        }
    }
    {
        double coeff = 1.0;
        int secIdx = -1, tanIdx = -1;
        int m_sec = 0, n_tan = 0;
        bool pureSecTan = true;
        ASTNode *uNode = nullptr;
        for (size_t i = 0; i < factors.size(); ++i)
        {
            ASTNode *f = factors[i];
            if (f->token.type == TokenType::Number)
            {
                coeff *= std::stod(f->token.value);
            }
            else if (f->token.value == "^" && f->left && f->left->token.value == "sec")
            {
                secIdx = i;
                m_sec = std::round(std::stod(f->right->token.value));
                uNode = f->left->right;
            }
            else if (f->token.value == "^" && f->left && f->left->token.value == "tan")
            {
                tanIdx = i;
                n_tan = std::round(std::stod(f->right->token.value));
                uNode = f->left->right;
            }
            else if (f->token.type == TokenType::Function && f->token.value == "sec")
            {
                secIdx = i;
                m_sec = 1;
                uNode = f->right;
            }
            else if (f->token.type == TokenType::Function && f->token.value == "tan")
            {
                tanIdx = i;
                n_tan = 1;
                uNode = f->right;
            }
            else
            {
                pureSecTan = false;
                break;
            }
        }
        if (pureSecTan && uNode)
        {
            if (n_tan == 1 && m_sec >= 1)
            {
                ASTNode *res = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                res->left = doubleToFractionAST(coeff / m_sec);
                if (m_sec == 1)
                {
                    ASTNode *secNode = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                    secNode->right = copyTree(uNode);
                    res->right = secNode;
                }
                else
                {
                    ASTNode *secPow = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                    ASTNode *secBase = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                    secBase->right = copyTree(uNode);
                    secPow->left = secBase;
                    secPow->right = new ASTNode({TokenType::Number, std::to_string(m_sec), MathFunc::None});
                    res->right = secPow;
                }
                return res;
            }
            if (m_sec == 2 && n_tan >= 0)
            {
                ASTNode *res = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                res->left = doubleToFractionAST(coeff / (n_tan + 1.0));
                ASTNode *tanPow = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode *tanBase = new ASTNode({TokenType::Function, "tan", MathFunc::tan});
                tanBase->right = copyTree(uNode);
                tanPow->left = tanBase;
                tanPow->right = new ASTNode({TokenType::Number, std::to_string(n_tan + 1), MathFunc::None});
                res->right = tanPow;
                if (n_tan + 1 == 1)
                {
                    tanPow->left = nullptr;
                    deleteTree(tanPow);
                    res->right = tanBase;
                }
                return res;
            }
            if (n_tan == 0 && m_sec >= 3 && m_sec % 2 != 0)
            {
                double coeff1 = 1.0 / (m_sec - 1.0);
                ASTNode *term1 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                term1->left = doubleToFractionAST(coeff1 * coeff);
                ASTNode *secPow = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode *secBase = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                secBase->right = copyTree(uNode);
                secPow->left = secBase;
                secPow->right = new ASTNode({TokenType::Number, std::to_string(m_sec - 2), MathFunc::None});
                if (m_sec - 2 == 1)
                {
                    secPow->left = nullptr;
                    deleteTree(secPow);
                    secPow = secBase;
                }
                ASTNode *tanNode = new ASTNode({TokenType::Function, "tan", MathFunc::tan});
                tanNode->right = copyTree(uNode);
                ASTNode *trigMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                trigMul->left = secPow;
                trigMul->right = tanNode;
                term1->right = trigMul;
                double coeff2 = (m_sec - 2.0) / (m_sec - 1.0);
                ASTNode *nextTarget = nullptr;
                if (m_sec - 2 == 1)
                {
                    nextTarget = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                    nextTarget->right = copyTree(uNode);
                }
                else
                {
                    nextTarget = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                    ASTNode *nb = new ASTNode({TokenType::Function, "sec", MathFunc::sec});
                    nb->right = copyTree(uNode);
                    nextTarget->left = nb;
                    nextTarget->right = new ASTNode({TokenType::Number, std::to_string(m_sec - 2), MathFunc::None});
                }
                ASTNode *integratedPart = integrate(nextTarget, depth + 1);
                deleteTree(nextTarget);
                if (integratedPart)
                {
                    ASTNode *term2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    term2->left = doubleToFractionAST(coeff2 * coeff);
                    term2->right = integratedPart;
                    ASTNode *finalRes = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                    finalRes->left = term1;
                    finalRes->right = term2;
                    return finalRes;
                }
                else
                {
                    deleteTree(term1);
                }
            }
        }
    }
    {
        int cscIdx = -1, cotIdx = -1;
        int m_csc = 0, n_cot = 0;
        double coeff_csc_cot = 1.0;
        std::vector<ASTNode *> otherFactors_csc_cot;
        for (size_t i = 0; i < factors.size(); ++i)
        {
            ASTNode *f = factors[i];
            if (f->token.type == TokenType::Number)
            {
                coeff_csc_cot *= std::stod(f->token.value);
            }
            else if (f->token.value == "^" && f->left && f->left->token.value == "csc")
            {
                cscIdx = i;
                m_csc = std::round(std::stod(f->right->token.value));
            }
            else if (f->token.value == "^" && f->left && f->left->token.value == "cot")
            {
                cotIdx = i;
                n_cot = std::round(std::stod(f->right->token.value));
            }
            else if (f->token.type == TokenType::Function && f->token.value == "csc")
            {
                cscIdx = i;
                m_csc = 1;
            }
            else if (f->token.type == TokenType::Function && f->token.value == "cot")
            {
                cotIdx = i;
                n_cot = 1;
            }
            else
            {
                otherFactors_csc_cot.push_back(f);
            }
        }
        if (otherFactors_csc_cot.empty() && (cscIdx != -1 || cotIdx != -1))
        {
            ASTNode *uNode = nullptr;
            if (cscIdx != -1)
                uNode = (factors[cscIdx]->token.value == "^") ? factors[cscIdx]->left->right : factors[cscIdx]->right;
            else
                uNode = (factors[cotIdx]->token.value == "^") ? factors[cotIdx]->left->right : factors[cotIdx]->right;
            if (m_csc >= 4 && m_csc % 2 == 0)
            {
                int k = (m_csc - 2) / 2;
                ASTNode *cotNode = new ASTNode({TokenType::Function, "cot", MathFunc::cot});
                cotNode->right = copyTree(uNode);
                ASTNode *cotSq = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                cotSq->left = cotNode;
                cotSq->right = new ASTNode({TokenType::Number, "2", MathFunc::None});
                ASTNode *cotSqPlusOne = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                cotSqPlusOne->left = cotSq;
                cotSqPlusOne->right = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode *cscConverted = (k == 1) ? cotSqPlusOne : new ASTNode({TokenType::Operator, "^", MathFunc::None});
                if (k != 1)
                {
                    cscConverted->left = cotSqPlusOne;
                    cscConverted->right = new ASTNode({TokenType::Number, std::to_string(k), MathFunc::None});
                }
                ASTNode *cscSq = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode *cBase = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                cBase->right = copyTree(uNode);
                cscSq->left = cBase;
                cscSq->right = new ASTNode({TokenType::Number, "2", MathFunc::None});
                ASTNode *restNodes = cscSq;
                if (n_cot > 0)
                {
                    ASTNode *cotOriginal = (n_cot == 1) ? new ASTNode({TokenType::Function, "cot", MathFunc::cot}) : new ASTNode({TokenType::Operator, "^", MathFunc::None});
                    if (n_cot == 1)
                    {
                        cotOriginal->right = copyTree(uNode);
                    }
                    else
                    {
                        ASTNode *tb = new ASTNode({TokenType::Function, "cot", MathFunc::cot});
                        tb->right = copyTree(uNode);
                        cotOriginal->left = tb;
                        cotOriginal->right = new ASTNode({TokenType::Number, std::to_string(n_cot), MathFunc::None});
                    }
                    ASTNode *mul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    mul->left = cscSq;
                    mul->right = cotOriginal;
                    restNodes = mul;
                }
                ASTNode *convertedFormula = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                convertedFormula->left = cscConverted;
                convertedFormula->right = restNodes;
                coeff_csc_cot = -coeff_csc_cot;
                if (coeff_csc_cot != 1.0)
                {
                    ASTNode *cNode = new ASTNode({TokenType::Number, formatDouble(coeff_csc_cot), MathFunc::None});
                    ASTNode *r = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    r->left = cNode;
                    r->right = convertedFormula;
                    convertedFormula = r;
                }
                ASTNode *simplifiedFinal = simplify(convertedFormula);
                ASTNode *result = integrate(simplifiedFinal, depth);
                deleteTree(simplifiedFinal);
                if (result)
                    return result;
            }
            else if (n_cot >= 3 && n_cot % 2 != 0)
            {
                int k = (n_cot - 1) / 2;
                ASTNode *cscNode = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                cscNode->right = copyTree(uNode);
                ASTNode *cscSq = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                cscSq->left = cscNode;
                cscSq->right = new ASTNode({TokenType::Number, "2", MathFunc::None});
                ASTNode *cscSqMinusOne = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                cscSqMinusOne->left = cscSq;
                cscSqMinusOne->right = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode *cotConverted = (k == 1) ? cscSqMinusOne : new ASTNode({TokenType::Operator, "^", MathFunc::None});
                if (k != 1)
                {
                    cotConverted->left = cscSqMinusOne;
                    cotConverted->right = new ASTNode({TokenType::Number, std::to_string(k), MathFunc::None});
                }
                ASTNode *duNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                ASTNode *cOne = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                cOne->right = copyTree(uNode);
                ASTNode *tOne = new ASTNode({TokenType::Function, "cot", MathFunc::cot});
                tOne->right = copyTree(uNode);
                duNode->left = cOne;
                duNode->right = tOne;
                ASTNode *restNodes = duNode;
                int remaining_csc = m_csc - 1;
                if (remaining_csc > 0)
                {
                    ASTNode *cscRem = (remaining_csc == 1) ? new ASTNode({TokenType::Function, "csc", MathFunc::csc}) : new ASTNode({TokenType::Operator, "^", MathFunc::None});
                    if (remaining_csc == 1)
                    {
                        cscRem->right = copyTree(uNode);
                    }
                    else
                    {
                        ASTNode *cb = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                        cb->right = copyTree(uNode);
                        cscRem->left = cb;
                        cscRem->right = new ASTNode({TokenType::Number, std::to_string(remaining_csc), MathFunc::None});
                    }
                    ASTNode *mul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    mul->left = cscRem;
                    mul->right = duNode;
                    restNodes = mul;
                }
                ASTNode *convertedFormula = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                convertedFormula->left = cotConverted;
                convertedFormula->right = restNodes;
                coeff_csc_cot = -coeff_csc_cot;
                if (coeff_csc_cot != 1.0)
                {
                    ASTNode *cNode = new ASTNode({TokenType::Number, formatDouble(coeff_csc_cot), MathFunc::None});
                    ASTNode *r = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    r->left = cNode;
                    r->right = convertedFormula;
                    convertedFormula = r;
                }
                ASTNode *simplifiedFinal = simplify(convertedFormula);
                ASTNode *result = integrate(simplifiedFinal, depth);
                deleteTree(simplifiedFinal);
                if (result)
                    return result;
            }
            else if (m_csc % 2 != 0 && n_cot >= 2 && n_cot % 2 == 0)
            {
                int k = n_cot / 2;
                ASTNode *cscNode = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                cscNode->right = copyTree(uNode);
                ASTNode *cscSq = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                cscSq->left = cscNode;
                cscSq->right = new ASTNode({TokenType::Number, "2", MathFunc::None});
                ASTNode *cscSqMinusOne = new ASTNode({TokenType::Operator, "-", MathFunc::None});
                cscSqMinusOne->left = cscSq;
                cscSqMinusOne->right = new ASTNode({TokenType::Number, "1", MathFunc::None});
                ASTNode *cotConverted = (k == 1) ? cscSqMinusOne : new ASTNode({TokenType::Operator, "^", MathFunc::None});
                if (k != 1)
                {
                    cotConverted->left = cscSqMinusOne;
                    cotConverted->right = new ASTNode({TokenType::Number, std::to_string(k), MathFunc::None});
                }
                ASTNode *restNodes = nullptr;
                if (m_csc == 1)
                {
                    restNodes = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                    restNodes->right = copyTree(uNode);
                }
                else
                {
                    restNodes = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                    ASTNode *cb = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                    cb->right = copyTree(uNode);
                    restNodes->left = cb;
                    restNodes->right = new ASTNode({TokenType::Number, std::to_string(m_csc), MathFunc::None});
                }
                ASTNode *convertedFormula = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                convertedFormula->left = cotConverted;
                convertedFormula->right = restNodes;
                if (coeff_csc_cot != 1.0)
                {
                    ASTNode *cNode = new ASTNode({TokenType::Number, formatDouble(coeff_csc_cot), MathFunc::None});
                    ASTNode *r = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    r->left = cNode;
                    r->right = convertedFormula;
                    convertedFormula = r;
                }
                ASTNode *simplifiedFinal = simplify(convertedFormula);
                ASTNode *result = integrate(simplifiedFinal, depth);
                deleteTree(simplifiedFinal);
                if (result)
                    return result;
            }
        }
    }
    {
        double coeff = 1.0;
        int cscIdx = -1, cotIdx = -1;
        int m_csc = 0, n_cot = 0;
        bool pureCscCot = true;
        ASTNode *uNode = nullptr;
        for (size_t i = 0; i < factors.size(); ++i)
        {
            ASTNode *f = factors[i];
            if (f->token.type == TokenType::Number)
            {
                coeff *= std::stod(f->token.value);
            }
            else if (f->token.value == "^" && f->left && f->left->token.value == "csc")
            {
                cscIdx = i;
                m_csc = std::round(std::stod(f->right->token.value));
                uNode = f->left->right;
            }
            else if (f->token.value == "^" && f->left && f->left->token.value == "cot")
            {
                cotIdx = i;
                n_cot = std::round(std::stod(f->right->token.value));
                uNode = f->left->right;
            }
            else if (f->token.type == TokenType::Function && f->token.value == "csc")
            {
                cscIdx = i;
                m_csc = 1;
                uNode = f->right;
            }
            else if (f->token.type == TokenType::Function && f->token.value == "cot")
            {
                cotIdx = i;
                n_cot = 1;
                uNode = f->right;
            }
            else
            {
                pureCscCot = false;
                break;
            }
        }
        if (pureCscCot && uNode)
        {
            if (n_cot == 1 && m_csc >= 1)
            {
                ASTNode *res = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                res->left = doubleToFractionAST(-coeff / m_csc);
                if (m_csc == 1)
                {
                    ASTNode *cscNode = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                    cscNode->right = copyTree(uNode);
                    res->right = cscNode;
                }
                else
                {
                    ASTNode *cscPow = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                    ASTNode *cscBase = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                    cscBase->right = copyTree(uNode);
                    cscPow->left = cscBase;
                    cscPow->right = new ASTNode({TokenType::Number, std::to_string(m_csc), MathFunc::None});
                    res->right = cscPow;
                }
                return res;
            }
            if (m_csc == 2 && n_cot >= 0)
            {
                ASTNode *res = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                res->left = doubleToFractionAST(-coeff / (n_cot + 1.0));
                ASTNode *cotPow = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode *cotBase = new ASTNode({TokenType::Function, "cot", MathFunc::cot});
                cotBase->right = copyTree(uNode);
                cotPow->left = cotBase;
                cotPow->right = new ASTNode({TokenType::Number, std::to_string(n_cot + 1), MathFunc::None});
                res->right = cotPow;
                if (n_cot + 1 == 1)
                {
                    cotPow->left = nullptr;
                    deleteTree(cotPow);
                    res->right = cotBase;
                }
                return res;
            }
            if (n_cot == 0 && m_csc >= 3 && m_csc % 2 != 0)
            {
                double coeff1 = -1.0 / (m_csc - 1.0);
                ASTNode *term1 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                term1->left = doubleToFractionAST(coeff1 * coeff);
                ASTNode *cscPow = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                ASTNode *cscBase = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                cscBase->right = copyTree(uNode);
                cscPow->left = cscBase;
                cscPow->right = new ASTNode({TokenType::Number, std::to_string(m_csc - 2), MathFunc::None});
                if (m_csc - 2 == 1)
                {
                    cscPow->left = nullptr;
                    deleteTree(cscPow);
                    cscPow = cscBase;
                }
                ASTNode *cotNode = new ASTNode({TokenType::Function, "cot", MathFunc::cot});
                cotNode->right = copyTree(uNode);
                ASTNode *trigMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                trigMul->left = cscPow;
                trigMul->right = cotNode;
                term1->right = trigMul;
                double coeff2 = (m_csc - 2.0) / (m_csc - 1.0);
                ASTNode *nextTarget = nullptr;
                if (m_csc - 2 == 1)
                {
                    nextTarget = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                    nextTarget->right = copyTree(uNode);
                }
                else
                {
                    nextTarget = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                    ASTNode *nb = new ASTNode({TokenType::Function, "csc", MathFunc::csc});
                    nb->right = copyTree(uNode);
                    nextTarget->left = nb;
                    nextTarget->right = new ASTNode({TokenType::Number, std::to_string(m_csc - 2), MathFunc::None});
                }
                ASTNode *integratedPart = integrate(nextTarget, depth + 1);
                deleteTree(nextTarget);
                if (integratedPart)
                {
                    ASTNode *term2 = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                    term2->left = doubleToFractionAST(coeff2 * coeff);
                    term2->right = integratedPart;
                    ASTNode *finalRes = new ASTNode({TokenType::Operator, "+", MathFunc::None});
                    finalRes->left = term1;
                    finalRes->right = term2;
                    return finalRes;
                }
                else
                {
                    deleteTree(term1);
                }
            }
        }
    }

    return nullptr;
}

// ===========================================================================
// 🌟 終極完全體：微積分核心積分引擎 (Integrate Engine)
// ===========================================================================
ASTNode *integrate(ASTNode *node, int depth)
{
    if (node == nullptr || depth > 40)
        return nullptr;
    integrate_call_count++;
    // ==========================================
    // 【防線 1】隱形單位元注入 (拯救孤立的 ln, arcsin 等)
    // ==========================================
    if (node->token.value != "*" && node->token.value != "+" && node->token.value != "-" && node->token.value != "/")
    {
        if (ASTNode *res = handleImplicitByParts(node, depth))
            return res;
    }

    // ==========================================
    // 【防線 2】純常數與特種查表 (例如 e*x, e^(ax)sin(bx))
    // ==========================================
    if (isConstant(node))
    {
        ASTNode *mulNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
        mulNode->left = copyTree(node);
        mulNode->right = new ASTNode({TokenType::Variable, "x", MathFunc::None});
        return mulNode;
    }
    if (node->token.type == TokenType::Operator && node->token.value == "*")
    {
        ExpTrigMatch match = matchExpTrigPattern(node);
        if (match.isMatched)
            return buildExpTrigResult(match);
    }

    // ==========================================
    // 【防線 3】基本查表與線性法則 (加減法拆解)
    // ==========================================
    ASTNode *result = nullptr;
    if ((result = tableIntegral(node)))
        return result;
    if ((result = linearityIntegral(node, depth)))
        return result;

    // ==========================================
    // 【防線 4】分數與有理函數處理 (Irreducible Quad, Partial Fractions, Long Div)
    // ==========================================
    if (node->token.type == TokenType::Operator && (node->token.value == "/" || node->token.value == "*"))
    {
        if (ASTNode *res = handleRationalFunction(node, depth))
            return res;
    }

    // ==========================================
    // 【防線 5】正統三角代換 (Trig Substitution)
    // ==========================================
    if (ASTNode *res = tryTrigSubstitution(node, depth))
        return res;

    // ==========================================
    // 【防線 6】乘法 (*) 與 次方 (^) 專用複合戰略
    // ==========================================
    if (node->token.type == TokenType::Operator && (node->token.value == "*" || node->token.value == "^"))
    {
        // 6-A：常數提取 (僅限乘法)
        if (node->token.value == "*")
        {
            if (isConstant(node->left))
            {
                ASTNode *cRes = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                cRes->left = copyTree(node->left);
                cRes->right = integrate(node->right, depth);
                if (cRes->right != nullptr)
                    return cRes;
                delete cRes->left;
                delete cRes;
            }
            else if (isConstant(node->right))
            {
                ASTNode *cRes = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                cRes->right = copyTree(node->right);
                cRes->left = integrate(node->left, depth);
                if (cRes->left != nullptr)
                    return cRes;
                delete cRes->right;
                delete cRes;
            }
        }

        // 打平樹枝，將連續乘積收集至清單
        std::vector<ASTNode *> factors;
        collectFactors(node, factors);

        // 6-B：變數變換 (U-Substitution)
        if (!USE_REDUCTION_FORMULA)
        {
            if (ASTNode *res = handleUSubstitution(node, factors, depth))
                return res;
        }

        // 6-C ~ 6-G：三角多項式複合打擊
        if (ASTNode *res = handleTrigPolynomials(node, factors, depth))
            return res;

        // 6-H：分部積分 (Integration By Parts - 僅限乘法，最後大絕招)
        if (node->token.value == "*")
        {
            if (ASTNode *res = integrationByParts(node, depth + 1))
                return res;
        }
    }

    // ==========================================
    // 【防線 7】頂層降維打擊 (暴力代數展開，僅限深度 0)
    // ==========================================
    if (depth == 0)
    {
        ASTNode *expandedTree = expand(copyTree(node));
        ASTNode *simplifiedExpanded = simplify(expandedTree);
        if (!isSameTree(node, simplifiedExpanded))
        {
            ASTNode *finalResult = integrate(simplifiedExpanded, 1);
            deleteTree(simplifiedExpanded);
            if (finalResult != nullptr)
                return finalResult;
        }
        else
        {
            deleteTree(simplifiedExpanded);
        }
    }

    // 所有的招數都用盡了，宣告無法積分
    return nullptr;
}

// =======================================================
// 🌟 救援小隊：分數轉換與印出前處理
// =======================================================

// 將小數轉化為分數節點
ASTNode *doubleToFractionAST(double val)
{
    if (std::abs(val - std::round(val)) < 1e-9)
    {
        return new ASTNode({TokenType::Number, formatDouble(std::round(val)), MathFunc::None});
    }
    int sign = (val < 0) ? -1 : 1;
    val = std::abs(val);
    double tolerance = 1.0E-6;
    double h1 = 1, h2 = 0, k1 = 0, k2 = 1, b = val;
    do
    {
        double a = std::floor(b);
        double aux = h1;
        h1 = a * h1 + h2;
        h2 = aux;
        aux = k1;
        k1 = a * k1 + k2;
        k2 = aux;
        if (b - a < tolerance)
            break;
        b = 1.0 / (b - a);
    } while (std::abs(val - h1 / k1) > val * tolerance && k1 < 10000);

    int num = sign * static_cast<int>(std::round(h1));
    int den = static_cast<int>(std::round(k1));

    if (den == 1)
        return new ASTNode({TokenType::Number, std::to_string(num), MathFunc::None});

    ASTNode *divNode = new ASTNode({TokenType::Operator, "/", MathFunc::None});
    divNode->left = new ASTNode({TokenType::Number, std::to_string(num), MathFunc::None});
    divNode->right = new ASTNode({TokenType::Number, std::to_string(den), MathFunc::None});
    return divNode;
}

// 印出前的最後一道手續：掃描全樹，將小數節點置換為分數節點
ASTNode *postProcessFractions(ASTNode *node)
{
    if (!node)
        return nullptr;
    node->left = postProcessFractions(node->left);
    node->right = postProcessFractions(node->right);
    if (node->token.type == TokenType::Number)
    {
        double val = std::stod(node->token.value);
        if (std::abs(val - std::round(val)) > 1e-9)
        {
            ASTNode *fracNode = doubleToFractionAST(val);
            deleteTree(node);
            return fracNode;
        }
    }
    return node;
}

struct PolyTerm
{
    double coeff;
    ASTNode *base;
};
void extractTerms(ASTNode *node, std::vector<PolyTerm> &terms, double sign)
{
    if (!node)
        return;
    if (node->token.value == "+")
    {
        extractTerms(node->left, terms, sign);
        extractTerms(node->right, terms, sign);
    }
    else if (node->token.value == "-")
    {
        // 處理減法與括號負號： -(A+B) 會把 -1 的 sign 往右邊傳遞
        if (!node->left || (node->left->token.type == TokenType::Number && node->left->token.value == "0"))
        {
            extractTerms(node->right, terms, -sign);
        }
        else
        {
            extractTerms(node->left, terms, sign);
            extractTerms(node->right, terms, -sign);
        }
    }
    else
    {
        // 抵達葉節點，萃取係數與變數基底
        double c = sign;
        ASTNode *base = node;
        if (node->token.value == "*")
        {
            if (node->left && node->left->token.type == TokenType::Number)
            {
                c *= std::stod(node->left->token.value);
                base = node->right;
            }
            else if (node->right && node->right->token.type == TokenType::Number)
            {
                c *= std::stod(node->right->token.value);
                base = node->left;
            }
        }
        else if (node->token.type == TokenType::Number)
        {
            c *= std::stod(node->token.value);
            base = nullptr; // nullptr 代表純數字常數
        }
        terms.push_back({c, base});
    }
}
// =======================================================
// 🌟 幕後代打函數：真正執行化簡的核心 (帶有深度追蹤)
// =======================================================
ASTNode *simplifyHelper(ASTNode *node, int depth)
{
    if (node == nullptr)
        return nullptr;

    // 🛡️ 真正的深度防護，防止 Stack Overflow
    if (depth > 500)
        return node;

    node->left = simplifyHelper(node->left, depth + 1);
    node->right = simplifyHelper(node->right, depth + 1);

    if (node->token.type == TokenType::Number || node->token.type == TokenType::Variable || node->token.type == TokenType::Constant)
    {
        return node;
    }

    // =======================================================
    // 🌟 函數 (Function) 化簡
    // =======================================================
    if (node->token.type == TokenType::Function)
    {
        if (node->token.value == "ln" && node->right)
        {
            if (node->right->token.type == TokenType::Number && std::stod(node->right->token.value) == 1.0)
            {
                deleteTree(node);
                return new ASTNode({TokenType::Number, "0", MathFunc::None});
            }
            bool isEuler = false;
            if (node->right->token.value == "e" || node->right->token.value == "E")
                isEuler = true;
            if (node->right->token.type == TokenType::Number && std::abs(std::stod(node->right->token.value) - exp(1.0)) < 1e-9)
                isEuler = true;
            if (isEuler)
            {
                deleteTree(node);
                return new ASTNode({TokenType::Number, "1", MathFunc::None});
            }
            if (node->right->token.type == TokenType::Operator && node->right->token.value == "^")
            {
                ASTNode *baseNode = node->right->left;
                bool isBaseEuler = false;
                if (baseNode && (baseNode->token.value == "e" || baseNode->token.value == "E"))
                    isBaseEuler = true;
                if (baseNode && baseNode->token.type == TokenType::Number && std::abs(std::stod(baseNode->token.value) - exp(1.0)) < 1e-9)
                    isBaseEuler = true;
                if (isBaseEuler)
                {
                    ASTNode *keepNode = copyTree(node->right->right);
                    deleteTree(node);
                    return simplifyHelper(keepNode, depth);
                }
            }
        }
        if (node->right && node->right->token.type == TokenType::Number)
        {
            double val = stod(node->right->token.value);
            double result = 0;
            bool computable = false;
            string funcName = node->token.value;
            if (funcName == "ln" && val > 0)
            {
                result = log(val);
                computable = true;
            }
            else if (funcName == "log" && val > 0)
            {
                result = log10(val);
                computable = true;
            }
            else if (funcName == "sin")
            {
                result = sin(val);
                computable = true;
            }
            else if (funcName == "cos")
            {
                result = cos(val);
                computable = true;
            }

            if (computable && abs(result - round(result)) < 1e-9)
            {
                result = round(result);
                deleteTree(node);
                return new ASTNode({TokenType::Number, formatDouble(result), MathFunc::None});
            }
        }
        return node;
    }

    string op = node->token.value;

    // =======================================================
    // 🌟 純數字運算
    // =======================================================
    if (node->left && node->right && node->left->token.type == TokenType::Number && node->right->token.type == TokenType::Number)
    {
        double l_val = stod(node->left->token.value);
        double r_val = stod(node->right->token.value);
        double result = 0;
        if (op == "+")
            result = l_val + r_val;
        else if (op == "-")
            result = l_val - r_val;
        else if (op == "*")
            result = l_val * r_val;
        else if (op == "/")
        {
            if (std::abs(r_val) < 1e-9)
                return node; // 避免除以 0
            result = l_val / r_val;
        }
        else if (op == "^")
            result = pow(l_val, r_val);

        deleteTree(node);
        return new ASTNode({TokenType::Number, formatDouble(result), MathFunc::None});
    }

    // =======================================================
    // 🌟 運算符號化簡：除法 (/)
    // =======================================================
    if (op == "/")
    {
        if (node->left && node->left->token.value == "0")
        {
            deleteTree(node);
            return new ASTNode({TokenType::Number, "0", MathFunc::None});
        }
        if (node->right && node->right->token.value == "1")
        {
            ASTNode *keep = node->left;
            node->left = nullptr;
            deleteTree(node);
            return keep;
        }
        if (isSameTree(node->left, node->right))
        {
            deleteTree(node);
            return new ASTNode({TokenType::Number, "1", MathFunc::None});
        }

        // 🚀 破解分母盲區： A / (B * C) 拆解成 (A / B) * (1 / C)
        if (node->right && node->right->token.value == "*")
        {
            ASTNode *newMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
            ASTNode *div1 = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            div1->left = copyTree(node->left);
            div1->right = copyTree(node->right->left);
            ASTNode *div2 = new ASTNode({TokenType::Operator, "/", MathFunc::None});
            div2->left = new ASTNode({TokenType::Number, "1", MathFunc::None});
            div2->right = copyTree(node->right->right);
            newMul->left = div1;
            newMul->right = div2;
            deleteTree(node);
            return simplifyHelper(newMul, depth);
        }

        if (node->right && node->right->token.type == TokenType::Number)
        {
            double denom = std::stod(node->right->token.value);
            if (std::abs(denom) > 1e-9)
            {
                ASTNode *newMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                newMul->left = new ASTNode({TokenType::Number, formatDouble(1.0 / denom), MathFunc::None});
                newMul->right = copyTree(node->left);
                deleteTree(node);
                return simplifyHelper(newMul, depth);
            }
        }
    }

    // =======================================================
    // 🌟 運算符號化簡：乘法 (*)
    // =======================================================
    if (op == "*")
    {
        if ((node->left && node->left->token.value == "0") || (node->right && node->right->token.value == "0"))
        {
            deleteTree(node);
            return new ASTNode({TokenType::Number, "0", MathFunc::None});
        }
        if (node->left && node->left->token.value == "1")
        {
            ASTNode *keep = node->right;
            node->right = nullptr;
            deleteTree(node);
            return keep;
        }
        if (node->right && node->right->token.value == "1")
        {
            ASTNode *keep = node->left;
            node->left = nullptr;
            deleteTree(node);
            return keep;
        }

        std::vector<ASTNode *> factors;
        collectFactors(node, factors);
        double final_coeff = 1.0;

        struct BasePower
        {
            ASTNode *base;
            double power;
        };
        std::vector<BasePower> merged;

        // 🚀 新增裝備：三角倒數辨識雷達
        auto isReciprocal = [](ASTNode *b1, ASTNode *b2)
        {
            if (!b1 || !b2)
                return false;
            if (b1->token.type != TokenType::Function || b2->token.type != TokenType::Function)
                return false;
            if (!isSameTree(b1->right, b2->right))
                return false;
            string f1 = b1->token.value, f2 = b2->token.value;
            return (f1 == "sin" && f2 == "csc") || (f1 == "csc" && f2 == "sin") ||
                   (f1 == "cos" && f2 == "sec") || (f1 == "sec" && f2 == "cos") ||
                   (f1 == "tan" && f2 == "cot") || (f1 == "cot" && f2 == "tan");
        };

        for (ASTNode *f : factors)
        {
            if (f->token.type == TokenType::Number)
            {
                final_coeff *= std::stod(f->token.value);
            }
            else
            {
                ASTNode *current_base = f;
                double current_power = 1.0;

                if (f->token.type == TokenType::Operator && f->token.value == "^" && f->right && f->right->token.type == TokenType::Number)
                {
                    current_base = f->left;
                    current_power = std::stod(f->right->token.value);
                }
                else if (f->token.type == TokenType::Operator && f->token.value == "/" &&
                         f->left && f->left->token.type == TokenType::Number)
                {
                    final_coeff *= std::stod(f->left->token.value);
                    ASTNode *denom = f->right;
                    if (denom->token.type == TokenType::Operator && denom->token.value == "^" &&
                        denom->right && denom->right->token.type == TokenType::Number)
                    {
                        current_base = denom->left;
                        current_power = -std::stod(denom->right->token.value);
                    }
                    else
                    {
                        current_base = denom;
                        current_power = -1.0;
                    }
                }

                bool found = false;
                for (auto &mp : merged)
                {
                    if (isSameTree(mp.base, current_base))
                    {
                        mp.power += current_power;
                        found = true;
                        break;
                    }
                    // 🚀 核心升級：倒數相遇，直接發動次方的反物質湮滅！
                    else if (isReciprocal(mp.base, current_base))
                    {
                        mp.power -= current_power;
                        found = true;
                        break;
                    }
                }
                if (!found)
                    merged.push_back({current_base, current_power});
            }
        }

        std::vector<ASTNode *> new_factors;
        if (std::abs(final_coeff - 1.0) > 1e-9 || merged.empty())
        {
            new_factors.push_back(new ASTNode({TokenType::Number, formatDouble(final_coeff), MathFunc::None}));
        }
        for (const auto &mp : merged)
        {
            if (std::abs(mp.power) < 1e-9)
                continue;
            if (std::abs(mp.power - 1.0) < 1e-9)
            {
                new_factors.push_back(copyTree(mp.base));
            }
            else
            {
                ASTNode *powNode = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                powNode->left = copyTree(mp.base);
                powNode->right = new ASTNode({TokenType::Number, formatDouble(mp.power), MathFunc::None});
                new_factors.push_back(powNode);
            }
        }

        ASTNode *rebuilt = rebuildProduct(new_factors);
        for (ASTNode *nf : new_factors)
        {
            deleteTree(nf);
        }

        if (!isSameTree(node, rebuilt))
        {
            deleteTree(node);
            return simplifyHelper(rebuilt, depth);
        }
        else
        {
            deleteTree(rebuilt);
        }
    }

    // =======================================================
    // 🌟 運算符號化簡：加法 (+) & 減法 (-)
    // =======================================================
    if (op == "+" || op == "-")
    {
        std::vector<PolyTerm> terms;
        extractTerms(node, terms, 1.0);
        std::vector<PolyTerm> merged;
        double const_sum = 0.0;
        for (const auto &t : terms)
        {
            if (t.base == nullptr)
            {
                const_sum += t.coeff;
            }
            else
            {
                bool found = false;
                for (auto &m : merged)
                {
                    if (isSameTree(m.base, t.base))
                    {
                        m.coeff += t.coeff;
                        found = true;
                        break;
                    }
                }
                if (!found)
                    merged.push_back({t.coeff, t.base});
            }
        }

        ASTNode *rebuilt = nullptr;
        auto addTerm = [&](ASTNode *termNode, double coeff)
        {
            if (!rebuilt)
            {
                if (coeff < 0)
                {
                    if (termNode->token.value == "*" && termNode->left && termNode->left->token.type == TokenType::Number)
                    {
                        deleteTree(termNode->left);
                        termNode->left = new ASTNode({TokenType::Number, formatDouble(coeff), MathFunc::None});
                        rebuilt = termNode;
                    }
                    else
                    {
                        ASTNode *negMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                        negMul->left = new ASTNode({TokenType::Number, "-1", MathFunc::None});
                        negMul->right = termNode;
                        rebuilt = negMul;
                    }
                }
                else
                {
                    rebuilt = termNode;
                }
            }
            else
            {
                ASTNode *opNode = new ASTNode({TokenType::Operator, (coeff < 0) ? "-" : "+", MathFunc::None});
                opNode->left = rebuilt;
                opNode->right = termNode;
                rebuilt = opNode;
            }
        };

        if (std::abs(const_sum) > 1e-9)
        {
            ASTNode *numNode = new ASTNode({TokenType::Number, formatDouble(std::abs(const_sum)), MathFunc::None});
            addTerm(numNode, const_sum);
        }
        for (const auto &m : merged)
        {
            if (std::abs(m.coeff) < 1e-9)
                continue;
            ASTNode *baseCopy = copyTree(m.base);
            ASTNode *termNode = nullptr;
            double absCoeff = std::abs(m.coeff);
            if (std::abs(absCoeff - 1.0) < 1e-9)
            {
                termNode = baseCopy;
            }
            else
            {
                termNode = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                termNode->left = new ASTNode({TokenType::Number, formatDouble(absCoeff), MathFunc::None});
                termNode->right = baseCopy;
            }
            addTerm(termNode, m.coeff);
        }

        if (!rebuilt)
            rebuilt = new ASTNode({TokenType::Number, "0", MathFunc::None});
        deleteTree(node);
        return rebuilt;
    }

    // =======================================================
    // 🌟 運算符號化簡：次方 (^)
    // =======================================================
    if (op == "^")
    {
        // 🚀 破解三角函數倒數盲區：將 cos(x)^-n 自動轉換為 sec(x)^n
        if (node->left && node->left->token.type == TokenType::Function &&
            node->right && node->right->token.type == TokenType::Number)
        {
            double p = std::stod(node->right->token.value);
            if (p < 0)
            {
                string fn = node->left->token.value;
                string newFn = "";
                if (fn == "cos")
                    newFn = "sec";
                else if (fn == "sin")
                    newFn = "csc";
                else if (fn == "tan")
                    newFn = "cot";
                else if (fn == "sec")
                    newFn = "cos";
                else if (fn == "csc")
                    newFn = "sin";
                else if (fn == "cot")
                    newFn = "tan";

                if (!newFn.empty())
                {
                    ASTNode *newPow = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                    newPow->left = new ASTNode({TokenType::Function, newFn, strToMathFunc(newFn)});
                    if (node->left->right)
                    {
                        newPow->left->right = copyTree(node->left->right);
                    }
                    newPow->right = new ASTNode({TokenType::Number, formatDouble(-p), MathFunc::None});
                    deleteTree(node);
                    return simplifyHelper(newPow, depth);
                }
            }
        }

        // 🚀 加入 (A^B)^C = A^(B*C) 的指數律
        if (node->left && node->left->token.type == TokenType::Operator && node->left->token.value == "^")
        {
            if (node->left->right && node->left->right->token.type == TokenType::Number &&
                node->right && node->right->token.type == TokenType::Number)
            {
                double power1 = std::stod(node->left->right->token.value);
                double power2 = std::stod(node->right->token.value);
                ASTNode *newPow = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                newPow->left = copyTree(node->left->left);
                newPow->right = new ASTNode({TokenType::Number, formatDouble(power1 * power2), MathFunc::None});
                deleteTree(node);
                return simplifyHelper(newPow, depth);
            }
        }

        if (node->right && node->right->token.value == "1")
        {
            ASTNode *keep = node->left;
            node->left = nullptr;
            deleteTree(node);
            return keep;
        }

        if (node->right && node->right->token.value == "0.5")
        {
            if (node->left && node->left->token.type == TokenType::Operator && node->left->token.value == "*")
            {
                ASTNode *innerMul = node->left;
                if (innerMul->left && innerMul->left->token.type == TokenType::Number &&
                    innerMul->right && innerMul->right->token.type == TokenType::Operator && innerMul->right->token.value == "^" &&
                    innerMul->right->right && innerMul->right->right->token.value == "2")
                {
                    double A = std::stod(innerMul->left->token.value);
                    if (A >= 0)
                    {
                        ASTNode *res = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                        res->left = new ASTNode({TokenType::Number, formatDouble(sqrt(A)), MathFunc::None});
                        res->right = copyTree(innerMul->right->left);
                        deleteTree(node);
                        return res;
                    }
                }
            }
        }
        if (node->left && node->left->token.type == TokenType::Operator && node->left->token.value == "*")
        {
            ASTNode *innerMul = node->left;
            if (innerMul->left && innerMul->left->token.type == TokenType::Number && node->right && node->right->token.type == TokenType::Number)
            {
                double base_c = std::stod(innerMul->left->token.value);
                double power = std::stod(node->right->token.value);
                ASTNode *newMul = new ASTNode({TokenType::Operator, "*", MathFunc::None});
                newMul->left = new ASTNode({TokenType::Number, formatDouble(std::pow(base_c, power)), MathFunc::None});
                ASTNode *newPow = new ASTNode({TokenType::Operator, "^", MathFunc::None});
                newPow->left = copyTree(innerMul->right);
                newPow->right = copyTree(node->right);
                newMul->right = newPow;
                deleteTree(node);
                return newMul;
            }
        }
    }

    return node;
}

// =======================================================
// 🌟 對外開放的主介面 (完美適配原有的 .hpp)
// =======================================================
ASTNode *simplify(ASTNode *node)
{
    // 直接呼叫 Helper 函數，並設定初始深度為 0
    return simplifyHelper(node, 0);
}