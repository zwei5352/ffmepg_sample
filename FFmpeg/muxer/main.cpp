#include "PLUFFmpegMux.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "libavutil/timestamp.h"
#include "libavformat/avformat.h"
#ifdef __cplusplus
}
#endif
// static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
// {
//     AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
//
//     printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
//         tag,
//         av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
//         av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
//         av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
//         pkt->stream_index);
// }

int main(int argc, char **argv)
{
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    CodecParam codec_param = { 0 };
    PLUHandler handler = NULL;
    const char *in_filename, *out_filename;
    int ret, i;

    in_filename = R"(D:\videoFile\720p_h264.mov)";
    out_filename = R"(..\out.ts)";

    av_register_all();

    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", in_filename);
        goto end;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        goto end;
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    codec_param.file_name = out_filename;

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVCodecContext *codec_ctx = in_stream->codec;
        const AVCodec *codec = codec_ctx->codec;
        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            codec_param.has_video = 1;
            codec_param.height = codec_ctx->height;
            codec_param.width = codec_ctx->width;
            codec_param.v_bitrate = codec_ctx->bit_rate;
            codec_param.fps_num = codec_ctx->framerate.num;
            codec_param.fps_den = codec_ctx->framerate.den;
            codec_param.v_codec_name = "libx264";//codec_ctx->codec->name;
            codec_param.v_extradata_size = codec_ctx->extradata_size;
            codec_param.v_extradata = codec_ctx->extradata;
        }
        if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            codec_param.has_audio = 1;
            codec_param.a_bitrate = codec_ctx->bit_rate;
            codec_param.samplerate = codec_ctx->sample_rate;
            codec_param.audio_channel - codec_ctx->channels;
            codec_param.a_codec_name = "aac";// codec_ctx->codec->name;
            codec_param.a_extradata_size = codec_ctx->extradata_size;
            codec_param.a_extradata = codec_ctx->extradata;
        }
    }

    handler = plumux_initparam(&codec_param);

    while (1) {
        AVStream *in_stream, *out_stream;
        plu_packet_info packet_info;
        AVRational time_base_pkt;
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
            break;

        in_stream = ifmt_ctx->streams[pkt.stream_index];

        //log_packet(ifmt_ctx, &pkt, "in");

        /* copy packet */
        if (in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            packet_info.type = PACKET_VIDEO;
            time_base_pkt = { codec_param.fps_den, codec_param.fps_num };
        }
        else if (in_stream->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            packet_info.type = PACKET_AUDIO;
            time_base_pkt = { 1, codec_param.samplerate };
        }
        else {
            continue;
        }
        pkt.pos = -1;
        //log_packet(ofmt_ctx, &pkt, "out");
        packet_info.buffer = pkt.data;
        packet_info.buffer_size = pkt.size;
        packet_info.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, time_base_pkt, (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet_info.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, time_base_pkt, (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        //packet_info.index = pkt.stream_index;
        packet_info.base_num = time_base_pkt.num;
        packet_info.base_den = time_base_pkt.den;

        ret = plumux_write_packet(handler, &packet_info);

        av_packet_unref(&pkt);
    }

    plumux_write_trailer(handler);

end:

    avformat_close_input(&ifmt_ctx);

    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %d\n", (ret));
        return 1;
    }

    return 0;
}