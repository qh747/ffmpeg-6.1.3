#include "testAvLog.h"

// 测试av_log功能
void FuncTestAvLog() {
    // 测试日志输出基本功能
    // TEST_AV_LOG::TestLogOutputBasic();

    // 测试设置日志输出级别功能
    TEST_AV_LOG::TestSetLogLevel();
}

int main() { 
    FuncTestAvLog();

    return 0;
}