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
    enum plu_packet_type type;
}plu_packet_info;

typedef struct  CodecParam
{
    const char *file_name;
    char *muxer_settings;

    //video parameter
    int has_video;
    int width;
    int height;
    int v_bitrate;
    const char *v_codec_name;
    int fps_num;
    int fps_den;
    void *v_extradata;
    int v_extradata_size;

    //audio parameter
    int has_audio;
    int a_bitrate;
    int samplerate;
    int audio_channel;
    const char *a_codec_name;
    void *a_extradata;
    int a_extradata_size;
}CodecParam;

typedef void* PLUHandler;

PLUHandler plumux_initparam(const CodecParam *param);
int plumux_write_packet(PLUHandler pHandler, const plu_packet_info *packet);
int plumux_write_trailer(PLUHandler pHandler);

#endif // if