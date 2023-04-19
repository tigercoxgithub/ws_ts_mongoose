#ifndef STREAMFRAMES_H
#define STREAMFRAMES_H

#include "logging.h"
#include <libavcodec/avcodec.h> //ffmpeg
#include <libavformat/avformat.h> //ffmpeg

int streamframes(const char* inputName, char** changable);

#endif /* STREAMFRAMES_H */