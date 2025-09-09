#include <cstdio>
#include <cstdint>

extern "C" {
#include <libavformat/avformat.h>
}

#include "testAvIO.h"

namespace TEST_AV_IO {

// 测试打开文件mp4文件
void TestOpenMp4File() {
    // 初始化网络协议
    avformat_network_init();

    AVFormatContext* fmtCtx   = nullptr;
    AVInputFormat*   inFmtCtx = nullptr;
    do {
        // 创建avformat上下文
        fmtCtx = avformat_alloc_context();
        if (nullptr == fmtCtx) {
            av_log(fmtCtx, AV_LOG_ERROR, "avformat_alloc_context() failed\n");
            break;
        }

        // 打开mp4文件
        if (0 != avformat_open_input(&fmtCtx, "../../resource/video1.mp4", inFmtCtx, nullptr)) {
            av_log(fmtCtx, AV_LOG_ERROR, "avformat_open_input() failed\n");
            break;
        }

        // 获取媒体流信息
        if (0 > avformat_find_stream_info(fmtCtx, nullptr)) {
            av_log(fmtCtx, AV_LOG_ERROR, "avformat_find_stream_info() failed\n");
            break;
        }

        // 输出媒体流信息
        av_log(fmtCtx, AV_LOG_INFO, "Open stream success. stream count: %d\n", fmtCtx->nb_streams);

    } while(false);

    // 资源清理
    avformat_close_input(&fmtCtx);
    avformat_free_context(fmtCtx);
    avformat_network_deinit();
}

// 测试打开rtsp媒体流
void TestOpenRtspStream() {
    // 初始化网络协议
    avformat_network_init();

    AVFormatContext* fmtCtx   = nullptr;
    AVInputFormat*   inFmtCtx = nullptr;
    do {
        // 创建avformat上下文
        fmtCtx = avformat_alloc_context();
        if (nullptr == fmtCtx) {
            av_log(fmtCtx, AV_LOG_ERROR, "avformat_alloc_context() failed\n");
            break;
        }

        // 打开mp4文件
        if (0 != avformat_open_input(&fmtCtx, "rtsp://192.168.1.236:554/live/stream", inFmtCtx, nullptr)) {
            av_log(fmtCtx, AV_LOG_ERROR, "avformat_open_input() failed\n");
            break;
        }

        // 获取媒体流信息
        if (0 > avformat_find_stream_info(fmtCtx, nullptr)) {
            av_log(fmtCtx, AV_LOG_ERROR, "avformat_find_stream_info() failed\n");
            break;
        }

        // 输出媒体流信息
        av_log(fmtCtx, AV_LOG_INFO, "Open stream success. stream count: %d\n", fmtCtx->nb_streams);

    } while(false);

    // 资源清理
    avformat_close_input(&fmtCtx);
    avformat_free_context(fmtCtx);
    avformat_network_deinit();
}

// 测试打开自定义I/O
void TestOpenSelfDefineIO() {
    // 打开本地文件，作为自定义IO输入
    auto file = fopen("../../resource/video1.mp4", "rb");
    if (nullptr == file) {
        av_log(nullptr, AV_LOG_ERROR, "fopen() failed\n");
        return;
    }

    // 准备缓冲区，用于存放文件输入数据
    std::size_t size = 4096;
    uint8_t* buf = static_cast<uint8_t*>(av_malloc(size));

    AVIOContext*     ioCtx    = nullptr;
    AVFormatContext* fmtCtx   = nullptr;
    AVInputFormat*   inFmtCtx = nullptr;
    
    do {
        // 创建AVIOContext
        ioCtx = avio_alloc_context(buf, size, 0, file,
            // read packet
            [](void* opaque, uint8_t* buf, int bufSize) -> int { 
                auto file = static_cast<FILE*>(opaque);
                std::size_t size = fread(buf, 1, bufSize, file);

                av_log(nullptr, AV_LOG_INFO, "Read size: %ld. Buffer size: %d\n", size, bufSize);
                return size;
            },
            // write packet cb
            nullptr, 
            // seek packet cb
            nullptr);

        if (nullptr == ioCtx) {
            av_log(nullptr, AV_LOG_ERROR, "avio_alloc_context() failed\n");
            break;
        }
        
        // 创建avformat上下文
        fmtCtx = avformat_alloc_context();
        if (nullptr == fmtCtx) {
            av_log(fmtCtx, AV_LOG_ERROR, "avformat_alloc_context() failed\n");
            break;
        }

        // 关联自定义输入IO
        fmtCtx->pb = ioCtx;
        fmtCtx->flags = AVFMT_FLAG_CUSTOM_IO;

        // 创建inputformat上下文
        if (avformat_open_input(&fmtCtx, "", inFmtCtx, nullptr) < 0) {
            av_log(fmtCtx, AV_LOG_ERROR, "avformat_open_input() failed\n");
            break;
        }

        // 获取媒体流信息
        if (0 > avformat_find_stream_info(fmtCtx, nullptr)) {
            av_log(fmtCtx, AV_LOG_ERROR, "avformat_find_stream_info() failed\n");
            break;
        }

        // 输出媒体流信息
        av_log(fmtCtx, AV_LOG_INFO, "Open stream success. stream count: %d\n", fmtCtx->nb_streams);
        av_dump_format(fmtCtx, 0, "video1.mp4", 0);

    } while(false);

    // 资源清理
    avio_context_free(&ioCtx);
    avformat_close_input(&fmtCtx);
    avformat_free_context(fmtCtx);
    
    fclose(file);
}

} // namespace TEST_AV_IO