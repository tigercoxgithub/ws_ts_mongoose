#ifndef STREAMFRAMES_H
#define STREAMFRAMES_H

#include "logging.h"
#include <libavcodec/avcodec.h> //ffmpeg
#include <libavformat/avformat.h> //ffmpeg
#include <libavutil/pixdesc.h> //ffmpeg pixel format

struct videoArgs 
{
  struct mg_mgr *mgr;
  const char *inputName;
  int initialised;
  struct AVFormatContext *pFormatContext;
  int video_stream_index;
  struct AVCodecContext *pCodecContext;
  unsigned char* imageDataBuffer;
  size_t imageDataSize;
};
int streamframes(struct videoArgs *ARGS);
int get_frames(struct videoArgs *ARGS);
int decode_packet(AVPacket *pPacket, AVFrame *pFrame, struct videoArgs *ARGS);
int save_frame_as_jpeg(AVFrame *pFrame, char *frame_filename,  struct videoArgs *ARGS);



#endif /* STREAMFRAMES_H */