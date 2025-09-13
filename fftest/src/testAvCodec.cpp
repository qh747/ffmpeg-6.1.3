#include <thread>

extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
}

#include "testAvCodec.h"

namespace TEST_AV_CODEC {

namespace TEST_AV_CODEC_FUNC {

// 功能函数 - 编码一帧数据
static int FuncEncode(AVCodecContext* inEncCtx, AVFrame* inFrame, AVPacket* outPkt, FILE* outFile) {
    char errbuf[1024] = {0};

    std::hash<std::thread::id> hasher;
    int tid = static_cast<int>(hasher(std::this_thread::get_id()));

    // 将原始帧数据发送到编码器
    if (nullptr != inFrame) {
        av_log(nullptr, AV_LOG_INFO, "THREAD: %d. Input frame into encoder. pts: %ld\n", tid, inFrame->pts);
    }   

    int ret = avcodec_send_frame(inEncCtx, inFrame);
    if (0 != ret) {
        av_strerror(ret, errbuf, sizeof(errbuf));
        av_log(nullptr, AV_LOG_ERROR, "THREAD: %d. avcodec_send_frame() failed. ret: %d, errbuf: %s\n", tid, ret, errbuf);
        return ret;
    }

    // 接收编码后的压缩数据
    while (ret >= 0) {
        ret = avcodec_receive_packet(inEncCtx, outPkt);

        if (0 != ret) {
            // 编码错误
            if (AVERROR(EAGAIN) == ret) {
                av_log(nullptr, AV_LOG_WARNING, "THREAD: %d. Not enough frames for output packet from encoder.\n", tid);
                break;
            }   
            else if (AVERROR_EOF == ret) {
                av_log(nullptr, AV_LOG_WARNING, "THREAD: %d. No more frames to receive from encoder.\n", tid);
                return ret;
            }
            else {
                av_strerror(ret, errbuf, sizeof(errbuf));
                av_log(nullptr, AV_LOG_ERROR, "THREAD: %d. avcodec_receive_packet() failed. ret: %d, errbuf: %s\n", tid, ret, errbuf);
                return ret;
            }
        }
        else {
            // 编码成功
            av_log(nullptr, AV_LOG_INFO, "THREAD: %d. Output packet from encoder. pts: %ld. dts: %ld. size: %d\n", 
                tid, outPkt->pts, outPkt->dts, outPkt->size);

            // 输出编码后的数据
            fwrite(outPkt->data, outPkt->size, 1, outFile);
            av_packet_unref(outPkt);
            break;
        }
    }
    return 0;
}

} // namespace TEST_AV_CODEC_FUNC

// 测试编码随机数
void TestEncodeRandom() {
    // 打开写入文件
    FILE* fp = fopen("output.mp4", "wb");
    if (nullptr == fp) {    
        av_log(nullptr, AV_LOG_ERROR, "fopen() failed. file: %s\n", "output.mp4");
        return;
    }

    // 创建编码器
    const AVCodec* enc = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (nullptr == enc) {
        av_log(nullptr, AV_LOG_ERROR, "avcodec_find_encoder() failed. encoder id: %d\n", static_cast<int>(AV_CODEC_ID_H264));
        return;
    }

    AVCodecContext* encCtx = avcodec_alloc_context3(enc);
    if (nullptr == encCtx) {
        av_log(nullptr, AV_LOG_ERROR, "avcodec_alloc_context3() failed.\n");
        return;
    }

    // 设置编码器参数
    encCtx->width        = 352;
    encCtx->height       = 288;
    encCtx->pix_fmt      = AV_PIX_FMT_YUV420P;
    encCtx->gop_size     = 10;
    encCtx->bit_rate     = 400000;
    encCtx->time_base    = (AVRational){1, 25};
    encCtx->framerate    = (AVRational){25, 1};
    encCtx->max_b_frames = 1;

    av_opt_set(encCtx->priv_data, "preset", "slow", 0);

    char errbuf[1024] = {0};
    AVFrame* frame = nullptr;
    AVPacket* pkt  = nullptr;

    do {
        // 创建编码前的frame数据
        frame = av_frame_alloc();
        if (nullptr == frame) {
            av_log(nullptr, AV_LOG_ERROR, "av_frame_alloc() failed.\n");
            break;
        }

        frame->format = encCtx->pix_fmt;
        frame->width  = encCtx->width;
        frame->height = encCtx->height;

        int ret = av_frame_get_buffer(frame, 0);
        if (0 != ret) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(nullptr, AV_LOG_ERROR, "av_frame_get_buffer() failed. ret: %d, errbuf: %s\n", ret, errbuf);
            break;
        }

        // 创建编码后的packet数据
        pkt = av_packet_alloc();
        if (nullptr == pkt) {
            av_log(nullptr, AV_LOG_ERROR, "av_packet_alloc() failed.\n");
            break;
        }

        // 打开编码器
        ret = avcodec_open2(encCtx, enc, nullptr);
        if (0 != ret) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(nullptr, AV_LOG_ERROR, "avcodec_open2() failed. ret: %d, errbuf: %s\n", ret, errbuf);
            break;
        }

        // 编码5秒视频
        av_log(nullptr, AV_LOG_INFO, "Encode start. --------------------\n");

        int time = static_cast<int>(av_q2d(encCtx->framerate)) * 5;
        for (int cnt = 0; cnt < time; ++cnt) {
            // 确保当前帧可写入数据
            ret = av_frame_make_writable(frame);
            if (0 != ret) {
                av_strerror(ret, errbuf, sizeof(errbuf));
                av_log(nullptr, AV_LOG_ERROR, "av_frame_make_writable() failed. ret: %d, errbuf: %s\n", ret, errbuf);
                break;
            }

            // 写入帧数据的YUV分量
            for (int h = 0; h < frame->height; ++h) {
                for (int w = 0; w < frame->width; ++w) {
                    // Y分量
                    frame->data[0][h * frame->linesize[0] + w] = (h + w + cnt) % 256;

                    // UV分量
                    if (h < (frame->height / 2) && w < (frame->width / 2)) {
                        frame->data[1][h * frame->linesize[1] + w] = (128 + h + cnt) % 256;
                        frame->data[2][h * frame->linesize[2] + w] = (64  + w + cnt) % 256;
                    }
                }
            }

            // 写入帧数据的编码时间戳
            frame->pts = cnt;

            // 编码一帧数据
            if (0 != TEST_AV_CODEC_FUNC::FuncEncode(encCtx, frame, pkt, fp)) {
                break;
            }
        }

        // 刷新解码器剩余缓存数据
        av_log(nullptr, AV_LOG_INFO, "Encode done. Flush encoder.--------------------\n");
        TEST_AV_CODEC_FUNC::FuncEncode(encCtx, nullptr, pkt, fp);

    } while(false);

    // 释放资源
    fclose(fp);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&encCtx);
}

}; // namespace TEST_AV_CODEC