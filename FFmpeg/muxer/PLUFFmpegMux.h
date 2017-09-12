#ifndef PLUFFMPEGMUX_H_
#define PLUFFMPEGMUX_H_

#include <stdint.h>
enum plu_packet_type {
    PACKET_VIDEO,
    PACKET_AUDIO
};

typedef struct plu_packet_info {
    int         base_num;
    int         base_den;
    int64_t     pts;
    int64_t     dts;
    void*       buffer;
    int         buffer_size;
    bool        keyframe;
    int         stream_index;
    enum plu_packet_type type;
}plu_packet_info;


typedef union UMediaInfo {
    struct SVideoParam {
        int width;
        int height;
        int v_bitrate;
        const char *v_codec_name;
        int fps_num;
        int fps_den;
        void *v_extradata;
        int v_extradata_size;
        int stream_index;

    }video;

    struct SAudioParam {
        int a_bitrate;
        int samplerate;
        int audio_channel;
        const char *a_codec_name;
        void *a_extradata;
        int a_extradata_size;
        int stream_index;

    }audio;
}UMediaInfo;

typedef struct SCodecInfo {
    enum plu_packet_type    media_type;
    UMediaInfo              media_info;
}SCodecInfo;


typedef struct  CodecParam
{
    CodecParam():codec_info(NULL),
        file_name{ 0 }, muxer_settings{0}
    {
        nb_streams = 0;
    }
    char file_name[1024];
    char muxer_settings[1024];
    
    SCodecInfo  *codec_info;
    int         nb_streams;
}CodecParam;

typedef void* PLUHandler;

PLUHandler plumux_initparam(const CodecParam *param);
int plumux_write_packet(PLUHandler pHandler, const plu_packet_info *packet);
int plumux_write_trailer(PLUHandler pHandler);

#endif // if