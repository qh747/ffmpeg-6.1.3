#include <string>

extern "C" {
#include <libavutil/log.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavformat/avformat.h>
}

#include "testAvFormat.h"

namespace TEST_AV_FORMAT {

// 测试输出文件的媒体流信息
void TestShowStreamInfo() {
    // 指定打开的文件名称
    std::string filename = "../../resource/video3.mp4";

    // 打开文件
    AVFormatContext* fmtCtx = nullptr;
    AVInputFormat*   inFmt  = nullptr;
    AVDictionary*    dict   = nullptr;

    char errBuf[1024] = "0";
    do {
        int ret = avformat_open_input(&fmtCtx, filename.c_str(), inFmt, &dict);
        if (0 != ret) {
            av_strerror(ret, errBuf, sizeof(errBuf));
            av_log(fmtCtx, AV_LOG_ERROR, "avformat_open_input() failed. err info: %s\n", errBuf);
            break;
        }

        // 获取媒体流信息
        ret = avformat_find_stream_info(fmtCtx, nullptr);
        if (ret < 0) {
            av_strerror(ret, errBuf, sizeof(errBuf));
            av_log(fmtCtx, AV_LOG_ERROR, "avformat_find_stream_info() failed. err info: %s\n", errBuf);
            break;
        }

        // 输出基本媒体信息
        av_log(fmtCtx, AV_LOG_INFO, "Open file success.\n");
        av_log(fmtCtx, AV_LOG_INFO, "  [URL]:          %s\n",         fmtCtx->url);
        av_log(fmtCtx, AV_LOG_INFO, "  [BIT RATE]:     %.3f (b/s)\n", fmtCtx->bit_rate / 1000.0);
        av_log(fmtCtx, AV_LOG_INFO, "  [STREAM COUNT]: %d\n",         fmtCtx->nb_streams);
        av_log(fmtCtx, AV_LOG_INFO, "  [DURATION]:     %02ld:%02ld:%02ld.%03ld\n", 
            (fmtCtx->duration / AV_TIME_BASE) / 3600, 
            (fmtCtx->duration / AV_TIME_BASE) % 3600 / 60, 
            (fmtCtx->duration / AV_TIME_BASE) % 60, 
            (fmtCtx->duration / 1000) % 1000);

        av_log(fmtCtx, AV_LOG_INFO, "--------------------------\n");

        // 输出媒体流信息
        for (int idx = 0; idx < fmtCtx->nb_streams; ++idx) {
            AVStream* stream = fmtCtx->streams[idx];
            if (AVMEDIA_TYPE_AUDIO == stream->codecpar->codec_type) {
                av_log(fmtCtx, AV_LOG_INFO, "Audio info.\n");
                av_log(fmtCtx, AV_LOG_INFO, "  [INDEX]:       %d\n",      stream->index);
                av_log(fmtCtx, AV_LOG_INFO, "  [CODEC]:       %s\n",      avcodec_get_name(stream->codecpar->codec_id));
                av_log(fmtCtx, AV_LOG_INFO, "  [CHANNEL]:     %d\n",      stream->codecpar->ch_layout.nb_channels);
                av_log(fmtCtx, AV_LOG_INFO, "  [SAMPLE FMT]:  %s\n",      av_get_sample_fmt_name(static_cast<AVSampleFormat>(stream->codecpar->format)));
                av_log(fmtCtx, AV_LOG_INFO, "  [SAMPLE RATE]: %d (Hz)\n", stream->codecpar->sample_rate);

                int duration = stream->duration * (av_q2d(stream->time_base) * 1000);
                int hour = (duration / 1000) / 3600;
                int min  = (duration / 1000) % 3600 / 60;
                int sec  = (duration / 1000) % 60;
                int msec = duration % 1000;
                av_log(fmtCtx, AV_LOG_INFO, "  <DURATION>:    %02d:%02d:%02d.%03d\n", hour, min, sec, msec);
            }
            else if (AVMEDIA_TYPE_VIDEO == stream->codecpar->codec_type) {
                av_log(fmtCtx, AV_LOG_INFO, "Video info.\n");
                av_log(fmtCtx, AV_LOG_INFO, "  [INDEX]:        %d\n",         stream->index);
                av_log(fmtCtx, AV_LOG_INFO, "  [CODEC]:        %s\n",         avcodec_get_name(stream->codecpar->codec_id));
                av_log(fmtCtx, AV_LOG_INFO, "  [FRAME RATE]:   %.3f (fps)\n", av_q2d(stream->avg_frame_rate));
                av_log(fmtCtx, AV_LOG_INFO, "  [WIDTH:HEIGHT]: %d:%d\n",      stream->codecpar->width, stream->codecpar->height);
                av_log(fmtCtx, AV_LOG_INFO, "  [PIXEL FMT]:    %s\n",         av_get_pix_fmt_name(static_cast<AVPixelFormat>(stream->codecpar->format)));

                int duration = stream->duration * (av_q2d(stream->time_base) * 1000);
                int hour = (duration / 1000) / 3600;
                int min  = (duration / 1000) % 3600 / 60;
                int sec  = (duration / 1000) % 60;
                int msec = duration % 1000;
                av_log(fmtCtx, AV_LOG_INFO, "  <DURATION>:     %02d:%02d:%02d.%03d\n", hour, min, sec, msec);
            }
        }
    } while(false);

    // 资源释放
    avformat_close_input(&fmtCtx);
}

} // namespace TEST_AV_FORMAT