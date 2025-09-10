#include <string>

extern "C" {
#include <libavutil/log.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavformat/avformat.h>
}

#include "testAvFormat.h"

namespace TEST_AV_FORMAT {

namespace TEST_AV_FORMAT_FUNC {
// 输出媒体流基本信息
void FuncPrintBasicStreamInfo(AVFormatContext* fmtCtx) {
    av_log(fmtCtx, AV_LOG_INFO, "Basic stream info.\n");
    av_log(fmtCtx, AV_LOG_INFO, "  [URL]:          %s\n",         fmtCtx->url);
    av_log(fmtCtx, AV_LOG_INFO, "  [BIT RATE]:     %.3f (kb/s)\n", fmtCtx->bit_rate / 1000.0);
    av_log(fmtCtx, AV_LOG_INFO, "  [STREAM COUNT]: %d\n",         fmtCtx->nb_streams);
    av_log(fmtCtx, AV_LOG_INFO, "  [DURATION]:     %02ld:%02ld:%02ld.%03ld\n", 
        (fmtCtx->duration / AV_TIME_BASE) / 3600, 
        (fmtCtx->duration / AV_TIME_BASE) % 3600 / 60, 
        (fmtCtx->duration / AV_TIME_BASE) % 60, 
        (fmtCtx->duration / 1000) % 1000);

    av_log(fmtCtx, AV_LOG_INFO, "--------------------------\n");
}

// 输出音频媒体流信息
void FuncPrintAudioStreamInfo(AVFormatContext* fmtCtx, AVStream* stream) {
    av_log(fmtCtx, AV_LOG_INFO, "Audio stream info.\n");
    av_log(fmtCtx, AV_LOG_INFO, "  [INDEX]:       %d\n",      stream->index);
    av_log(fmtCtx, AV_LOG_INFO, "  [CODEC]:       %s\n",      avcodec_get_name(stream->codecpar->codec_id));
    av_log(fmtCtx, AV_LOG_INFO, "  [CHANNEL]:     %d\n",      stream->codecpar->ch_layout.nb_channels);
    av_log(fmtCtx, AV_LOG_INFO, "  [SAMPLE FMT]:  %s\n",      av_get_sample_fmt_name(static_cast<AVSampleFormat>(stream->codecpar->format)));
    av_log(fmtCtx, AV_LOG_INFO, "  [SAMPLE RATE]: %d (Hz)\n", stream->codecpar->sample_rate);

    int duration = stream->duration * (av_q2d(stream->time_base) * 1000);
    int hour = (duration / 1000) / 3600;
    int min  = (duration / 1000) % 3600 / 60;
    int sec  = (duration / 1000) % 60;
    int msec = (duration % 1000);
    av_log(fmtCtx, AV_LOG_INFO, "  [DURATION]:    %02d:%02d:%02d.%03d\n", hour, min, sec, msec);

    av_log(fmtCtx, AV_LOG_INFO, "--------------------------\n");
}

// 输出视频媒体流信息
void FuncPrintVideoStreamInfo(AVFormatContext* fmtCtx, AVStream* stream) {
    av_log(fmtCtx, AV_LOG_INFO, "Video stream info.\n");
    av_log(fmtCtx, AV_LOG_INFO, "  [INDEX]:        %d\n",         stream->index);
    av_log(fmtCtx, AV_LOG_INFO, "  [CODEC]:        %s\n",         avcodec_get_name(stream->codecpar->codec_id));
    av_log(fmtCtx, AV_LOG_INFO, "  [FRAME RATE]:   %.3f (fps)\n", av_q2d(stream->avg_frame_rate));
    av_log(fmtCtx, AV_LOG_INFO, "  [WIDTH:HEIGHT]: %d:%d\n",      stream->codecpar->width, stream->codecpar->height);
    av_log(fmtCtx, AV_LOG_INFO, "  [PIXEL FMT]:    %s\n",         av_get_pix_fmt_name(static_cast<AVPixelFormat>(stream->codecpar->format)));

    int duration = stream->duration * (av_q2d(stream->time_base) * 1000);
    int hour = (duration / 1000) / 3600;
    int min  = (duration / 1000) % 3600 / 60;
    int sec  = (duration / 1000) % 60;
    int msec = (duration % 1000);
    av_log(fmtCtx, AV_LOG_INFO, "  [DURATION]:     %02d:%02d:%02d.%03d\n", hour, min, sec, msec);

    av_log(fmtCtx, AV_LOG_INFO, "--------------------------\n");
}  

// 输出音频帧信息
void FuncPrintAudioFrame(AVFormatContext* fmtCtx, AVStream* stream, AVPacket* pkt) {
    av_log(fmtCtx, AV_LOG_INFO, "Audio frame info.\n");
    av_log(fmtCtx, AV_LOG_INFO, "  [INDEX]:       %d\n",          pkt->stream_index);
    av_log(fmtCtx, AV_LOG_INFO, "  [SIZE]:        %d\n",          pkt->size);
    av_log(fmtCtx, AV_LOG_INFO, "  [POS]:         %ld (bytes)\n", pkt->pos);
    av_log(fmtCtx, AV_LOG_INFO, "  [DURATION]:    %.3f (ms)\n",   pkt->duration * (av_q2d(stream->time_base) * 1000));
    av_log(fmtCtx, AV_LOG_INFO, "  [TIME BASE]:   %d/%d\n",       stream->time_base.num, stream->time_base.den);
    
    int pts   = (pkt->pts < 0 ? 0 : pkt->pts) * (av_q2d(stream->time_base) * 1000);
    int ptsH  = pts / 1000 / 3600;
    int ptsM  = (pts / 1000 / 60) % 60;
    int ptsS  = (pts / 1000) % 60;
    int ptsMS = (pts % 1000);
    av_log(fmtCtx, AV_LOG_INFO, "  [PTS]:         %02d:%02d:%02d.%03d\n", ptsH, ptsM, ptsS, ptsMS);

    int dts   = (pkt->dts < 0 ? 0 : pkt->dts) * (av_q2d(stream->time_base) * 1000);
    int dtsH  = dts / 1000 / 3600;
    int dtsM  = (dts / 1000 / 60) % 60;
    int dtsS  = (dts / 1000) % 60;
    int dtsMS = (dts % 1000);
    av_log(fmtCtx, AV_LOG_INFO, "  [DTS]:         %02d:%02d:%02d.%03d\n", dtsH, dtsM, dtsS, dtsMS);

    av_log(fmtCtx, AV_LOG_INFO, "--------------------------\n");
}

// 输出视频帧信息
void FuncPrintVideoFrame(AVFormatContext* fmtCtx, AVStream* stream, AVPacket* pkt) {
    av_log(fmtCtx, AV_LOG_INFO, "Video frame info.\n");
    av_log(fmtCtx, AV_LOG_INFO, "  [INDEX]:       %d\n",          pkt->stream_index);
    av_log(fmtCtx, AV_LOG_INFO, "  [SIZE]:        %d\n",          pkt->size);
    av_log(fmtCtx, AV_LOG_INFO, "  [POS]:         %ld (bytes)\n", pkt->pos);
    av_log(fmtCtx, AV_LOG_INFO, "  [DURATION]:    %.3f (ms)\n",   pkt->duration * (av_q2d(stream->time_base) * 1000));
    av_log(fmtCtx, AV_LOG_INFO, "  [TIME BASE]:   %d/%d\n",       stream->time_base.num, stream->time_base.den);
    
    int pts   = (pkt->pts < 0 ? 0 : pkt->pts) * (av_q2d(stream->time_base) * 1000);
    int ptsH  = pts / 1000 / 3600;
    int ptsM  = (pts / 1000 / 60) % 60;
    int ptsS  = (pts / 1000) % 60;
    int ptsMS = (pts % 1000);
    av_log(fmtCtx, AV_LOG_INFO, "  [PTS]:         %02d:%02d:%02d.%03d\n", ptsH, ptsM, ptsS, ptsMS);

    int dts   = (pkt->dts < 0 ? 0 : pkt->dts) * (av_q2d(stream->time_base) * 1000);
    int dtsH  = dts / 1000 / 3600;
    int dtsM  = (dts / 1000 / 60) % 60;
    int dtsS  = (dts / 1000) % 60;
    int dtsMS = (dts % 1000);
    av_log(fmtCtx, AV_LOG_INFO, "  [DTS]:         %02d:%02d:%02d.%03d\n", dtsH, dtsM, dtsS, dtsMS);

    av_log(fmtCtx, AV_LOG_INFO, "--------------------------\n");
}

} // namespace TEST_AV_FORMAT_FUNC

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
        TEST_AV_FORMAT_FUNC::FuncPrintBasicStreamInfo(fmtCtx);

        // 输出媒体流信息
        for (int idx = 0; idx < fmtCtx->nb_streams; ++idx) {
            AVStream* stream = fmtCtx->streams[idx];
            if (AVMEDIA_TYPE_AUDIO == stream->codecpar->codec_type) {
                TEST_AV_FORMAT_FUNC::FuncPrintAudioStreamInfo(fmtCtx, stream);
            }
            else if (AVMEDIA_TYPE_VIDEO == stream->codecpar->codec_type) {
                TEST_AV_FORMAT_FUNC::FuncPrintVideoStreamInfo(fmtCtx, stream);
            }
        }
    } while(false);

    // 资源释放
    avformat_close_input(&fmtCtx);
}

// 测试输出文件的前100条帧信息
void TestShowFstHundredFrames() {
    // 指定打开的文件名称
    std::string filename = "../../resource/output.mp4";

    AVFormatContext* fmtCtx = nullptr;
    AVInputFormat*   inFmt  = nullptr;
    AVDictionary*    dict   = nullptr;

    char errBuf[1024] = "0";
    do { 
        // 打开文件
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

        // 获取媒体流索引
        int audioIdx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        int videoIdx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        
        av_log(fmtCtx, AV_LOG_INFO, "Open file success.\n");
        TEST_AV_FORMAT_FUNC::FuncPrintBasicStreamInfo(fmtCtx);

        AVStream* audioStream = nullptr;
        if (audioIdx >= 0) {
            audioStream = fmtCtx->streams[audioIdx];
            TEST_AV_FORMAT_FUNC::FuncPrintAudioStreamInfo(fmtCtx, audioStream);
        }

        AVStream* videoStream = nullptr;
        if (videoIdx >= 0) {
            videoStream = fmtCtx->streams[videoIdx];
            TEST_AV_FORMAT_FUNC::FuncPrintVideoStreamInfo(fmtCtx, videoStream);
        }

        if (audioIdx < 0 && videoIdx < 0) {
            av_log(fmtCtx, AV_LOG_ERROR, "No audio and video stream found.\n");
            break;
        }

        // 获取前100帧信息
        AVPacket* pkt = av_packet_alloc();
        for (int cnt = 0; cnt < 100; ++cnt) { 
            // 读取一帧数据
            ret = av_read_frame(fmtCtx, pkt);
            if (ret < 0) {
                av_log(fmtCtx, AV_LOG_ERROR, "av_read_frame() failed. err info: %s\n", errBuf);
                break;
            }

            // 输出一帧数据
            if (audioIdx == pkt->stream_index) {
                TEST_AV_FORMAT_FUNC::FuncPrintAudioFrame(fmtCtx, audioStream, pkt);
            }
            else if (videoIdx == pkt->stream_index) {
                TEST_AV_FORMAT_FUNC::FuncPrintVideoFrame(fmtCtx, videoStream, pkt);
            }
            else {
                av_log(fmtCtx, AV_LOG_ERROR, "av_read_frame() error. Unknown stream index: %d\n", pkt->stream_index);
            }

            av_packet_unref(pkt);
        }

        av_packet_free(&pkt);

    } while(false);

    // 释放媒体资源
    avformat_close_input(&fmtCtx);
}

} // namespace TEST_AV_FORMAT