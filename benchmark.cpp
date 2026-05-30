#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <iomanip>
#include "calculus.hpp"
#include "parser.hpp"

extern bool USE_REDUCTION_FORMULA;
extern int integrate_call_count;

struct TestCase
{
    std::string problem;
    int iterations;
};

// 📊 報表系統：時間與演算法步數的雙軌判定
void printReport(std::string mode, std::chrono::duration<double, std::milli> totalTime, int iterations, double &avgTimeRef, double &avgCallsRef)
{
    double avgTime = totalTime.count() / iterations;
    double avgCalls = (double)integrate_call_count / iterations;

    avgTimeRef = avgTime;
    avgCallsRef = avgCalls;

    std::cout << "  --------------------------------------------------\n";
    std::cout << "  模式: " << mode << "\n";
    std::cout << "  - 執行次數: " << iterations << "\n";
    std::cout << "  - 總耗時: " << std::fixed << std::setprecision(4) << totalTime.count() << " ms\n";
    std::cout << "  - ⏳ 平均單次耗時: " << std::setprecision(4) << avgTime << " ms\n";
    std::cout << "  - 🎯 平均遞迴步數: " << std::setprecision(1) << avgCalls << " 次/題\n";
    std::cout << "  --------------------------------------------------\n";
}

void runBenchmark()
{
    std::vector<TestCase> testCases = {
        {"sec(x)^3 * tan(x)^3", 1},
        {"sec(x)^3 * tan(x)^3", 100},
        {"sec(x)^3 * tan(x)^3", 10000},
        {"sec(x)^11 * tan(x)^11", 1},
        {"sec(x)^11 * tan(x)^11", 10},
        {"sec(x)^11 * tan(x)^11", 100},
        {"sec(x)^21 * tan(x)^21", 1},
        {"sec(x)^21 * tan(x)^21", 10}};

    std::cout << "\n========================================================\n";
    std::cout << " 🚀 啟動演算法極限測速 (時間複雜度檢測)\n";
    std::cout << "========================================================\n";

    for (const auto &tc : testCases)
    {
        std::string input = tc.problem;
        int iters = tc.iterations;

        std::cout << "\n========================================================\n";
        std::cout << " 🎯 測試題目: " << input << " (執行 " << iters << " 次)\n";
        std::cout << "========================================================\n";

        std::vector<Token> tokens = tokenize(input);
        std::vector<Token> postfix = infixToPostfix(tokens);

        double avgTime1 = 0.0, avgTime2 = 0.0;
        double calls1 = 0.0, calls2 = 0.0;

        // ==========================================
        // --- 戰術一：代換展開法 ---
        // ==========================================
        USE_REDUCTION_FORMULA = false;
        integrate_call_count = 0; // 🚀 測速前，將計步器歸零

        auto start1 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iters; i++)
        {
            ASTNode *root = buildAST(postfix);
            ASTNode *res = integrate(root, 0);
            deleteTree(res);
            deleteTree(root);
        }
        auto end1 = std::chrono::high_resolution_clock::now();
        printReport("代換展開法 (戰術一)", end1 - start1, iters, avgTime1, calls1);

        // ==========================================
        // --- 戰術二：遞迴降次法 ---
        // ==========================================
        USE_REDUCTION_FORMULA = true;
        integrate_call_count = 0; // 🚀 測速前，將計步器歸零

        auto start2 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iters; i++)
        {
            ASTNode *root = buildAST(postfix);
            ASTNode *res = integrate(root, 0);
            deleteTree(res);
            deleteTree(root);
        }
        auto end2 = std::chrono::high_resolution_clock::now();
        printReport("遞迴降次法 (戰術二)", end2 - start2, iters, avgTime2, calls2);

        // ==========================================
        // 🏆 最終戰況判定官
        // ==========================================
        std::cout << "  🏆 [最終戰況綜合判定]\n";
        std::cout << "  -> ⏳ 運算速度勝出: " << (avgTime1 < avgTime2 ? "【戰術一】代換法" : "【戰術二】公式法") << "\n";
        std::cout << "  -> 🎯 演算法極限勝出: " << (calls1 < calls2 ? "【戰術一】代換法" : "【戰術二】公式法") << " (步數較少)\n";
        std::cout << "========================================================\n\n";
    }
}
