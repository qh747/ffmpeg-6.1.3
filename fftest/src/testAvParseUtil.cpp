#include <cstdint>

extern "C" {
#include <libavutil/log.h>
#include <libavutil/rational.h>
#include <libavutil/parseutils.h>
}

#include "testAvParseUtil.h"

namespace TEST_AV_PARSE_UTIL { 

// 测试视频分辨率解析功能
void TestParseVideoSize() {
    int width  = 0;
    int height = 0;

    if (av_parse_video_size(&width, &height, "1920*1080") >= 0) {
        av_log(nullptr, AV_LOG_INFO, "Parse 1920x1080. Result width: %d, height: %d\n", width, height);
    }

    if (av_parse_video_size(&width, &height, "vga") >= 0) {
        av_log(nullptr, AV_LOG_INFO, "Parse vga. Result width: %d, height: %d\n", width, height);
    }

    if (av_parse_video_size(&width, &height, "hd1080") >= 0) {
        av_log(nullptr, AV_LOG_INFO, "Parse hd1080. Result width: %d, height: %d\n", width, height);
    }

    if (av_parse_video_size(&width, &height, "ntsc") >= 0) {
        av_log(nullptr, AV_LOG_INFO, "Parse ntsc. Result width: %d, height: %d\n", width, height);
    }

    if (av_parse_video_size(&width, &height, "pal") >= 0) {
        av_log(nullptr, AV_LOG_INFO, "Parse pal. Result width: %d, height: %d\n", width, height);
    }
}

// 测试视频帧率解析功能
void TestParseVideoRate() {
    AVRational rate = {0, 0};

    if (av_parse_video_rate(&rate, "25/1") >= 0) {
        av_log(nullptr, AV_LOG_INFO, "Parse 25/1. Result rate: %d/%d\n", rate.num, rate.den);
    }

    if (av_parse_video_rate(&rate, "ntsc") >= 0) {
        av_log(nullptr, AV_LOG_INFO, "Parse ntsc. Result rate: %d/%d\n", rate.num, rate.den);
    }

    if (av_parse_video_rate(&rate, "pal") >= 0) {
        av_log(nullptr, AV_LOG_INFO, "Parse pal. Result rate: %d/%d\n", rate.num, rate.den);
    }
}

// 测试时间解析功能
void TestParseTime() {
    int64_t timeval = 0;

    // 时长
    if (av_parse_time(&timeval, "00:00:01", 1) >= 0) {
        av_log(nullptr, AV_LOG_INFO, "Parse 00:00:01. Result timeval(microsecond): %ld\n", timeval);
    }

    // 时间戳
    if (av_parse_time(&timeval, "00:00:01", 0) >= 0) {
        av_log(nullptr, AV_LOG_INFO, "Parse 00:00:01. Result timeval(microsecond): %ld\n", timeval);
    }
}

} // namespace TEST_AV_PARSE_UTIL