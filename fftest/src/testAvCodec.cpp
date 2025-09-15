#include <string>
#include <thread>
#include <memory>
#include <functional>

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "testAvCodec.h"

namespace TEST_AV_CODEC {

namespace TEST_AV_CODEC_DATA {

typedef struct InputContext {
    using Ptr = std::shared_ptr<InputContext>;
    using WkPtr = std::weak_ptr<InputContext>;

    //               输入文件名
    std::string      inFileName;

    //               输入文件格式上下文
    AVFormatContext* inFmtCtx      { nullptr };

    //               输入音频流
    AVStream*        inAudioStream { nullptr };

    //               输入视频流
    AVStream*        inVideoStream { nullptr };

    //               输出读取的压缩数据
    AVPacket*        inPkt { nullptr };

    ~InputContext() {
        av_packet_free(&inPkt);
        avformat_close_input(&inFmtCtx);
    }

} InputContext;

typedef struct OutputContext {
    using Ptr = std::shared_ptr<OutputContext>;
    using WkPtr = std::weak_ptr<OutputContext>;
    using OutputFileCb = std::function<void(AVFrame*)>;

    //               输出解码后的帧数据
    AVFrame*         outFrame { nullptr };

    /** ----------------- audio -------------------- */

    //               输出音频文件名
    std::string      outAudioFileName;

    //               输出音频文件写入计数
    int              outAudioFileWriteCount { 0 };

    //               输出音频文件
    FILE*            outAudioFile { nullptr };

    //               输出音频文件回调函数
    OutputFileCb     outAudioFileCb { nullptr };

    //               输出音频媒体流索引
    int              outAudioStreamIdx { -1 };

    //               输出音频流
    AVStream*        outAudioStream { nullptr };

    //               输出音频编码器上下文
    AVCodecContext*  outAudioCodecCtx { nullptr };

    /** ------------------ video ------------------- */

    //               输出视频文件名
    std::string      outVideoFileName;

    //               输出视频文件写入计数
    int              outVideoFileWriteCount { 0 };

    //               输出视频文件
    FILE*            outVideoFile { nullptr };

    //               输出视频文件回调函数
    OutputFileCb     outVideoFileCb { nullptr };

    //               输出视频流
    AVStream*        outVideoStream { nullptr };

    //               输出视频媒体流索引
    int              outVideoStreamIdx { -1 };

    //               输出视频编码器上下文
    AVCodecContext*  outVideoCodecCtx { nullptr };

    //               输出视频图像缓存
    uint8_t*         outVideoImageBuffer[4] { nullptr }; 

    //               输出视频图像缓存大小
    int              outVideoImageBufSize { -1 };

    //               输出视频图像缓存行字节数
    int              outVideoImageLineSize[4] { 0 };

    ~OutputContext() {
        av_frame_free(&outFrame);

        fclose(outAudioFile);
        avcodec_free_context(&outAudioCodecCtx);

        fclose(outVideoFile);
        av_free(outVideoImageBuffer[0]);
        avcodec_free_context(&outVideoCodecCtx);
    }

} OutputContext;

} // namespace TEST_AV_CODEC_DATA

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

// 功能函数 - 解码一帧数据
static int FuncDecode(const AVPacket* inPkt, 
    AVCodecContext* outDecCtx, AVFrame* outFrame, TEST_AV_CODEC_DATA::OutputContext::OutputFileCb outFileCb) {
    
    // 将编码后的压缩数据送入解码器
    char errbuf[1024] = {0};
    int ret = avcodec_send_packet(outDecCtx, inPkt);
    if (ret < 0) {
        av_strerror(ret, errbuf, sizeof(errbuf));
        av_log(NULL, AV_LOG_ERROR, "avcodec_send_packet() failed. ret: %d, errbuf: %s\n", ret, errbuf);
        return -1;
    }

    while (true) {
        // 从解码器中获取解码后的数据
        ret = avcodec_receive_frame(outDecCtx, outFrame);

        if (ret < 0) {
            // 解码失败
            if (ret == AVERROR(EAGAIN)) {
                av_log(nullptr, AV_LOG_WARNING, "Not enough packets for output frame from decoder.\n");
                break;
            }
            else if (ret == AVERROR_EOF) {
                av_log(nullptr, AV_LOG_WARNING, "No more frames to receive from decoder.\n");
            }
            else {
                av_strerror(ret, errbuf, sizeof(errbuf));
                av_log(nullptr, AV_LOG_ERROR, "avcodec_receive_frame() failed. ret: %d, errbuf: %s\n", ret, errbuf);
            }

            return ret;
        }
        else {
            // 将解码后的原始数据写入文件
            if (nullptr != outFileCb) {
                outFileCb(outFrame);
            }
        }

        av_frame_unref(outFrame);
    }

    return 0;
}

// 功能函数 - 打开解码器
static int FuncOpenDecoder(AVFormatContext* inFmtCtx, AVMediaType inType, 
    int* outStreamIdxPtr, AVStream** outStreamPtr, AVCodecContext** outCodecCtxPtr, const std::string& outFileName, FILE** outFilePtr) { 
    char errbuf[1024] = {0};

    // 查找输入媒体流信息
    int outStreamIdx = av_find_best_stream(inFmtCtx, inType, -1, -1, nullptr, 0);
    if (outStreamIdx < 0) {
        av_strerror(outStreamIdx, errbuf, sizeof(errbuf));
        av_log(nullptr, AV_LOG_ERROR, "av_find_best_stream() failed. type: %s. ret: %d. errbuf: %s\n", 
            av_get_media_type_string(inType), outStreamIdx, errbuf);
        return -1;
    }

    // 查找输入媒体流解码器
    AVStream* inStream = inFmtCtx->streams[outStreamIdx];
    const AVCodec* inCodec = avcodec_find_decoder(inStream->codecpar->codec_id);
    if (nullptr == inCodec) {
        av_log(nullptr, AV_LOG_ERROR, "avcodec_find_encoder() failed. codec name: %s. type: %s\n", 
            avcodec_get_name(inStream->codecpar->codec_id), av_get_media_type_string(inType));
        return -1;
    }

    // 创建输出解码器
    AVCodecContext* outCodecCtx = avcodec_alloc_context3(inCodec);
    if (nullptr == outCodecCtx) {
        av_log(nullptr, AV_LOG_ERROR, "avcodec_alloc_context3() failed. codec name: %s. type: %s\n", 
            avcodec_get_name(inStream->codecpar->codec_id), av_get_media_type_string(inType));
        return -1;
    }

    // 输出解码器参数赋值
    int ret = avcodec_parameters_to_context(outCodecCtx, inStream->codecpar);
    if (ret < 0) {
        av_strerror(ret, errbuf, sizeof(errbuf));
        av_log(nullptr, AV_LOG_ERROR, "avcodec_parameters_to_context() failed. type: %s. ret: %d. errbuf: %s\n", 
            av_get_media_type_string(inType), outStreamIdx, errbuf);
        return -1;
    }

    // 打开解码器
    ret = avcodec_open2(outCodecCtx, inCodec, nullptr);
    if (0 != ret) {
        av_strerror(ret, errbuf, sizeof(errbuf));
        av_log(nullptr, AV_LOG_ERROR, "avcodec_open2() failed. type: %s. ret: %d. errbuf: %s\n", 
            av_get_media_type_string(inType), outStreamIdx, errbuf);
        return -1;
    }

    // 打开输出文件
    FILE* fp = fopen(outFileName.c_str(), "wb");
    if (nullptr == fp) {
        av_log(nullptr, AV_LOG_ERROR, "fopen() failed. file: %s\n", outFileName.c_str());
        return -1;
    }

    (*outFilePtr)      = fp;
    (*outStreamPtr)    = inStream;
    (*outStreamIdxPtr) = outStreamIdx;
    (*outCodecCtxPtr)  = outCodecCtx;
    return 0;
}

} // namespace TEST_AV_CODEC_FUNC

// 测试编码
void TestEncode() {
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

// 测试解码
void TestDecode() {
    // 输入
    auto inCtx = std::make_shared<TEST_AV_CODEC_DATA::InputContext>();
    {
        inCtx->inFileName = "../../resource/video3.mp4";
        
        // 创建输入格式上下文
        char errbuf[1024] = {0};
        int ret = avformat_open_input(&(inCtx->inFmtCtx), inCtx->inFileName.c_str(), nullptr, nullptr);
        if (0 != ret) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(inCtx->inFmtCtx, AV_LOG_ERROR, "avformat_open_input() failed. ret: %d, errbuf: %s\n", ret, errbuf);
            return;
        }

        // 查找输入媒体流信息
        ret = avformat_find_stream_info(inCtx->inFmtCtx, nullptr);
        if (ret < 0) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(inCtx->inFmtCtx, AV_LOG_ERROR, "avformat_find_stream_info() failed. ret: %d, errbuf: %s\n", ret, errbuf);
            return;
        }

        // 创建输入解码前数据的缓存
        inCtx->inPkt = av_packet_alloc();
        if (nullptr == inCtx->inPkt) {
            av_log(inCtx->inFmtCtx, AV_LOG_ERROR, "av_packet_alloc() failed.\n");
            return;
        }

        av_log(inCtx->inFmtCtx, AV_LOG_INFO, "Open input success. file name: %s--------------------\n", inCtx->inFileName.c_str());
    }

    // 输出
    auto outCtx = std::make_shared<TEST_AV_CODEC_DATA::OutputContext>();
    {
        // 创建输出解码后数据的缓存
        outCtx->outFrame = av_frame_alloc();
        if (nullptr == outCtx->outFrame) {
            av_log(nullptr, AV_LOG_ERROR, "av_frame_alloc() failed.\n");
            return;
        }

        // 音频输出
        outCtx->outAudioFileName = "./output.pcm";
        if (0 == TEST_AV_CODEC_FUNC::FuncOpenDecoder(inCtx->inFmtCtx, AVMEDIA_TYPE_AUDIO, &(outCtx->outAudioStreamIdx), 
            &(outCtx->outAudioStream), &(outCtx->outAudioCodecCtx), outCtx->outAudioFileName, &(outCtx->outAudioFile))) {

            // 指定写音频文件回调函数
            TEST_AV_CODEC_DATA::OutputContext::WkPtr wkOutCtx = outCtx;
            outCtx->outAudioFileCb = [wkOutCtx](AVFrame* outFrame) {
                auto outCtx = wkOutCtx.lock();
                if (nullptr == outCtx) {
                    av_log(nullptr, AV_LOG_ERROR, "Write audio file failed. global output context invalid\n");
                    return;
                }

                std::size_t writeSize = outFrame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(outFrame->format));

                char timebuf[AV_TS_MAX_STRING_SIZE] = {0};
                av_log(NULL, AV_LOG_INFO, "Write audio file. count: %d. samples: %d. size: %lu. pts: %s\n", 
                    ++outCtx->outAudioFileWriteCount, outFrame->nb_samples, writeSize, 
                    av_ts_make_time_string(timebuf, outFrame->pts, &(outCtx->outAudioCodecCtx->time_base)));

                fwrite(outFrame->extended_data[0], 1, writeSize, outCtx->outAudioFile);
            };

            av_log(nullptr, AV_LOG_INFO, "Open audio output success. file name: %s--------------------\n", outCtx->outAudioFileName.c_str());
        }

        // 视频输出
        outCtx->outVideoFileName = "./output.yuv";
        if (0 == TEST_AV_CODEC_FUNC::FuncOpenDecoder(inCtx->inFmtCtx, AVMEDIA_TYPE_VIDEO, &(outCtx->outVideoStreamIdx), 
            &(outCtx->outVideoStream), &(outCtx->outVideoCodecCtx), outCtx->outVideoFileName, &(outCtx->outVideoFile))) {

            // 分配视频输出缓存
            char errbuf[1024] = {0};

            outCtx->outVideoImageBufSize = av_image_alloc(
                outCtx->outVideoImageBuffer, 
                outCtx->outVideoImageLineSize,
                outCtx->outVideoCodecCtx->width, 
                outCtx->outVideoCodecCtx->height,
                outCtx->outVideoCodecCtx->pix_fmt, 
                1
            );

            if (outCtx->outVideoImageBufSize < 0) {
                av_strerror(outCtx->outVideoImageBufSize, errbuf, sizeof(errbuf));
                av_log(nullptr, AV_LOG_ERROR, "av_image_alloc() failed. ret: %d, errbuf: %s\n", outCtx->outVideoImageBufSize, errbuf);
                return;
            }

            // 指定写视频文件回调函数
            TEST_AV_CODEC_DATA::OutputContext::WkPtr wkOutCtx = outCtx;
            outCtx->outVideoFileCb = [wkOutCtx](AVFrame* outFrame) {
                auto outCtx = wkOutCtx.lock();
                if (nullptr == outCtx) {
                    av_log(nullptr, AV_LOG_ERROR, "Write video file failed. global output context invalid\n");
                    return;
                }

                if (outFrame->width  != outCtx->outVideoCodecCtx->width  || 
                    outFrame->height != outCtx->outVideoCodecCtx->height ||
                    outFrame->format != outCtx->outVideoCodecCtx->pix_fmt) {
                    av_log(nullptr, AV_LOG_ERROR, "Write video file failed. frame format invalid. frame fomat: %d * %d %s.\
                        codec format: %d * %d %s\n", 
                        outFrame->width, outFrame->height, 
                        av_get_pix_fmt_name(static_cast<AVPixelFormat>(outFrame->format)), 
                        outCtx->outVideoCodecCtx->width, outCtx->outVideoCodecCtx->height, 
                        av_get_pix_fmt_name(outCtx->outVideoCodecCtx->pix_fmt));
                    return;
                }

                av_log(nullptr, AV_LOG_INFO, "Write video file. count: %d. codec: %c\n", 
                    ++outCtx->outVideoFileWriteCount, av_get_picture_type_char(outFrame->pict_type));

                uint8_t** srcOutBuffer = reinterpret_cast<uint8_t**>(outFrame->data);
                uint8_t** dstOutBuffer = reinterpret_cast<uint8_t**>(outCtx->outVideoImageBuffer);

                av_image_copy(
                    dstOutBuffer, outCtx->outVideoImageLineSize,
                    srcOutBuffer, outFrame->linesize,
                    outCtx->outVideoCodecCtx->pix_fmt, outCtx->outVideoCodecCtx->width, outCtx->outVideoCodecCtx->height
                );

                fwrite(outCtx->outVideoImageBuffer[0], 1, outCtx->outVideoImageBufSize, outCtx->outVideoFile);
            };

            av_log(nullptr, AV_LOG_INFO, "Open video output success. file name: %s--------------------\n", outCtx->outVideoFileName.c_str());
        }

        if (nullptr == outCtx->outAudioStream && nullptr == outCtx->outVideoStream) {
            av_log(nullptr, AV_LOG_ERROR, "Open output failed. Please check the input file: %s\n", inCtx->inFileName.c_str());
            return;
        }
    }

    // 解码
    {
        int ret = 0;
        char errBuf[1024] = {0};

        av_log(nullptr, AV_LOG_INFO, "Decode start. --------------------\n");
        while (true) {
            ret = av_read_frame(inCtx->inFmtCtx, inCtx->inPkt);
            if (ret < 0) {
                if (ret == AVERROR_EOF) {
                    av_log(nullptr, AV_LOG_INFO, "Decode end of input file.\n");
                } 
                else {
                    av_strerror(ret, errBuf, sizeof(errBuf));
                    av_log(nullptr, AV_LOG_ERROR, "av_read_frame() failed. ret: %d. errbuf: %s\n", ret, errBuf);
                }
                
                break;
            }

            if (outCtx->outAudioStreamIdx == inCtx->inPkt->stream_index) {
                ret = TEST_AV_CODEC_FUNC::FuncDecode(inCtx->inPkt, outCtx->outAudioCodecCtx, outCtx->outFrame, outCtx->outAudioFileCb);
            }
            else if (outCtx->outVideoStreamIdx == inCtx->inPkt->stream_index) {
                ret = TEST_AV_CODEC_FUNC::FuncDecode(inCtx->inPkt, outCtx->outVideoCodecCtx, outCtx->outFrame, outCtx->outVideoFileCb);
            }
            else {
                av_log(nullptr, AV_LOG_WARNING, "av_read_frame() warning. invalid stream index: %d\n", inCtx->inPkt->stream_index);
            }

            av_packet_unref(inCtx->inPkt);

            if (ret < 0) {
                break;
            }
        }

        // 刷新解码器剩余数据
        av_log(nullptr, AV_LOG_INFO, "Decode end. --------------------\n");
        if (nullptr != outCtx->outVideoCodecCtx) {
            TEST_AV_CODEC_FUNC::FuncDecode(nullptr, outCtx->outVideoCodecCtx, outCtx->outFrame, outCtx->outVideoFileCb);
        }

        if (nullptr != outCtx->outAudioCodecCtx) {
            TEST_AV_CODEC_FUNC::FuncDecode(nullptr, outCtx->outAudioCodecCtx, outCtx->outFrame, outCtx->outAudioFileCb);
        }
    }
    
    // 播放方式提示
    {
        if (nullptr != outCtx->outVideoStream) {
            av_log(nullptr, AV_LOG_INFO, "Play the output video file with the command:\n\
                ffplay -f rawvideo -i %s -pixel_format %s -video_size %dx%d\n",
                outCtx->outVideoFileName.c_str(),
                av_get_pix_fmt_name(outCtx->outVideoCodecCtx->pix_fmt), 
                outCtx->outVideoCodecCtx->width, 
                outCtx->outVideoCodecCtx->height);
        }

        if (nullptr != outCtx->outAudioStream) {
            auto sampleFmt = outCtx->outAudioCodecCtx->sample_fmt;
            auto channels = outCtx->outAudioCodecCtx->ch_layout.nb_channels;

            if (av_sample_fmt_is_planar(sampleFmt)) {
                const char* packed = av_get_sample_fmt_name(sampleFmt);
                av_log(nullptr, AV_LOG_WARNING, "Warning: the sample format the decoder produced is planar \
                    (%s). This example will output the first channel only.\n", packed ? packed : "?");

                sampleFmt = av_get_packed_sample_fmt(sampleFmt);
                channels = 1;
            }

            std::string sampleFmtName;
            if (AV_SAMPLE_FMT_U8 == sampleFmt) {
                sampleFmtName = "u8";  
            }
            else if (AV_SAMPLE_FMT_S16 == sampleFmt) {
                sampleFmtName = AV_NE("s16be", "s16le");
            }
            else if (AV_SAMPLE_FMT_S32 == sampleFmt) {
                sampleFmtName = AV_NE("s32be", "s32le");
            }
            else if (AV_SAMPLE_FMT_FLT == sampleFmt) {
                sampleFmtName = AV_NE("f32be", "f32le");
            }
            else if (AV_SAMPLE_FMT_DBL == sampleFmt) {
                sampleFmtName = AV_NE("f64be", "f64le");
            }

            av_log(nullptr, AV_LOG_INFO, "Play the output audio file with the command:\n\
                ffplay -f %s -ac %d -ar %d %s\n",
                sampleFmtName.c_str(), channels, outCtx->outAudioCodecCtx->sample_rate, outCtx->outAudioFileName.c_str());
        }
    }
}

}; // namespace TEST_AV_CODEC