#include <string>
#include <functional>

extern "C" {
#include <libavutil/log.h>
#include <libavformat/avformat.h>
}

#include "testAvLog.h"

namespace TEST_AV_LOG { 

// 测试日志输出基本功能
void TestLogOutputBasic() {
    auto fmtCtx = avformat_alloc_context();

    av_log(fmtCtx, AV_LOG_PANIC,   "Panic: Something went really wrong and we will crash now.\n");
	av_log(fmtCtx, AV_LOG_FATAL,   "Fatal: Something went wrong and recovery is not possible.\n");
	av_log(fmtCtx, AV_LOG_ERROR,   "Error: Something went wrong and cannot losslessly be recovered.\n");
	av_log(fmtCtx, AV_LOG_WARNING, "Warning: This may or may not lead to problems.\n");
	av_log(fmtCtx, AV_LOG_INFO,    "Info: Standard information.\n");

    // 以下内容不会输出，因为默认日志级别为：AV_LOG_INFO
	av_log(fmtCtx, AV_LOG_VERBOSE, "Verbose: Detailed information.\n");
	av_log(fmtCtx, AV_LOG_DEBUG,   "Debug: Stuff which is only useful for libav* developers.\n");

    avformat_free_context(fmtCtx);
}

// 测试设置日志输出级别功能
void TestSetLogLevel() {
    auto fmtCtx = avformat_alloc_context();

    // 获取默认日志级别
    int logLevel = av_log_get_level();

    // 设置日志级别
    av_log_set_level(AV_LOG_DEBUG);

    av_log(fmtCtx, AV_LOG_INFO,    "Old log level: %d. New log level: %d\n", logLevel, AV_LOG_DEBUG);
    av_log(fmtCtx, AV_LOG_PANIC,   "Panic: Something went really wrong and we will crash now.\n");
	av_log(fmtCtx, AV_LOG_FATAL,   "Fatal: Something went wrong and recovery is not possible.\n");
	av_log(fmtCtx, AV_LOG_ERROR,   "Error: Something went wrong and cannot losslessly be recovered.\n");
	av_log(fmtCtx, AV_LOG_WARNING, "Warning: This may or may not lead to problems.\n");
	av_log(fmtCtx, AV_LOG_INFO,    "Info: Standard information.\n");
	av_log(fmtCtx, AV_LOG_VERBOSE, "Verbose: Detailed information.\n");
	av_log(fmtCtx, AV_LOG_DEBUG,   "Debug: Stuff which is only useful for libav* developers.\n");

    avformat_free_context(fmtCtx);
}

} // namespace TEST_AV_LOG