#include "testAvIO.h"
#include "testAvLog.h"
#include "testAvFormat.h"
#include "testAvParseUtil.h"
#include "testAvDictionary.h"

// 测试av_log功能
void FuncTestAvLog() {
    // 测试日志输出基本功能
    TEST_AV_LOG::TestLogOutputBasic();

    // 测试设置日志输出级别功能
    TEST_AV_LOG::TestSetLogLevel();
}

// 测试AVDictionary功能
void FuncTestAVDictionary() {
    // 测试字典基本功能
    TEST_AV_DICTIONARY::TestDictionaryBasic();

    // 测试字典遍历功能
    TEST_AV_DICTIONARY::TestDictionaryForeach();
}

// 测试AVParseUtil功能
void FuncTestAVParseUtil() {
    // 测试视频分辨率解析功能
    TEST_AV_PARSE_UTIL::TestParseVideoSize();

    // 测试视频帧率解析功能
    TEST_AV_PARSE_UTIL::TestParseVideoRate();

    // 测试时间解析功能
    TEST_AV_PARSE_UTIL::TestParseTime();
}

// 测试AVIO功能
void FuncTestAVIO() {
    // 测试打开mp4文件功能
    TEST_AV_IO::TestOpenMp4File();

    // 测试打开rtsp流功能
    TEST_AV_IO::TestOpenRtspStream();

    // 测试打开自定义I/O
    TEST_AV_IO::TestOpenSelfDefineIO();
}

// 测试AVFormat功能
void FuncTestAVFormat() {
    // 测试显示流信息功能
    // TEST_AV_FORMAT::TestShowStreamInfo();

    // 测试输出文件的前100条帧信息
    // TEST_AV_FORMAT::TestShowFstHundredFrames();

    // 测试将mp4文件重新解封装/封装为flv文件
    TEST_AV_FORMAT::TestRemuxMp4ToFlv();
}

int main() { 
    // 测试av_log功能
    // FuncTestAvLog();

    // 测试AVDictionary功能
    // FuncTestAVDictionary();

    // 测试AVParseUtil功能
    // FuncTestAVParseUtil();

    // 测试AVIO功能
    // FuncTestAVIO();

    // 测试AVFormat功能
    FuncTestAVFormat();

    return 0;
}