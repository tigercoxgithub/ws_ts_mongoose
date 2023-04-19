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
    struct mg_mgr * arg1;
    char *arg2;
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

struct TimerArgs *tester = args;
const void *constVoidData = (const void *)tester->arg2;
    

  struct mg_mgr *mgr = (struct mg_mgr *) tester->arg1;
  // Traverse over all connections
  for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next) {
    // Broadcast to everybody
    mg_ws_send(c, constVoidData, 2, WEBSOCKET_OP_TEXT);
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
    args.arg2 = "T";

  if(streamframes(argv[1], &args.arg2) != 0){
    logging("streamframes(argv[1]) falied.");
    return -1;
  }
  


  struct mg_mgr mgr;  // Event manager
  mg_mgr_init(&mgr);  // Initialise event manager
  logging("Starting WS listener on %s/websocket\n", s_listen_on);
  mg_log_set(MG_LL_DEBUG);  // Set log level

  args.arg1 = &mgr;
  mg_timer_add(&mgr, 2000, MG_TIMER_REPEAT, timer_fn, &args);  //add a timer to broadcast latest  frame
  

  mg_http_listen(&mgr, s_listen_on, fn, NULL);  // Create HTTP listener
  for (;;) mg_mgr_poll(&mgr, 500);             // Infinite event loop
  mg_mgr_free(&mgr);
  return 0;
}
