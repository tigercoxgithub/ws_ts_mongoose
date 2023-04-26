#ifndef STREAMFRAMES_H
#define STREAMFRAMES_H

#include "logging.h"
#include <libavcodec/avcodec.h> //ffmpeg
#include <libavformat/avformat.h> //ffmpeg
#include <libavutil/pixdesc.h> //ffmpeg pixel format

int streamframes(const char* inputName, char** initialised, AVFormatContext **pFormatContext, int *video_stream_index, AVCodecContext **pCodecContext );
int get_frames(AVFormatContext *pFormatContext, int video_stream_index, AVCodecContext *pCodecContext, unsigned char** imageDataBuffer, size_t *imageDataSize);
int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame, unsigned char** imageDataBuffer, size_t *imageDataSize);
int save_frame_as_jpeg(AVFrame *pFrame, char* frame_filename,  unsigned char** imageDataBuffer, size_t *imageDataSize);



#endif /* STREAMFRAMES_H */