#include "streamframes.h" // Include the header file that contains the function declaration

#include "logging.h"
#include <libavcodec/avcodec.h> //ffmpeg
#include <libavformat/avformat.h> //ffmpeg
#include <libavutil/pixdesc.h> //ffmpeg pixel format


int streamframes(const char* inputName, char** initialised, AVFormatContext **pFormatContext, AVPacket **pPacket, int *video_stream_index, AVCodecContext **pCodecContext, AVFrame **pFrame )
{

   
if(strcmp(*initialised, "N") == 0) {

    *initialised = "Y";
        
    logging("Initializing all the containers, codecs and protocols.");

	// AVFormatContext holds the header information from the format (Container)
	// Allocating memory for this component
	// http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
	*pFormatContext = avformat_alloc_context();
	if (!*pFormatContext)
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
	if (avformat_open_input(pFormatContext, inputName, NULL, NULL) != 0)
	{
		logging("ERROR could not open the file");
		return -1;
	}

	// now we have access to some information about our file
	// since we read its header we can say what format (container) it's
	// and some other information related to the format itself.
	logging("format %s, duration %lld us, bit_rate %lld",
		(*pFormatContext)->iformat->name, (*pFormatContext)->duration,
		(*pFormatContext)->bit_rate);

	logging("finding stream info from format");
	// read Packets from the Format to get stream information
	// this function populates pFormatContext->streams
	// (of size equals to pFormatContext->nb_streams)
	// the arguments are:
	// the AVFormatContext
	// and options contains options for codec corresponding to i-th stream.
	// On return each dictionary will be filled with options that were not found.
	// https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
	if (avformat_find_stream_info(*pFormatContext, NULL) < 0)
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
	//int video_stream_index = -1;

	// loop though all the streams and print its main information
	for (unsigned int i = 0; i < (*pFormatContext)->nb_streams; i++)
	{
		AVCodecParameters *pLocalCodecParameters = NULL;
		pLocalCodecParameters = (*pFormatContext)->streams[i]->codecpar;
		logging("AVStream->time_base before open coded %d/%d",
			(*pFormatContext)->streams[i]->time_base.num,
			(*pFormatContext)->streams[i]->time_base.den);
		logging("AVStream->r_frame_rate before open coded %d/%d",
			(*pFormatContext)->streams[i]->r_frame_rate.num,
			(*pFormatContext)->streams[i]->r_frame_rate.den);
		logging(
			"AVStream->start_time %" PRId64, (*pFormatContext)->streams[i]->start_time);
		logging("AVStream->duration %" PRId64, (*pFormatContext)->streams[i]->duration);

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
			if (*video_stream_index == -1)
			{
				*video_stream_index = i;
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

	if (*video_stream_index == -1)
	{
		logging("File %s does not contain a video stream!", inputName);
		return -1;
	}

	// https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
	*pCodecContext = avcodec_alloc_context3(pCodec);
	if (!*pCodecContext)
	{
		logging("failed to allocated memory for AVCodecContext");
		return -1;
	}

	// Fill the codec context based on the values from the supplied codec parameters
	// https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
	if (avcodec_parameters_to_context(*pCodecContext, pCodecParameters) < 0)
	{
		logging("failed to copy codec params to codec context");
		return -1;
	}

	// Initialize the AVCodecContext to use the given AVCodec.
	// https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
	if (avcodec_open2(*pCodecContext, pCodec, NULL) < 0)
	{
		logging("failed to open codec through avcodec_open2");
		return -1;
	}

	// https://ffmpeg.org/doxygen/trunk/structAVFrame.html
	*pFrame = av_frame_alloc();
	if (!*pFrame)
	{
		logging("failed to allocate memory for AVFrame");
		return -1;
	}
	// https://ffmpeg.org/doxygen/trunk/structAVPacket.html
	*pPacket = av_packet_alloc();
	if (!*pPacket)
	{
		logging("failed to allocate memory for AVPacket");
		return -1;
	}


    //get_frames(pFormatContext, pPacket, *video_stream_index, pCodecContext, pFrame);
} else {
	logging("Already initialised: %s ", *initialised);
	return -1;
}


    return 0;

    }


int get_frames(AVFormatContext *pFormatContext, AVPacket *pPacket, int video_stream_index, AVCodecContext *pCodecContext, AVFrame *pFrame, unsigned char** imageDataBuffer, size_t *imageDataSize)
{
	
    // logging("Image size now: %d", *imageDataSize);
    // *imageDataSize = pCodecContext->frame_number;

	// fill the Packet with data from the Stream
	// https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61
	
		// Receive the latest frame
		int receive_response = av_read_frame(pFormatContext, pPacket);

		if (receive_response == AVERROR(EAGAIN))
		{
			logging("No frame available, continue reading next packet");
			return -1;
		}
		else if (receive_response < 0)
		{
			logging("Error reading frame");
			return -1;
		}


		// if it's the video stream
		if (pPacket->stream_index == video_stream_index)
		{
            logging("AVPacket->pts %" PRId64, pPacket->pts);

			int decode_response = decode_packet(pPacket, pCodecContext, pFrame, imageDataBuffer, imageDataSize);
			if (decode_response < 0) {
				logging("Error decode_packet");
			    return -1;
			}
		}

	// https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
	av_packet_unref(pPacket);	//can now realocate

	// avformat_close_input(&pFormatContext); //closes rtsp
	// av_packet_free(&pPacket); //can not reuse anymore
	// av_frame_free(&pFrame); //can not reuse anymore
	// avcodec_free_context(&pCodecContext); //can not reuse anymore

	return 0;
}


int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame, unsigned char** imageDataBuffer, size_t *imageDataSize)
{


	// Supply raw packet data as input to a decoder
	// https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3
	int response = avcodec_send_packet(pCodecContext, pPacket);

	if (response < 0)
	{
		logging(
			"Error while sending a packet to the decoder: %s", av_err2str(response));
		return response;
	}

	while (response >= 0)
	{
		// Return decoded output data (into a frame) from a decoder
		// https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
		response = avcodec_receive_frame(pCodecContext, pFrame);
		if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
		{
			break;
		}
		else if (response < 0)
		{
			logging("Error while receiving a frame from the decoder: %s",
				av_err2str(response));
			return response;
		}

		if (response >= 0)
		{
			logging("Decoded Frame %d (type=%c, size=%d bytes, format=%d) pts %d key_frame "
					"%d [DTS %d]",
				pCodecContext->frame_number,
				av_get_picture_type_char(pFrame->pict_type), pFrame->pkt_size,
				pFrame->format, pFrame->pts, pFrame->key_frame,
				pFrame->coded_picture_number);
			
			const AVPixFmtDescriptor *pixDesc = av_pix_fmt_desc_get(pFrame->format);
    		 if (!pixDesc) {
      		  printf("Failed to get pixel format descriptor\n");

   			 }
			
            printf("Pixel format: %s\n", pixDesc->name);
			//printf("Colorspace format: %s\n", pFrame->colorspace);


			
			//JPEG FILE NAME
			char frame_filename[1024];
			snprintf(frame_filename, sizeof(frame_filename), "%s-%d.jpg", "frame",
			pCodecContext->frame_number);

			// Check if the frame is a planar YUV 4:2:0, 12bpp
			// That is the format of the provided .mp4 file
			// RGB formats will definitely not give a gray image
			// Other YUV image may do so, but untested, so give a warning
			if (pFrame->format != AV_PIX_FMT_YUV420P)
			{
				logging(
					"Warning: the generated file may not be a grayscale image, but "
					"could e.g. be just the R component if the video format is RGB");
			}



   			 int convertandsend = save_frame_as_jpeg(pFrame, frame_filename, imageDataBuffer, imageDataSize);
			 if (convertandsend < 0) { logging("Error saving jpeg"); }

		
		}
	}
	return 0;
}

int save_frame_as_jpeg(AVFrame *pFrame, char* frame_filename, unsigned char** imageDataBuffer, size_t *imageDataSize) {

logging("Incoming: %d", *imageDataSize);
//*imageDataSize = pCodecContext->frame_number;

AVCodec *jpegCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (!jpegCodec) {
        fprintf(stderr, "JPEG codec not found\n");
        return -1;
    }

    // Allocate an AVCodecContext for the JPEG encoder
    AVCodecContext *jpegCodecCtx = avcodec_alloc_context3(jpegCodec);
    if (!jpegCodecCtx) {
        fprintf(stderr, "Failed to allocate AVCodecContext for JPEG codec\n");
        return -1;
    }

// Set the codec properties
// Set the time base of the frame to 1/90000 (90 kHz time units)
AVRational time_base;
time_base.num = 1;
time_base.den = 90000;
jpegCodecCtx->time_base = time_base;
// Set the pts (presentation timestamp) value of the frame to 1000 (1 second)
// int64_t pts = 1000;
// jpegCodecCtx->pts = pts;
jpegCodecCtx->width = pFrame->width;
jpegCodecCtx->height = pFrame->height;
jpegCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P; // Use the JPEG-specific pixel format
jpegCodecCtx->color_range = AVCOL_RANGE_JPEG; // Set the color range to JPEG

    // Open the JPEG encoder
    if (avcodec_open2(jpegCodecCtx, jpegCodec, NULL) < 0) {
        fprintf(stderr, "Failed to open JPEG codec\n");
        avcodec_free_context(&jpegCodecCtx);
        return -1;
    }

    // Allocate an AVPacket for the encoded JPEG data
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

    // Encode the frame
    avcodec_send_frame(jpegCodecCtx, pFrame);
    avcodec_receive_packet(jpegCodecCtx, &packet);

    //-------Save the packet data to a file
    // FILE *f = fopen(frame_filename, "wb");
    // if (!f) {
    //     fprintf(stderr, "Failed to open output file '%s'\n", frame_filename);
    //     av_packet_unref(&packet);
    //     avcodec_free_context(&jpegCodecCtx);
    //     return -1;
    // }
    // fwrite(packet.data, 1, packet.size, f);
    // fclose(f);


//----------Save to buffer instead of file
// const char buffer = NULL;
// int buffer_size = packet.size;

//     // Open a temporary file in memory for writing
//     FILE* mem_file = fmemopen((void*)buffer, buffer_size, "w");
//     if (!mem_file) {
//         printf("Failed to open memory file\n");
//         return 1;
//     }

//     fwrite(packet.data, 1, packet.size, mem_file);
//     fclose(mem_file);

//     // Output the updated buffer


// packet.size;
// ws_sendframe_bin(NULL, &buffer, new_size);

// free(&buffer);


// Assume packet is of type AVPacket
// Assume imageDataBuffer is of type unsigned char**
// Assume imageDataSize is of type size_t*

// Allocate memory for the buffer based on packet size
*imageDataBuffer = (unsigned char*) av_malloc(packet.size);
if (*imageDataBuffer == NULL) {
   return -1;
}

// Copy packet data to the buffer
memcpy(*imageDataBuffer, packet.data, packet.size);

// Update the image data size
*imageDataSize = packet.size;





// Clean up
	
    av_packet_unref(&packet);
    avcodec_free_context(&jpegCodecCtx);

    printf("JPEG image saved as '%s'\n", frame_filename);
	return 0;
}