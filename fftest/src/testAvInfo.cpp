#include <string>
#include <memory>

extern "C" { 
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "testAvInfo.h"

namespace TEST_AV_INFO {

namespace TEST_AV_INFO_DATA {

typedef struct StreamInfo {
    using Ptr = std::shared_ptr<StreamInfo>;

    //              输入文件名称
    std::string     inFileName;

    //              输入文件时长（秒）
    double          duration { 0 };

    //              输入文件比特率（Mb/s）
    double          bitRate { 0 };

    /** ------------ 音频 --------------- */

    //              音频流索引
    int             audioIdx { -1 };

    //              采样率（Hz）
    int             sampleRate { 0 };

    //              通道数量
    int             channels { -1 };

    //              音频编码器名称
    std::string     audioCodecName;

    /** ------------ 视频 --------------- */

    //              视频流索引
    int             videoIdx { -1 };

    //              视频宽度
    int             width { -1 };

    //              视频高度
    int             height { -1 };

    //              视频帧率（fps）
    double          frameRate { 0 };

    //              视频编码器名称
    std::string     videoCodecName;

} StreamInfo;

} // namespace TEST_AV_INFO_DATA

// 测试输出文件的媒体流信息
void TestPrintStreamInfo() {
    auto streamInfo = std::make_shared<TEST_AV_INFO_DATA::StreamInfo>();
    streamInfo->inFileName = "../../resource/video3.mp4";

    AVFormatContext* inFmtCtx = nullptr;

    // 打开输入文件
    char errbuf[1024] = {0};
    int ret = avformat_open_input(&inFmtCtx, streamInfo->inFileName.c_str(), nullptr, nullptr);
    if (0 != ret) { 
        av_strerror(ret, errbuf, sizeof(errbuf));
        av_log(nullptr, AV_LOG_ERROR, "avformat_open_input() failed. ret: %d. errbuf: %s\n", ret, errbuf);
        return;
    }
    av_log(nullptr, AV_LOG_INFO, "Open input success. file name: %s\n", streamInfo->inFileName.c_str());

    do {
        // 获取输入文件媒体流信息
        ret = avformat_find_stream_info(inFmtCtx, nullptr);
        if (ret < 0) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(nullptr, AV_LOG_ERROR, "avformat_open_input() failed. ret: %d. errbuf: %s\n", ret, errbuf);
            break;
        }

        streamInfo->duration = static_cast<double>(inFmtCtx->duration) / static_cast<double>(AV_TIME_BASE);
        streamInfo->bitRate  = static_cast<double>(inFmtCtx->bit_rate) / static_cast<double>(AV_TIME_BASE);

        for (int idx = 0; idx < inFmtCtx->nb_streams; ++idx) {
            AVStream* stream = inFmtCtx->streams[idx];
            if (AVMediaType::AVMEDIA_TYPE_AUDIO == stream->codecpar->codec_type) {
                streamInfo->audioIdx       = idx;
                streamInfo->channels       = stream->codecpar->ch_layout.nb_channels;
                streamInfo->sampleRate     = stream->codecpar->sample_rate;
                streamInfo->audioCodecName = avcodec_get_name(stream->codecpar->codec_id);
            }
            else if (AVMediaType::AVMEDIA_TYPE_VIDEO == stream->codecpar->codec_type) {
                streamInfo->videoIdx       = idx;
                streamInfo->width          = stream->codecpar->width;
                streamInfo->height         = stream->codecpar->height;
                streamInfo->frameRate      = av_q2d(stream->avg_frame_rate);
                streamInfo->videoCodecName = avcodec_get_name(stream->codecpar->codec_id);
            }
        }

        // 媒体流信息打印
        av_log(nullptr, AV_LOG_INFO, "Input file duration: %.3f(s). bit rate: %.3f(Mb/s)\n", streamInfo->duration, streamInfo->bitRate);
        av_log(nullptr, AV_LOG_INFO, "-----------------------------------------------------------------------------\n");

        av_log(nullptr, AV_LOG_INFO, 
            "Audio info. idx: %d. codec: %s. sample rate: %d(Hz). channels: %d\n", 
            streamInfo->audioIdx, streamInfo->audioCodecName.c_str(), streamInfo->sampleRate, streamInfo->channels
        );
        av_log(nullptr, AV_LOG_INFO, "-----------------------------------------------------------------------------\n");

        av_log(nullptr, AV_LOG_INFO, 
            "Video info. idx: %d. codec: %s. width: %d(px). height: %d(px). frame rate: %.3f(fps)\n", 
            streamInfo->videoIdx, streamInfo->videoCodecName.c_str(), streamInfo->width, streamInfo->height, streamInfo->frameRate
        );
        av_log(nullptr, AV_LOG_INFO, "-----------------------------------------------------------------------------\n");

        if (-1 != streamInfo->audioIdx) {
            av_dump_format(inFmtCtx, streamInfo->audioIdx, streamInfo->inFileName.c_str(), 0);
            av_log(nullptr, AV_LOG_INFO, "-----------------------------------------------------------------------------\n");
        }
        
        if (-1 != streamInfo->videoIdx) {
            av_dump_format(inFmtCtx, streamInfo->videoIdx, streamInfo->inFileName.c_str(), 0);
            av_log(nullptr, AV_LOG_INFO, "-----------------------------------------------------------------------------\n");
        }

    } while(false);

    avformat_close_input(&inFmtCtx);
}

}; // namespace TEST_AV_INFO