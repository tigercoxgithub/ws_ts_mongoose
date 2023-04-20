// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved
//
// Example Websocket server. See https://mongoose.ws/tutorials/websocket-server/

#include "mongoose.h"

//Tiger added
#include <libavcodec/avcodec.h> //ffmpeg
#include <libavformat/avformat.h> //ffmpeg

#include <stdarg.h>  //cmdline args
#include "logging.h" 
#include "streamframes.h"

// Define a custom data structure to hold extra arguments
struct TimerArgs {
    struct mg_mgr * mgr;
    char *initialised;
    AVFormatContext *pFormatContext;
    AVPacket *pPacket;
    int video_stream_index;
    AVCodecContext *pCodecContext;
    AVFrame *pFrame; 
    
};

struct imageData {
  unsigned char* imageDataBuffer; // Buffer to hold image data
  size_t imageDataSize;           // Size of the image data
};


static const char *s_listen_on = "ws://localhost:8080";
static const char *s_web_root = ".";

// This RESTful server implements the following endpoints:
//   /websocket - upgrade to Websocket, and implement websocket echo server
//   /rest - respond with JSON string {"result": 123}
//   any other URI serves static files from s_web_root 
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_OPEN) {
    // c->is_hexdumping = 1;
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    if (mg_http_match_uri(hm, "/websocket")) {
      // Upgrade to websocket. From now on, a connection is a full-duplex
      // Websocket connection, which will receive MG_EV_WS_MSG events.
      mg_ws_upgrade(c, hm, NULL);
    } else if (mg_http_match_uri(hm, "/rest")) {
      // Serve REST response
      mg_http_reply(c, 200, "", "{\"result\": %d}\n", 123);
    } else {
      // Serve static files
      struct mg_http_serve_opts opts = {.root_dir = s_web_root};
      mg_http_serve_dir(c, ev_data, &opts);
    }
  } else if (ev == MG_EV_WS_MSG) {
    // Got websocket frame. Received data is wm->data. Echo it back!
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
   
  }
  (void) fn_data;
}



void timer_fn(void *args) {
logging("timer_fn called");

struct TimerArgs *timerArgs = args;
struct imageData imageDataArgs;
imageDataArgs.imageDataBuffer = NULL; // Initialize the buffer to NULL
imageDataArgs.imageDataSize = 0;     // Initialize the size to 0

get_frames(timerArgs->pFormatContext, timerArgs->pPacket, timerArgs->video_stream_index, timerArgs->pCodecContext, timerArgs->pFrame, &imageDataArgs.imageDataBuffer, &imageDataArgs.imageDataSize);

logging("Image size: %d", imageDataArgs.imageDataSize);

// logging("Printing bytes to console");
//     for (unsigned long i = 0; i < imageDataArgs.imageDataSize; i++) {
//         printf("%02X ", (unsigned int)&imageDataArgs.imageDataBuffer[i]);
//     }
//     printf("\n");

//convert type for the sake of mg_ws_send
//const void *constVoidData = (const void *)imageDataArgs.imageDataSize;


  struct mg_mgr *mgr = (struct mg_mgr *) timerArgs->mgr;
  // Traverse over all connections
  for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next) {
    // Broadcast to everybody
    //mg_ws_send(c, "hi", 2, WEBSOCKET_OP_TEXT);
    
    // Send the buffer as binary data
    mg_ws_send(c, imageDataArgs.imageDataBuffer, imageDataArgs.imageDataSize, WEBSOCKET_OP_BINARY);
  }

// Free the memory allocated for imageDataBuffer
if (imageDataArgs.imageDataBuffer != NULL) {
    free(imageDataArgs.imageDataBuffer);
    imageDataArgs.imageDataBuffer = NULL; // Set the pointer to NULL to avoid dangling pointer
}


}

int main(int argc, const char *argv[]) {

  //tiger added
  if (argc < 2)
	{
		logging("You need to specify a media file.\n");
		return -1;
	}
  logging("Your argument: %s", argv[1]);


  // Create a custom data structure to hold both streamframe timer_fn arguments
    struct TimerArgs args;
    args.initialised = "N";
    args.pFormatContext = NULL;
    args.pPacket = NULL;
    args.video_stream_index = -1;
    args.pCodecContext = NULL;
    args.pFrame = NULL;

  //initialise video grabbing
  int initialise_av = streamframes(argv[1], &args.initialised, &args.pFormatContext, &args.pPacket, &args.video_stream_index, &args.pCodecContext, &args.pFrame);
  if(initialise_av < 0){
    logging("Initialising streamframes() falied.");
    return -1;
  }
  


  struct mg_mgr mgr;  // Event manager
  mg_mgr_init(&mgr);  // Initialise event manager
  logging("Starting WS listener on %s/websocket\n", s_listen_on);
  mg_log_set(MG_LL_DEBUG);  // Set log level

  args.mgr = &mgr;
  mg_timer_add(&mgr, 3000, MG_TIMER_REPEAT, timer_fn, &args);  //add a timer to broadcast latest  frame
  

  mg_http_listen(&mgr, s_listen_on, fn, NULL);  // Create HTTP listener
  for (;;) mg_mgr_poll(&mgr, 500);             // Infinite event loop
  mg_mgr_free(&mgr);
  return 0;
}
