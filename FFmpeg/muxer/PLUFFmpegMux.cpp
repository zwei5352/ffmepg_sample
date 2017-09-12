#ifdef _WIN32
#define  _CRT_SECURE_NO_WARNINGS
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#define inline __inline

#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "PLUFFmpegMux.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavutil/version.h"
#ifdef __cplusplus
}
#endif

#define LOG_PRINT(fmt, ...)  do{\
                                fprintf(stderr, "[%s %d]---" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
                                }while(0)
#define LOG_INFO(fmt, ...) LOG_PRINT(fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) LOG_PRINT(fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG_PRINT(fmt, ##__VA_ARGS__)

typedef struct PLUMuxContext {
    PLUMuxContext() :out_fmt_ctx(NULL)
    {

    }
    ~PLUMuxContext()
    {
        if (out_fmt_ctx) {
            if ((out_fmt_ctx->oformat->flags & AVFMT_NOFILE) == 0 &&
                out_fmt_ctx->pb) {
                avio_closep(&out_fmt_ctx->pb);
            }
            avformat_free_context(out_fmt_ctx);
            out_fmt_ctx = NULL;
        }
        if (codec_param.codec_info) {
            delete codec_param.codec_info;
            codec_param.codec_info = NULL;
            codec_param.nb_streams = 0;
        }
    }
    int new_codec_param(int nb_streams)
    {
        codec_param.nb_streams = nb_streams;
        codec_param.codec_info = new SCodecInfo[nb_streams];
        return 0;
    }
    CodecParam codec_param;
    AVFormatContext  *out_fmt_ctx;
}PLUMuxContext;

static AVStream* plu_new_stream(PLUMuxContext *mux_ctx, AVFormatContext  *out_fmt_ctx, const char *codec_name) {
    CodecParam *codec_info = &mux_ctx->codec_param;
    AVStream *out_stream = NULL;
    AVCodec *codec = NULL;
    plu_packet_info pack_info = { 0 };

    if ((codec = avcodec_find_encoder_by_name(codec_name)) == NULL) {
        LOG_ERROR("Failed find encod name [%s]\n", codec_name);
        return NULL;
    }
    out_stream = avformat_new_stream(out_fmt_ctx, codec);
    if (!out_stream) {
        LOG_ERROR("Failed allocating output stream\n");
        return NULL;
    }

    return out_stream;
}

static int plu_init_output(PLUMuxContext *mux_ctx)
{
    int ret;
    AVFormatContext  *out_fmt_ctx = NULL;
    AVOutputFormat *output_format;
    CodecParam *codec_param = &mux_ctx->codec_param;

    output_format = av_guess_format(NULL, codec_param->file_name, NULL);
    if (output_format == NULL) {
        LOG_ERROR("Couldn't find an appropriate muxer for '%s'\n", codec_param->file_name);
        return -1;
    }

    ret = avformat_alloc_output_context2(&out_fmt_ctx, output_format, NULL, NULL);
    if (ret < 0) {
        LOG_ERROR("Couldn't initialize output context: %d\n", (ret));
        goto ERROR_EXIT;
    }

    for (int i = 0; i < codec_param->nb_streams; i++) {
        SCodecInfo codec_info = codec_param->codec_info[i];
        AVCodecContext *codec_ctx = NULL;
        AVStream *stream;
        if (codec_info.media_type == PACKET_VIDEO) {
            if ((stream = plu_new_stream(mux_ctx, out_fmt_ctx, codec_info.media_info.video.v_codec_name )) == NULL)
                goto ERROR_EXIT;

            codec_ctx = stream->codec;
            codec_ctx->bit_rate = codec_info.media_info.video.v_bitrate * 1000;
            codec_ctx->width = codec_info.media_info.video.width;
            codec_ctx->height = codec_info.media_info.video.height;
            codec_ctx->coded_width = codec_info.media_info.video.width;
            codec_ctx->coded_height = codec_info.media_info.video.height;
            codec_ctx->extradata_size = codec_info.media_info.video.v_extradata_size;
            codec_ctx->extradata = (uint8_t*)av_memdup(codec_info.media_info.video.v_extradata, codec_ctx->extradata_size);
            codec_ctx->time_base = { codec_info.media_info.video.fps_den, codec_info.media_info.video.fps_num };
            codec_ctx->framerate = { codec_info.media_info.video.fps_num , codec_info.media_info.video.fps_den };
            codec_ctx->codec_tag = 0;
            if (out_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        else if (codec_info.media_type == PACKET_AUDIO) {
            if ((stream = plu_new_stream(mux_ctx, out_fmt_ctx, codec_info.media_info.audio.a_codec_name)) == NULL)
                goto ERROR_EXIT;

            codec_ctx = stream->codec;
            codec_ctx->bit_rate = codec_info.media_info.audio.a_bitrate * 1000;
            codec_ctx->channels = codec_info.media_info.audio.audio_channel;
            codec_ctx->sample_rate = codec_info.media_info.audio.samplerate;
            codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
            codec_ctx->extradata_size = codec_info.media_info.audio.a_extradata_size;
            codec_ctx->extradata = (uint8_t*)av_memdup(codec_info.media_info.audio.a_extradata, codec_ctx->extradata_size);
            codec_ctx->channel_layout = av_get_default_channel_layout(codec_ctx->channels);
            codec_ctx->time_base = { 1, codec_ctx->sample_rate };
            codec_ctx->codec_tag = 0;
            if (out_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
    }
    av_dump_format(out_fmt_ctx, 0, codec_param->file_name, 1);
    mux_ctx->out_fmt_ctx = out_fmt_ctx;
    return 0;

ERROR_EXIT:
    if (out_fmt_ctx) {
        if ((out_fmt_ctx->oformat->flags & AVFMT_NOFILE) == 0 &&
            out_fmt_ctx->pb) {
            avio_closep(&out_fmt_ctx->pb);
        }
        avformat_free_context(out_fmt_ctx);
    }
    return -1;
}

static int plu_open_output(PLUMuxContext *mux_ctx)
{
    int ret;
    AVFormatContext  *out_fmt_ctx = mux_ctx->out_fmt_ctx;
    AVOutputFormat *format = mux_ctx->out_fmt_ctx->oformat;
    CodecParam *codec_info = &mux_ctx->codec_param;
    if ((format->flags & AVFMT_NOFILE) == 0) {
        ret = avio_open(&out_fmt_ctx->pb, codec_info->file_name, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOG_ERROR("Couldn't open '%s', %d", codec_info->file_name, ret);
            return -1;
        }
    }

    AVDictionary *dict = NULL;
    if (codec_info->muxer_settings &&
        (ret = av_dict_parse_string(&dict, codec_info->muxer_settings, "=", ",", 0))) {
        LOG_ERROR("Failed to parse muxer settings: %d\n%s",
            ret, codec_info->muxer_settings);
        av_dict_free(&dict);
    }
    if (av_dict_count(dict) > 0) {
        AVDictionaryEntry *entry = NULL;
        while ((entry = av_dict_get(dict, "", entry, AV_DICT_IGNORE_SUFFIX)))
            LOG_INFO("\n\t%s=%s", entry->key, entry->value);
        LOG_INFO("\n");
    }

    ret = avformat_write_header(out_fmt_ctx, &dict);
    if (ret < 0) {
        LOG_ERROR("Error opening '%s': %d", codec_info->file_name, ret);
        av_dict_free(&dict);
        if ((out_fmt_ctx->oformat->flags & AVFMT_NOFILE) == 0)
            avio_closep(&out_fmt_ctx->pb);
        return -1;
    }

    av_dict_free(&dict);

    return 0;
}

PLUHandler plumux_initparam(const CodecParam *param)
{
    PLUMuxContext *mux_ctx = new PLUMuxContext();
    CodecParam *codec_param = &mux_ctx->codec_param;
    snprintf(codec_param->file_name, sizeof(codec_param->file_name), param->file_name);
    snprintf(codec_param->muxer_settings, sizeof(codec_param->muxer_settings), param->muxer_settings);

    if (param->nb_streams > 0) {
        mux_ctx->new_codec_param(param->nb_streams);
        for (int i = 0; i < param->nb_streams; i++) {
            memcpy(&codec_param->codec_info[i], &param->codec_info[i], sizeof(*param->codec_info));
        }
    }

    if (plu_init_output(mux_ctx) < 0) {
        delete mux_ctx;
        return NULL;
    }
    if (plu_open_output(mux_ctx) < 0) {
        delete mux_ctx;
        return NULL;
    }

    return mux_ctx;
}

int plumux_write_packet(PLUHandler pHandler, const plu_packet_info *packet_info)
{
    PLUMuxContext *mux_ctx = (PLUMuxContext *)pHandler;
    if (mux_ctx == NULL || mux_ctx->out_fmt_ctx == NULL)
        return -1;

    int index = packet_info->stream_index;
    AVPacket packet = { 0 };
    AVFormatContext  *out_fmt_ctx = mux_ctx->out_fmt_ctx;
    AVStream *stream = NULL;
    AVRational time_base_pkt = { packet_info->base_num, packet_info->base_den };

    if (index == -1) {
        return -1;
    }

    stream = mux_ctx->out_fmt_ctx->streams[index];
    av_init_packet(&packet);

    packet.data = (uint8_t*)packet_info->buffer;
    packet.size = (int)packet_info->buffer_size;
    packet.stream_index = index;
    packet.pts = av_rescale_q_rnd(packet_info->pts,
        time_base_pkt, stream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    packet.dts = av_rescale_q_rnd(packet_info->dts,
        time_base_pkt, stream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    if (packet_info->keyframe)
        packet.flags = AV_PKT_FLAG_KEY;
    return av_interleaved_write_frame(out_fmt_ctx, &packet);
}

int plumux_write_trailer(PLUHandler pHandler) {
    PLUMuxContext *mux_ctx = (PLUMuxContext *)pHandler;

    if (mux_ctx == NULL || mux_ctx->out_fmt_ctx==NULL)
        return -1;

    AVFormatContext  *out_fmt_ctx = mux_ctx->out_fmt_ctx;
    av_write_trailer(out_fmt_ctx);
    delete mux_ctx;
    return 0;
}