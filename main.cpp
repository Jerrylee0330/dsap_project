#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include "AST.hpp"
#include "parser.hpp"
#include "calculus.hpp"

using namespace std;

extern void runBenchmark();

int main(int argc, char *argv[])
{
    system("chcp 65001 > nul");
    // ==========================================
    // 🚀 隱藏密技：透過指令直接啟動 (Command Line Args)
    // 只要在終端機輸入 .\engine.exe test 就會直接進入測速
    // ==========================================
    if (argc > 1)
    {
        std::string mode = argv[1];
        if (mode == "test" || mode == "benchmark")
        {
            runBenchmark();
            return 0; // 測速完直接結束程式
        }
    }

    // ==========================================
    // 🖥️ 開機選單：一般啟動時的畫面
    // ==========================================
    std::cout << "=================================================\n";
    std::cout << " 🚀 C++ 微積分引擎 (Calculus Engine) 終極版\n";
    std::cout << "=================================================\n";
    std::cout << "請選擇運行模式：\n";
    std::cout << "  [1] 🧮 互動計算機模式 (Calculator Mode)\n";
    std::cout << "  [2] ⏱️ 極限效能測速模式 (Benchmark Mode)\n";
    std::cout << "請輸入 1 或 2: ";

    std::string choice;
    std::getline(std::cin, choice);
    std::cout << "\n";

    if (choice == "2")
    {
        // 進入戰術二：測速模式
        runBenchmark();
    }
    else
    {
        // ==========================================
        // 進入戰術一：原版的互動計算機模式
        // ==========================================
        std::cout << "--- 進入互動計算機模式 (輸入 exit 離開) ---\n";

        while (true)
        {
            std::cout << "\nEnter a function of x (or type 'exit' to quit): ";
            std::string input;
            std::getline(std::cin, input);

            if (input == "exit" || input == "quit")
            {
                std::cout << "Engine Shutdown. Goodbye!\n";
                break;
            }
            if (input.empty())
            {
                continue;
            }

            // 1. 解析字串
            // (如果您原本在 lexer 之前還有 Format String 的函數，請加在這裡)
            std::vector<Token> tokens = tokenize(input);
            std::vector<Token> postfix = infixToPostfix(tokens);
            ASTNode *root = buildAST(postfix);

            if (!root)
            {
                std::cout << "[Error] 無法建立語法樹，請檢查輸入格式或括號是否對稱！\n";
                continue;
            }

            std::cout << "\n-------------------------------------------------\n";
            std::cout << "[Original] f(x) = " << treeToString(root) << "\n";

            // 2. 啟動微分引擎
            ASTNode *diff = derivative(copyTree(root));
            ASTNode *simDiff = simplify(diff);
            simDiff = postProcessFractions(simDiff); // 分數轉換
            std::cout << "[Derivative] f'(x) = " << (simDiff ? treeToString(simDiff) : "0") << "\n";
            deleteTree(simDiff); // 清理記憶體

            // 3. 啟動積分引擎
            ASTNode *integ = integrate(copyTree(root), 0);
            ASTNode *simInteg = simplify(integ);
            simInteg = postProcessFractions(simInteg); // 分數轉換

            if (simInteg)
            {
                std::cout << "[Integral] ∫ f(x) dx = " << treeToString(simInteg) << " + C\n";
            }
            else
            {
                std::cout << "[Integral] 引擎判定目前無法積分此函數\n";
            }
            deleteTree(simInteg); // 清理記憶體

            // 4. 清理原始語法樹
            deleteTree(root);
            std::cout << "-------------------------------------------------\n";
        }
    }

    return 0;
}