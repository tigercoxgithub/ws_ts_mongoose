#include "streamframes.h" // Include the header file that contains the function declaration

#include "logging.h"
#include <libavcodec/avcodec.h> //ffmpeg
#include <libavformat/avformat.h> //ffmpeg
#include <libavutil/pixdesc.h> //ffmpeg pixel format


int streamframes(struct videoArgs *ARGS)
{
	
	//const char* inputName, int* initialised, AVFormatContext **pFormatContext, int *video_stream_index, AVCodecContext **pCodecContext

 logging("Streamframes called: %d , %s , %d", ARGS->initialised, ARGS->inputName, ARGS->video_stream_index);
   
if(ARGS->initialised < 0) {
        
    logging("Initializing all the containers, codecs and protocols.");

	// AVFormatContext holds the header information from the format (Container)
	// Allocating memory for this component
	// http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
	ARGS->pFormatContext = avformat_alloc_context();
	if (!ARGS->pFormatContext)
	{
		logging("ERROR could not allocate memory for Format Context");
		return -1;
	}

	logging("opening the input file (%s) and loading format (container) header",
		ARGS->inputName);
	// Open the file and read its header. The codecs are not opened.
	// The function arguments are:
	// AVFormatContext (the component we allocated memory for),
	// url (filename),
	// AVInputFormat (if you pass NULL it'll do the auto detect)
	// and AVDictionary (which are options to the demuxer)
	// http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
	if (avformat_open_input(&ARGS->pFormatContext, ARGS->inputName, NULL, NULL) != 0)
	{
		logging("ERROR could not open the file");
		return -1;
	}

	// now we have access to some information about our file
	// since we read its header we can say what format (container) it's
	// and some other information related to the format itself.
	logging("format %s, duration %lld us, bit_rate %lld",
		ARGS->pFormatContext->iformat->name, ARGS->pFormatContext->duration,
		ARGS->pFormatContext->bit_rate);

	logging("finding stream info from format");
	// read Packets from the Format to get stream information
	// this function populates *pFormatContext->streams
	// (of size equals to *pFormatContext->nb_streams)
	// the arguments are:
	// the AVFormatContext
	// and options contains options for codec corresponding to i-th stream.
	// On return each dictionary will be filled with options that were not found.
	// https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
	if (avformat_find_stream_info(ARGS->pFormatContext, NULL) < 0)
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
	for (unsigned int i = 0; i < ARGS->pFormatContext->nb_streams; i++)
	{
		AVCodecParameters *pLocalCodecParameters = NULL;
		pLocalCodecParameters = ARGS->pFormatContext->streams[i]->codecpar;

		logging("AVStream->time_base before open coded %d/%d",
			ARGS->pFormatContext->streams[i]->time_base.num,
			ARGS->pFormatContext->streams[i]->time_base.den);
		logging("AVStream->r_frame_rate before open coded %d/%d",
			ARGS->pFormatContext->streams[i]->r_frame_rate.num,
			ARGS->pFormatContext->streams[i]->r_frame_rate.den);
		logging(
			"AVStream->start_time %" PRId64, ARGS->pFormatContext->streams[i]->start_time);
		logging("AVStream->duration %" PRId64, ARGS->pFormatContext->streams[i]->duration);

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
			if (ARGS->video_stream_index == -1)
			{
				ARGS->video_stream_index = i;
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

	if (ARGS->video_stream_index == -1)
	{
		logging("File %s does not contain a video stream!", ARGS->inputName);
		return -1;
	}

	    

	// https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
    ARGS->pCodecContext = avcodec_alloc_context3(pCodec);
	if (!ARGS->pCodecContext)
	{
		logging("failed to allocated memory for AVCodecContext");
		return -1;
	}

	// Fill the codec context based on the values from the supplied codec parameters
	// https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
	if (avcodec_parameters_to_context(ARGS->pCodecContext, pCodecParameters) < 0)
	{
		logging("failed to copy codec params to codec context");
		return -1;
	}



// Set the frame rate of stream 0 to 30 frames per second
// AVStream* stream = (*pFormatContext)->streams[*video_stream_index];
// stream->avg_frame_rate.num = 1; // Numerator (frames per second)
// stream->avg_frame_rate.den = 1;  // Denominator (seconds)

// Set the desired max_delay value in microseconds  (this  is the jitter-buffer)
int64_t maxDelay = 0; //3000000 = 3secs
// Set the max_delay value in AVFormatContext
ARGS->pFormatContext->max_delay = maxDelay;



	// Initialize the AVCodecContext to use the given AVCodec.
	// https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
	if (avcodec_open2(ARGS->pCodecContext, pCodec, NULL) < 0)
	{
		logging("failed to open codec through avcodec_open2");
		return -1;
	}

	


   ARGS->initialised = 1;

} else {
	logging("Already initialised: %d ", ARGS->initialised);
	return -1;
}

    return 0;

    }


int get_frames(struct videoArgs *ARGS)
{
//AVFormatContext **pFormatContext, int *video_stream_index, AVCodecContext **pCodecContext, unsigned char** imageDataBuffer, size_t *imageDataSize
	logging("getframes  called. video  index is: %d ", ARGS->video_stream_index);
	
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

if(ARGS->pCodecContext == NULL){
logging("getframes pCodecContext is null");
};
logging("getframes  next2 : %d ", ARGS->video_stream_index);
AVStream* stream = ARGS->pFormatContext->streams[ARGS->video_stream_index]; //pFormatContext causing segmentation fault!!!

logging("getframes  next3 : %d ", ARGS->video_stream_index);
AVRational frame_rate = stream->avg_frame_rate; 
logging("Incoming Frame rate: %d/%d Searching for I frames: \n", frame_rate.num, frame_rate.den);


//int64_t latestTimestamp = 0;
int retry = 0;
int atLeastOneVideo = -1;
int receive_response = 0;
while ( receive_response >= 0 ) {

		printf(".");

		//av_seek_frame(pFormatContext, -1, desiredTimestamp, AVSEEK_FLAG_BACKWARD);
		receive_response = av_read_frame(ARGS->pFormatContext, pPacket);

		if (receive_response == AVERROR(EAGAIN))
		{
			logging("No frames available"); 
			if (atLeastOneVideo == 0) {
			break;
			} else if (retry > 3 )  {
				logging("max retries reached");
				return -1;
			} else {
				retry += 1;
				continue;
			}
		}
		else if (receive_response < 0)
		{
			logging("Error reading frame"); //could reinitialise connection to rtsp stream here

			if (atLeastOneVideo == 0) {
			break;
			} else {
			return -1;
			}
		}


		// if it's the video stream
		if (pPacket->stream_index == ARGS->video_stream_index)
		{

        //logging("Got video AVPacket->pts %" PRId64, pPacket->pts);
		
			if (pPacket->data[0] == 0x00 && pPacket->data[1] == 0x00 && pPacket->data[2] == 0x00 && pPacket->data[3] == 0x01) {
    			// NALU start code prefix (0x000001) is present

			    int nalType = pPacket->data[4] & 0x1F; // Extract the NALU type from the first byte
				if (nalType == 7) {
        		// NALU type 7 indicates an I-frame or IDR frame
				//logging("GOT IFRAME!");
				
		       	atLeastOneVideo = 0;
				break;
				}
			}

		av_packet_unref(pPacket);
		continue;

		} else  {
			 logging("NON VIDEO PACKET RECEIVED");
			 av_packet_unref(pPacket);
			 continue;
		}
}


int decode_response = decode_packet(pPacket, pFrame, ARGS);
			if (decode_response < 0) {
				logging("Error decode_packet");
			    return -1;
			}



	// https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
	av_packet_unref(pPacket);	//can now realocate
	av_frame_unref(pFrame);

	av_packet_free(&pPacket); //can not reuse anymore
	pPacket = NULL;
	av_frame_free(&pFrame); //can not reuse anymore
	pFrame = NULL;
	// avformat_close_input(&pFormatContext); //closes rtsp 
	// avcodec_free_context(&pCodecContext); //can not reuse anymore

	

	return 0;
}

    


int decode_packet(AVPacket *pPacket, AVFrame *pFrame, struct videoArgs *ARGS)
{

//AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame, unsigned char **imageDataBuffer, size_t *imageDataSize
	// Supply raw packet data as input to a decoder
	// https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3
	int sendPacketToDecoder = avcodec_send_packet(ARGS->pCodecContext, pPacket); //--------------------------------IMPORTANT------

	if (sendPacketToDecoder < 0)
	{
		logging(
			"Error while sending a packet to the decoder: %s", av_err2str(sendPacketToDecoder));
		return sendPacketToDecoder;
	}

    int giveMeDecodedFrame = 0;
	while (giveMeDecodedFrame >= 0)
  	{
	
		// Return decoded output data (into a frame) from a decoder
		// https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c

		giveMeDecodedFrame = avcodec_receive_frame(ARGS->pCodecContext, pFrame); //--------------------------------IMPORTANT------
		if (giveMeDecodedFrame == AVERROR(EAGAIN) || giveMeDecodedFrame == AVERROR_EOF) {
      	break;
    	} else if (giveMeDecodedFrame < 0) {
      	logging("Error while receiving a frame from the decoder: %s", av_err2str(giveMeDecodedFrame));
      	return giveMeDecodedFrame;
    	}
		
		
		if (giveMeDecodedFrame >= 0) {

			logging("Decoded Frame %d (type=%c, size=%d bytes, format=%d) pts %d key_frame "
					"%d [DTS %d] ",
				ARGS->pCodecContext->frame_number,
				av_get_picture_type_char(pFrame->pict_type), pFrame->pkt_size,
				pFrame->format, pFrame->pts, pFrame->key_frame,
				pFrame->coded_picture_number);
			
			// const AVPixFmtDescriptor *pixDesc = av_pix_fmt_desc_get(pFrame->format);
    		//  if (!pixDesc) {
      		//   printf("Failed to get pixel format descriptor\n");
   			//  }
            // printf("Pixel format: %s\n", pixDesc->name);
			

			
			//JPEG FILE NAME
			char frame_filename[1024];
			snprintf(frame_filename, sizeof(frame_filename), "%s-%d.jpg", "frame",
			ARGS->pCodecContext->frame_number);

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



   			 int convertandsend = save_frame_as_jpeg(pFrame, frame_filename, ARGS);
			 if (convertandsend < 0) { logging("Error saving jpeg"); }
			}
	}
	
	//avcodec_flush_buffers(pCodecContext);  //Added to try to get rid of old data

	//added as experiment
	av_packet_unref(pPacket);	//can now realocate
	av_frame_unref(pFrame);
	


	return 0;
}

int save_frame_as_jpeg(AVFrame *pFrame, char *frame_filename, struct videoArgs *ARGS) 
{

logging("Incoming image size: %d", ARGS->imageDataSize);

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


jpegCodecCtx->bit_rate = 200000; // Adjust the bit rate as needed (was 400000)

// int width = 1280;
// int height = 720;
// pFrame->width = width;
// pFrame->height = height;
// pFrame->format = AV_PIX_FMT_YUV420P;
// jpegCodecCtx->width = 640; //setting absolute value cuts the sides off
// jpegCodecCtx->height = 360;
// jpegCodecCtx->qmin = 100;
// jpegCodecCtx->qmax = 100;
jpegCodecCtx->qcompress = 0.3; // this actually makes a good difference to size without  killing quality

jpegCodecCtx->width = pFrame->width;
jpegCodecCtx->height = pFrame->height;
jpegCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P; // Use the JPEG-specific pixel format
jpegCodecCtx->color_range = AVCOL_RANGE_JPEG; // Set the color range to JPEG

//experiment
// AVRational frameRate;
// frameRate.num = 1;  // Numerator of the frame rate
// frameRate.den = 1;   // Denominator of the frame rate
// jpegCodecCtx->framerate = frameRate;


    // Open the JPEG encoder
    if (avcodec_open2(jpegCodecCtx, jpegCodec, NULL) < 0) {
        fprintf(stderr, "Failed to open JPEG codec\n");
        avcodec_free_context(&jpegCodecCtx);
        return -1;
    }

    // Allocate an AVPacket for the encoded JPEG data
    AVPacket encodedJPEGPacket;
    av_init_packet(&encodedJPEGPacket);
    encodedJPEGPacket.data = NULL;
    encodedJPEGPacket.size = 0;

	avcodec_flush_buffers(jpegCodecCtx);  //Added to try to get rid of old data

    // Encode the frame
    avcodec_send_frame(jpegCodecCtx, pFrame);  
    avcodec_receive_packet(jpegCodecCtx, &encodedJPEGPacket);


// Allocate memory for the buffer based on packet size
ARGS->imageDataBuffer = (unsigned char*) av_malloc(encodedJPEGPacket.size);
if (ARGS->imageDataBuffer == NULL) {
	logging("Could not allocate memory for the buffer based on encodedJPEGPacket size");
   return -1;
}

// Copy encodedJPEGPacket data to the buffer
memcpy(ARGS->imageDataBuffer, encodedJPEGPacket.data, encodedJPEGPacket.size);

// Update the image data size
ARGS->imageDataSize = encodedJPEGPacket.size;


// Clean up
	
	av_frame_unref(pFrame);

    av_packet_unref(&encodedJPEGPacket);
    //av_packet_free(&encodedJPEGPacket);
	
    avcodec_free_context(&jpegCodecCtx);

    printf("\nFRAME NUMBER: '%s'\n", frame_filename);
	return 0;
}