#include <string>
#include <memory>
#include <sstream>
#include <functional>

extern "C" {
#include <libavutil/log.h>
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavcodec/packet.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

#include "testAvFilter.h"

namespace TEST_AV_FILTER {

namespace TEST_AV_FILTER_DATA {

typedef struct InputContext {
    using Ptr = std::shared_ptr<InputContext>;
    using WkPtr = std::weak_ptr<InputContext>;

    //                  输入文件名
    std::string         inFileName;

    //                  输入压缩数据
    AVPacket*           inPkt { nullptr };

    //                  输入上下文格式
    AVFormatContext*    inFmtCtx { nullptr };

    //                  输入视频流
    AVStream*           inVideoStream { nullptr };

    //                  输入视频解码器
    AVCodec*            inVideoCodec { nullptr };

    //                  输入视频解码器上下文
    AVCodecContext*     inVideoCodecCtx { nullptr };

    //                  输入视频流索引
    int                 inVideoStreamIdx { -1 };

    ~InputContext() {
        av_packet_free(&inPkt);
        avcodec_free_context(&inVideoCodecCtx);
        avformat_close_input(&inFmtCtx);
    }

} InputContext;

typedef struct OutputContext {
    using Ptr = std::shared_ptr<OutputContext>;
    using WkPtr = std::weak_ptr<OutputContext>;

    /** ------------------- 文件I/O ---------------------- */

    //                  输出文件名
    std::string         outFileName;

    //                  输出文件
    FILE*               outFile { nullptr };

    //                  压缩数据输出、过滤器输入数据
    AVFrame*            outInFrame { nullptr };

    //                  过滤器输出数据
    AVFrame*            outFrame { nullptr };

    /** ------------------- 过滤器 ---------------------- */

    //                  过滤信息描述
    std::string         filterDesc;

    //                  过滤器管理
    AVFilterGraph*      filterGraph { nullptr };

    /** ------------------- 输入过滤器 ---------------------- */

    //                  输入过滤器名称(不可自定义，会在ffmpeg支持的过滤器中查找)
    const std::string   inFilterName { "buffer" };

    //                  输入过滤器参数("video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d")
    std::string         inFilterArgs;

    //                  输入过滤器接口
    AVFilterInOut*      inFilterGate;

    //                  输出过滤器上下文
    AVFilterContext*    inFilterCtx { nullptr };

    /** ------------------- 输出过滤器 ---------------------- */

    //                  输出过滤器名称(不可自定义，会在ffmpeg支持的过滤器中查找)
    const std::string   outFilterName { "buffersink" };

    //                  输出过滤器接口
    AVFilterInOut*      outFilterGate;

    //                  输出过滤器上下文
    AVFilterContext*    outFilterCtx { nullptr };

    ~OutputContext() {
        fclose(outFile);
        av_frame_free(&outFrame);
        av_frame_free(&outInFrame);
        avfilter_inout_free(&inFilterGate);
        avfilter_inout_free(&outFilterGate);
        avfilter_graph_free(&filterGraph);
    }

} OutputContext;

} // namespace TEST_AV_FILTER_DATA

namespace TEST_AV_FILTER_FUNC { 

// 原始数据过滤处理回调函数
using FilterFrameCb = std::function<int(AVFrame*, AVFilterContext*, AVFilterContext*, AVFrame*, FILE*)>;

/** 
 * @brief  输入数据
 * @return -1 - 读取失败, 0 - 读取成功
 */
int FuncInputPacket(AVFormatContext* inFmtCtx, int inStreamIdx, AVPacket** outPktAddr) {
    char errbuf[1024] = {0};

    while (true) {
        // 从文件读取数据
        int ret = av_read_frame(inFmtCtx, *outPktAddr);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                av_log(inFmtCtx, AV_LOG_INFO, "Read end of input file.\n");
            } 
            else {
                av_strerror(ret, errbuf, sizeof(errbuf));
                av_log(inFmtCtx, AV_LOG_ERROR, "av_read_frame() failed. ret: %d. errbuf: %s\n", ret, errbuf);
            }

            return -1;
        }

        // 判断是否为感兴趣数据类型
        if ((*outPktAddr)->stream_index != inStreamIdx) {
            av_packet_unref(*outPktAddr);
            continue;
        }
        else {
            break;
        }
    }
    return 0;
}

/** 
 * @brief  输出数据
 * @return -1 - 失败, 0 - 成功
 */
int FuncOutputPacket(AVCodecContext* inDecCtx, AVPacket* inPkt, AVFrame* inFrame, AVFilterContext* inFilterCtx, 
    AVFilterContext* outFilterCtx, AVFrame* outFrame, FILE* outFile, FilterFrameCb filterCb) {
    char errbuf[1024] = {0};

    // 将数据发送到解码器
    int ret = avcodec_send_packet(inDecCtx, inPkt);
    if (ret < 0) {
        av_strerror(ret, errbuf, sizeof(errbuf));
        av_log(nullptr, AV_LOG_ERROR, "avcodec_send_packet() failed. ret: %d, errbuf: %s\n", ret, errbuf);
        return -1;
    }

    // 从解码器获取解码后的数据
    while (true) {
        ret = avcodec_receive_frame(inDecCtx, inFrame);
        if (ret < 0) {
            if (AVERROR(EAGAIN) == ret) {
                // 输入数据不够
                break;
            }
            else if (AVERROR_EOF != ret) {
                // 解码错误
                av_strerror(ret, errbuf, sizeof(errbuf));
                av_log(nullptr, AV_LOG_ERROR, "avcodec_receive_frame() failed. ret: %d, errbuf: %s\n", ret, errbuf);
            }
            return -1;
        }
        
        // 预估最佳显示时间戳
        inFrame->pts = inFrame->best_effort_timestamp;

        // 数据发送到过滤器处理
        ret = filterCb(inFrame, inFilterCtx, outFilterCtx, outFrame, outFile);
        av_frame_unref(inFrame);

        if (ret < 0) {
            return -1;
        }
    }

    return 0;
}

/** 
 * @brief  过滤数据
 * @return -1 - 失败, 0 - 成功
 */
int FuncFiltetFrame(AVFrame* inFrame, AVFilterContext* inFilterCtx, 
    AVFilterContext* outFilterCtx, AVFrame* outFrame, FILE* outFile) {
    char errbuf[1024] = {0};

    // 将原始数据发送到过滤器
    int ret = av_buffersrc_add_frame_flags(inFilterCtx, inFrame, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0) {
        av_strerror(ret, errbuf, sizeof(errbuf));
        av_log(nullptr, AV_LOG_ERROR, "av_buffersrc_add_frame_flags() failed. ret: %d, errbuf: %s\n", ret, errbuf);
        return -1;
    }

    // 从过滤器中获取过滤后的数据
    while (true) {
        ret = av_buffersink_get_frame(outFilterCtx, outFrame);
        if (ret < 0) {
            if (AVERROR(EAGAIN) == ret) {
                // 输入数据不够
                break;
            }
            else if (AVERROR_EOF != ret) {
                // 解码错误
                av_strerror(ret, errbuf, sizeof(errbuf));
                av_log(nullptr, AV_LOG_ERROR, "av_buffersink_get_frame() failed. ret: %d, errbuf: %s\n", ret, errbuf);
            }
            return -1;
        }

        // 写入数据到文件
        {
            av_log(nullptr, AV_LOG_INFO, 
                "Write frmae. width: %d x height: %d. format: %s. pts: %ld\n", 
                outFrame->width, 
                outFrame->height, 
                av_get_pix_fmt_name(static_cast<AVPixelFormat>(outFrame->format)), 
                outFrame->pts);

            fwrite(outFrame->data[0], 1, outFrame->width     * outFrame->height,     outFile);
            fwrite(outFrame->data[1], 1, (outFrame->width/2) * (outFrame->height/2), outFile);
            fwrite(outFrame->data[2], 1, (outFrame->width/2) * (outFrame->height/2), outFile);
        }
    
        av_frame_unref(outFrame);
    }

    return 0;
}

} // namespace TEST_AV_FILTER_FUNC

// 测试过滤器基本功能
void TestFilterBasic() {
    // 输入数据准备
    TEST_AV_FILTER_DATA::InputContext::Ptr inputCtx = std::make_shared<TEST_AV_FILTER_DATA::InputContext>();
    {
        char errbuf[1024] = {0};

        // 输入文件名
        inputCtx->inFileName = "../../resource/video3.mp4";

        // 输入上下文格式内存分配
        int ret = avformat_open_input(&(inputCtx->inFmtCtx), inputCtx->inFileName.c_str(), nullptr, nullptr);
        if (0 != ret) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(inputCtx->inFmtCtx, AV_LOG_ERROR, "avformat_open_input() failed. ret: %d. err info: %s\n", ret, errbuf);
            return;
        }

        // 获取输入媒体流信息
        ret = avformat_find_stream_info(inputCtx->inFmtCtx, nullptr);
        if (ret < 0) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(inputCtx->inFmtCtx, AV_LOG_ERROR, "avformat_find_stream_info() failed. ret: %d. err info: %s\n", ret, errbuf);
            return;
        }

        // 获取输入视频媒体流信息
        auto inVideoCodecAddrCst = const_cast<const AVCodec**>(&(inputCtx->inVideoCodec));
        inputCtx->inVideoStreamIdx = av_find_best_stream(inputCtx->inFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, inVideoCodecAddrCst, 0);
        if (inputCtx->inVideoStreamIdx < 0) {
            av_strerror(inputCtx->inVideoStreamIdx, errbuf, sizeof(errbuf));
            av_log(inputCtx->inFmtCtx, AV_LOG_ERROR, "av_find_best_stream() failed. ret: %d. err info: %s\n", 
                inputCtx->inVideoStreamIdx, errbuf);
            return;
        }
        inputCtx->inVideoStream = inputCtx->inFmtCtx->streams[inputCtx->inVideoStreamIdx];

        // 创建输入视频解码器上下文
        inputCtx->inVideoCodecCtx = avcodec_alloc_context3(nullptr);
        if (nullptr == inputCtx->inVideoCodecCtx) {
            av_log(inputCtx->inFmtCtx, AV_LOG_ERROR, "avcodec_alloc_context3() failed.\n");
            return;
        }

        ret = avcodec_parameters_to_context(inputCtx->inVideoCodecCtx, inputCtx->inVideoStream->codecpar);
        if (ret < 0) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(inputCtx->inFmtCtx, AV_LOG_ERROR, "avcodec_parameters_to_context() failed. ret: %d. err info: %s\n", ret, errbuf);
            return;
        }

        // 打开解码器
        ret = avcodec_open2(inputCtx->inVideoCodecCtx, inputCtx->inVideoCodec, nullptr);
        if (0 != ret) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(inputCtx->inFmtCtx, AV_LOG_ERROR, "avcodec_open2() failed. ret: %d. err info: %s\n", ret, errbuf);
            return;
        }

        // 输入压缩数据内存分配
        inputCtx->inPkt = av_packet_alloc();
        if (nullptr == inputCtx->inPkt) {
            av_log(inputCtx->inFmtCtx, AV_LOG_ERROR, "av_packet_alloc() failed.\n");
            return;
        }

        av_log(nullptr, AV_LOG_INFO, "Open input success. file name: %s. stream index: %d. codec: %s.\n", 
            inputCtx->inFileName.c_str(), inputCtx->inVideoStreamIdx, avcodec_get_name(inputCtx->inVideoStream->codecpar->codec_id));
    }

    // 输出数据准备
    TEST_AV_FILTER_DATA::OutputContext::Ptr outputCtx = std::make_shared<TEST_AV_FILTER_DATA::OutputContext>();
    {
        // 输出文件
        outputCtx->outFileName = "./output.yuv";
        outputCtx->outFile = fopen(outputCtx->outFileName.c_str(), "wb");
        if (nullptr == outputCtx->outFile) {
            av_log(nullptr, AV_LOG_ERROR, "fopen() failed\n");
            return;
        }

        // 压缩数据输出、过滤器输入数据内存分配
        outputCtx->outInFrame = av_frame_alloc();
        if (nullptr == outputCtx->outInFrame) {
            av_log(nullptr, AV_LOG_ERROR, "outin: av_frame_alloc() failed.\n");
            return;
        }

        // 过滤器输出数据内存分配
        outputCtx->outFrame = av_frame_alloc();
        if (nullptr == outputCtx->outFrame) {
            av_log(nullptr, AV_LOG_ERROR, "out: av_frame_alloc() failed.\n");
            return;
        }

        av_log(nullptr, AV_LOG_INFO, "Open output success. file name: %s.\n", outputCtx->outFileName.c_str());
    }

    // 过滤器数据准备
    {
        char errbuf[1024] = {0};

        // 创建过滤器管理对象
        outputCtx->filterGraph = avfilter_graph_alloc();
        if (nullptr == outputCtx->filterGraph) {
            av_log(nullptr, AV_LOG_ERROR, "avfilter_graph_alloc() failed.\n");
            return;
        }
        
        // 查找过滤器
        const AVFilter* inFilter = avfilter_get_by_name(outputCtx->inFilterName.c_str());
        if (nullptr == inFilter) {
            av_log(nullptr, AV_LOG_ERROR, "in: avfilter_get_by_name() failed. input filter name: %s\n", outputCtx->inFilterName.c_str());
            return;
        }

        const AVFilter* outFilter = avfilter_get_by_name(outputCtx->outFilterName.c_str());
        if (nullptr == inFilter) {
            av_log(nullptr, AV_LOG_ERROR, "out: avfilter_get_by_name() failed. output filter name: %s\n", outputCtx->outFilterName.c_str());
            return;
        }

        // 创建过滤器上下文
        std::stringstream ss;
        ss << "video_size="   << inputCtx->inVideoCodecCtx->width                   << "x" << inputCtx->inVideoCodecCtx->height << ":" 
           << "pix_fmt="      << inputCtx->inVideoCodecCtx->pix_fmt                 << ":"
           << "time_base="    << inputCtx->inVideoStream->time_base.num             << "/" << inputCtx->inVideoStream->time_base.den << ":"
           << "pixel_aspect=" << inputCtx->inVideoCodecCtx->sample_aspect_ratio.num << "/" << inputCtx->inVideoCodecCtx->sample_aspect_ratio.den;

        outputCtx->inFilterArgs = ss.str();

        int ret = avfilter_graph_create_filter(
            &(outputCtx->inFilterCtx), inFilter, "in", outputCtx->inFilterArgs.c_str(), nullptr, outputCtx->filterGraph
        );

        if (ret < 0) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(nullptr, AV_LOG_ERROR, "in: avfilter_graph_create_filter() failed. ret: %d. err info: %s\n", ret, errbuf);
            return;
        }

        ret = avfilter_graph_create_filter(
            &(outputCtx->outFilterCtx), outFilter, "out", nullptr, nullptr, outputCtx->filterGraph
        );

        if (ret < 0) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(nullptr, AV_LOG_ERROR, "out: avfilter_graph_create_filter() failed. ret: %d. err info: %s\n", ret, errbuf);
            return;
        }

        // 创建过滤器接口
        outputCtx->inFilterGate = avfilter_inout_alloc();
        if (nullptr == outputCtx->inFilterGate) {
            av_log(nullptr, AV_LOG_ERROR, "in: avfilter_inout_alloc() failed.\n");
            return;
        }

        outputCtx->outFilterGate = avfilter_inout_alloc();
        if (nullptr == outputCtx->outFilterGate) {
            av_log(nullptr, AV_LOG_ERROR, "out: avfilter_inout_alloc().\n");
            return;
        }

        // 连接过滤器
        outputCtx->inFilterGate->name       = av_strdup("in");
        outputCtx->inFilterGate->filter_ctx = outputCtx->inFilterCtx;
        outputCtx->inFilterGate->pad_idx    = 0;
        outputCtx->inFilterGate->next       = nullptr;

        outputCtx->outFilterGate->name       = av_strdup("out");
        outputCtx->outFilterGate->filter_ctx = outputCtx->outFilterCtx;
        outputCtx->outFilterGate->pad_idx    = 0;
        outputCtx->outFilterGate->next       = nullptr;

        // 添加过滤器描述
        outputCtx->filterDesc = "scale=640:480,hflip";
        ret = avfilter_graph_parse_ptr(
            outputCtx->filterGraph, outputCtx->filterDesc.c_str(), &(outputCtx->outFilterGate), &(outputCtx->inFilterGate), nullptr
        );

        if (ret < 0) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(nullptr, AV_LOG_ERROR, "avfilter_graph_parse_ptr() failed. ret: %d. err info: %s\n", ret, errbuf);
            return;
        }

        // 配置过滤器生效
        ret = avfilter_graph_config(outputCtx->filterGraph, nullptr);
        if (ret < 0) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(nullptr, AV_LOG_ERROR, "avfilter_graph_config() failed. ret: %d. err info: %s\n", ret, errbuf);
            return;
        }

        av_log(nullptr, AV_LOG_INFO, "Open filter success. desc: %s. args: %s. in: %s. out: %s\n", 
            outputCtx->filterDesc.c_str(), outputCtx->inFilterArgs.c_str(), outputCtx->inFilterName.c_str(), outputCtx->outFilterName.c_str()
        );
    }

    // 数据处理
    {
        av_log(nullptr, AV_LOG_INFO, "Start filter process.----------------------------\n");
        while (true) {
            // 输入压缩数据
            int ret = TEST_AV_FILTER_FUNC::FuncInputPacket(inputCtx->inFmtCtx, inputCtx->inVideoStreamIdx, &(inputCtx->inPkt));
            if (ret < 0) {
                break;
            }

            // 输出解码数据
            ret = TEST_AV_FILTER_FUNC::FuncOutputPacket(inputCtx->inVideoCodecCtx, inputCtx->inPkt,
                outputCtx->outInFrame, outputCtx->inFilterCtx, outputCtx->outFilterCtx, outputCtx->outFrame, outputCtx->outFile, 
                TEST_AV_FILTER_FUNC::FuncFiltetFrame
            );

            if (ret < 0) {
                break;
            }

            av_packet_unref(inputCtx->inPkt);
        }
        av_log(nullptr, AV_LOG_INFO, "End filter process.----------------------------\n");

        // 刷新解码器缓存数据
        TEST_AV_FILTER_FUNC::FuncOutputPacket(inputCtx->inVideoCodecCtx, nullptr,
            outputCtx->outInFrame, outputCtx->inFilterCtx, outputCtx->outFilterCtx, outputCtx->outFrame, outputCtx->outFile, 
            TEST_AV_FILTER_FUNC::FuncFiltetFrame
        );
    }

    // 播放方式提示
    {
        av_log(nullptr, AV_LOG_INFO, "Play the output video file with the command:\n\
            ffplay -f rawvideo -i %s -pixel_format %s -video_size 640x480\n",
            outputCtx->outFileName.c_str(), av_get_pix_fmt_name(inputCtx->inVideoCodecCtx->pix_fmt));
    }
}

} // namespace TEST_AV_FILTER