#include "streamframes.h" // Include the header file that contains the function declaration

#include "logging.h"
#include <libavcodec/avcodec.h> //ffmpeg
#include <libavformat/avformat.h> //ffmpeg

int streamframes(const char* inputName, char** changable)
{
   
    if(strcmp(*changable, "T") == 0) {
        logging("Got 'T' will change to 'S'");
        *changable = "S";
        return 0;
    }

    logging("Initializing all the containers, codecs and protocols.");

	// AVFormatContext holds the header information from the format (Container)
	// Allocating memory for this component
	// http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
	AVFormatContext *pFormatContext = avformat_alloc_context();
	if (!pFormatContext)
	{
		logging("ERROR could not allocate memory for Format Context");
		return -1;
	}

	logging("opening the input file (%s) and loading format (container) header",
		inputName);
	// Open the file and read its header. The codecs are not opened.
	// The function arguments are:
	// AVFormatContext (the component we allocated memory for),
	// url (filename),
	// AVInputFormat (if you pass NULL it'll do the auto detect)
	// and AVDictionary (which are options to the demuxer)
	// http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
	if (avformat_open_input(&pFormatContext, inputName, NULL, NULL) != 0)
	{
		logging("ERROR could not open the file");
		return -1;
	}

	// now we have access to some information about our file
	// since we read its header we can say what format (container) it's
	// and some other information related to the format itself.
	logging("format %s, duration %lld us, bit_rate %lld",
		pFormatContext->iformat->name, pFormatContext->duration,
		pFormatContext->bit_rate);

	logging("finding stream info from format");
	// read Packets from the Format to get stream information
	// this function populates pFormatContext->streams
	// (of size equals to pFormatContext->nb_streams)
	// the arguments are:
	// the AVFormatContext
	// and options contains options for codec corresponding to i-th stream.
	// On return each dictionary will be filled with options that were not found.
	// https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
	if (avformat_find_stream_info(pFormatContext, NULL) < 0)
	{
		logging("ERROR could not get the stream info");
		return -1;
	}

	// the component that knows how to enCOde and DECode the stream
	// it's the codec (audio or video)
	// http://ffmpeg.org/doxygen/trunk/structAVCodec.html
	AVCodec *pCodec = NULL;
	// this component describes the properties of a codec used by the stream i
	// https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html
	AVCodecParameters *pCodecParameters = NULL;
	int video_stream_index = -1;

	// loop though all the streams and print its main information
	for (unsigned int i = 0; i < pFormatContext->nb_streams; i++)
	{
		AVCodecParameters *pLocalCodecParameters = NULL;
		pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
		logging("AVStream->time_base before open coded %d/%d",
			pFormatContext->streams[i]->time_base.num,
			pFormatContext->streams[i]->time_base.den);
		logging("AVStream->r_frame_rate before open coded %d/%d",
			pFormatContext->streams[i]->r_frame_rate.num,
			pFormatContext->streams[i]->r_frame_rate.den);
		logging(
			"AVStream->start_time %" PRId64, pFormatContext->streams[i]->start_time);
		logging("AVStream->duration %" PRId64, pFormatContext->streams[i]->duration);

		logging("finding the proper decoder (CODEC)");

		AVCodec *pLocalCodec = NULL;

		// finds the registered decoder for a codec ID
		// https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
		pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

		if (pLocalCodec == NULL)
		{
			logging("ERROR unsupported codec!");
			// In this example if the codec is not found we just skip it
			continue;
		}

		// when the stream is a video we store its index, codec parameters and codec
		if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			if (video_stream_index == -1)
			{
				video_stream_index = i;
				pCodec = pLocalCodec;
				pCodecParameters = pLocalCodecParameters;
			}

			logging("Video Codec: resolution %d x %d", pLocalCodecParameters->width,
				pLocalCodecParameters->height);
		}
		else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			logging("Audio Codec: %d channels, sample rate %d",
				pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
		}

		// print its name, id and bitrate
		logging("Codec %s ID %d bit_rate %lld", pLocalCodec->name, pLocalCodec->id,
			pLocalCodecParameters->bit_rate);
	}

	if (video_stream_index == -1)
	{
		logging("File %s does not contain a video stream!", inputName);
		return -1;
	}

	// https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
	AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
	if (!pCodecContext)
	{
		logging("failed to allocated memory for AVCodecContext");
		return -1;
	}

	// Fill the codec context based on the values from the supplied codec parameters
	// https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
	if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0)
	{
		logging("failed to copy codec params to codec context");
		return -1;
	}

	// Initialize the AVCodecContext to use the given AVCodec.
	// https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
	if (avcodec_open2(pCodecContext, pCodec, NULL) < 0)
	{
		logging("failed to open codec through avcodec_open2");
		return -1;
	}

	// https://ffmpeg.org/doxygen/trunk/structAVFrame.html
	AVFrame *pFrame = av_frame_alloc();
	if (!pFrame)
	{
		logging("failed to allocate memory for AVFrame");
		return -1;
	}
	// https://ffmpeg.org/doxygen/trunk/structAVPacket.html
	AVPacket *pPacket = av_packet_alloc();
	if (!pPacket)
	{
		logging("failed to allocate memory for AVPacket");
		return -1;
	}

    return 0;

    }