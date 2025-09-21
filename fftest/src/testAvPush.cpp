#include <string>
#include <memory>

extern "C" {
#include <libavutil/log.h>
#include <libavutil/time.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "testAvPush.h"

namespace TEST_AV_PUSH {

namespace TEST_AV_PUSH_DATA {

typedef struct InputContext {
    using Ptr = std::shared_ptr<InputContext>;

    //                  输入文件名称
    std::string         inFileName;

    //                  输入文件格式上下文
    AVFormatContext*    inFmtCtx { nullptr };

    //                  输入视频流索引
    int                 inVideoIdx { -1 };

    ~InputContext() {
        avformat_close_input(&inFmtCtx);
    }

} InputContext;

typedef struct OutputContext {
    using Ptr = std::shared_ptr<OutputContext>;

    //                  输出rtsp流url
    std::string         outUrl;

    //                  输出文件格式上下文
    AVFormatContext*    outFmtCtx { nullptr };

    ~OutputContext() {
        if (nullptr != outFmtCtx) {
            avio_close(outFmtCtx->pb);
        }

        avformat_free_context(outFmtCtx);
    }

} OutputContext;

} // namespace TEST_AV_PUSH_DATA

// 测试RTSP推流
void TestRtspPush() {
    // 网络环境初始化
    avformat_network_init();

    // 输入数据准备
    auto inputCtx = std::make_shared<TEST_AV_PUSH_DATA::InputContext>();
    {
        char errbuf[1024] = { 0 };

        // 输入文件名称
        inputCtx->inFileName = "../../resource/video2.mp4";

        // 输入上下文格式
        int ret = avformat_open_input(&(inputCtx->inFmtCtx), inputCtx->inFileName.c_str(), nullptr, nullptr);
        if (0 != ret) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(nullptr, AV_LOG_ERROR, "avformat_open_input() failed. ret: %d. errbuf: %s\n", ret, errbuf);
            return;
        }

        // 获取输入媒体流信息
        ret = avformat_find_stream_info(inputCtx->inFmtCtx, nullptr);
        if (ret < 0) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(nullptr, AV_LOG_ERROR, "avformat_find_stream_info() failed. ret: %d. errbuf: %s\n", ret, errbuf);
            return;
        }

        // 获取输入视频流索引
        inputCtx->inVideoIdx = av_find_best_stream(inputCtx->inFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (inputCtx->inVideoIdx < 0) {
            av_strerror(inputCtx->inVideoIdx, errbuf, sizeof(errbuf));
            av_log(nullptr, AV_LOG_ERROR, "av_find_best_stream() failed. ret: %d. errbuf: %s\n", inputCtx->inVideoIdx, errbuf);
            return;
        }

        av_log(nullptr, AV_LOG_INFO, "Open input success. file name: %s\n", inputCtx->inFileName.c_str());
    }

    // 输出数据准备
    auto outputCtx = std::make_shared<TEST_AV_PUSH_DATA::OutputContext>();
    {
        char errbuf[1024] = {0};

        // 输出rtsp流url
        outputCtx->outUrl = "rtsp://192.168.1.236:554/live/stream";

        // 输出上下文格式
        int ret = avformat_alloc_output_context2(&(outputCtx->outFmtCtx), nullptr, "rtsp", outputCtx->outUrl.c_str());
        if (ret < 0) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(nullptr, AV_LOG_ERROR, "avformat_alloc_output_context2() failed. ret: %d. errbuf: %s\n", ret, errbuf);
            return;
        }

        // 创建输出流
        for (int cnt = 0; cnt < inputCtx->inFmtCtx->nb_streams; ++cnt) {
            AVStream* inStream  = inputCtx->inFmtCtx->streams[cnt];
            const AVCodec* inCodec = avcodec_find_decoder(inStream->codecpar->codec_id);

            AVStream* outStream = avformat_new_stream(outputCtx->outFmtCtx, inCodec);
            if (nullptr == outStream) {
                av_log(nullptr, AV_LOG_ERROR, "avformat_new_stream() failed. type: %s\n", av_get_media_type_string(inCodec->type));
                return;
            }

            ret = avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
            if (ret < 0) {
                av_strerror(ret, errbuf, sizeof(errbuf));
                av_log(nullptr, AV_LOG_ERROR, "avcodec_parameters_copy() failed. ret: %d. errbuf: %s\n", ret, errbuf);
                return;
            }
        }

        /**
         * 1.打开输出url(rtsp协议无需调用avio_open2()函数就可以实现rtsp推流
         * 2.执行avio_open2()函数会报错Protocol Not Found
         * 3.原因为url_protocols未包含rtsp协议
         */
        // ret = avio_open2(&(outputCtx->outFmtCtx->pb), outputCtx->outUrl.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
        // if (ret < 0) {
        //     av_strerror(ret, errbuf, sizeof(errbuf));
        //     av_log(nullptr, AV_LOG_ERROR, "avio_open() failed. url: %s, err: %s\n", outputCtx->outUrl.c_str(), errbuf);
        //     return;
        // }

        av_log(nullptr, AV_LOG_INFO, "Open output success. url: %s\n", outputCtx->outUrl.c_str());
    }

    // 输出数据
    {
        char errbuf[1024] = {0};

        // 输出文件首部
        int ret = avformat_write_header(outputCtx->outFmtCtx, nullptr);
        if (ret < 0) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(nullptr, AV_LOG_ERROR, "avformat_write_header() failed. ret: %d. errbuf: %s\n", ret, errbuf);
            return;
        }

        // 输出文件内容
        int       frameIdx  = 0;
        int64_t   startTime = av_gettime();
        AVStream* refStream = inputCtx->inFmtCtx->streams[inputCtx->inVideoIdx];

        av_log(nullptr, AV_LOG_INFO, 
            "Ref info. frame rate: %.3f. time base: {%d, %d}\n", 
            av_q2d(refStream->r_frame_rate), refStream->time_base.num, refStream->time_base.den
        );

        AVPacket inPkt;
        while (true) { 
            // 读取一帧数据
            ret = av_read_frame(inputCtx->inFmtCtx, &inPkt);
            if (ret < 0) {
                if (ret == AVERROR_EOF) {
                    av_log(nullptr, AV_LOG_INFO, "Read end of input file.\n");
                    break;
                } 
                else {
                    av_strerror(ret, errbuf, sizeof(errbuf));
                    av_log(nullptr, AV_LOG_ERROR, "av_read_frame() failed. ret: %d. errbuf: %s\n", ret, errbuf);
                    return;
                }
            }
            
            AVStream* inStream  = inputCtx->inFmtCtx->streams[inPkt.stream_index];
            AVStream* outStream = outputCtx->outFmtCtx->streams[inPkt.stream_index];

            // 处理无效的时间戳
            if (AV_NOPTS_VALUE == inPkt.pts) {
                /**
                 * 计算每帧预估时长: 1秒 / 帧率(帧/秒)
                 *   1.AV_TIME_BASE:            1秒时长, 单位: 微秒, 值: 1 000 000
                 *   2.refStream->r_frame_rate: 帧率(帧/秒), 值: 30 / 1
                 */
                int64_t durationPerFrame = static_cast<double>(static_cast<double>(AV_TIME_BASE) / av_q2d(refStream->r_frame_rate));
                av_log(nullptr, AV_LOG_INFO, 
                    "Invalid pts. time base: %d. frame rate: %.3f. duration per frame: %ld\n", 
                    AV_TIME_BASE, av_q2d(refStream->r_frame_rate), durationPerFrame
                );

                /**
                 * 计算每帧实际时长: 每帧预估时长 / 时间基
                 */
                inPkt.duration = static_cast<int64_t>(
                    static_cast<double>(durationPerFrame) / static_cast<double>(av_q2d(refStream->time_base) * AV_TIME_BASE)
                );
                
                /**
                 * 计算每帧的显示时间戳: (帧索引 * 每帧的时长) / 时间基
                 */
                inPkt.pts = static_cast<int64_t>(
                    static_cast<double>(frameIdx * durationPerFrame) / static_cast<double>(av_q2d(refStream->time_base) * AV_TIME_BASE)
                );
                inPkt.dts = inPkt.pts;
            }

            // 更新并输出视频/音频帧索引
            if (inputCtx->inVideoIdx == inPkt.stream_index) {
                // 将pts时间戳转换为标准时间格式
                int64_t ptsTime = av_rescale_q(inPkt.pts, refStream->time_base, AVRational{1, AV_TIME_BASE});

                // 获取当前时间
                int64_t nowTime = av_gettime() - startTime;

                // 如果pts时间戳大于当前时间，则未到显示时间，线程休眠等待
                if (ptsTime > nowTime) {
                    av_usleep(ptsTime - nowTime);
                }

                av_log(nullptr, AV_LOG_INFO, "Send video packet to output. idx: %8d\n", frameIdx++);
            }
            else {
                av_log(nullptr, AV_LOG_INFO, "Send audio packet to output. idx: %8d\n", frameIdx);
            }

            inPkt.pts = av_rescale_q_rnd(
                inPkt.pts, 
                inStream->time_base, 
                outStream->time_base, 
                static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX)
            );

            inPkt.dts = av_rescale_q_rnd(
                inPkt.dts, 
                inStream->time_base, 
                outStream->time_base, 
                static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX)
            );
            
            inPkt.duration = av_rescale_q(
                inPkt.duration, 
                inStream->time_base, 
                outStream->time_base
            );

            inPkt.pos = -1;

            // 输出数据
            ret = av_interleaved_write_frame(outputCtx->outFmtCtx, &inPkt);
            if (0 != ret) {
                av_strerror(ret, errbuf, sizeof(errbuf));
                av_log(nullptr, AV_LOG_ERROR, "av_interleaved_write_frame() failed. ret: %d. errbuf: %s\n", ret, errbuf);
                return;
            }

            av_packet_unref(&inPkt);
        }

        // 输出文件尾部
        ret = av_write_trailer(outputCtx->outFmtCtx);
        if (0 != ret) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(nullptr, AV_LOG_ERROR, "av_write_trailer() failed. ret: %d. errbuf: %s\n", ret, errbuf);
        }
    }

    // 网络环境释放
    avformat_network_deinit();
}

} // namespace TEST_AV_PUSH