// Example Websocket server. See https://mongoose.ws/tutorials/websocket-server/

#include "mongoose.h"

// Tiger added
#include <libavcodec/avcodec.h>    //ffmpeg
#include <libavformat/avformat.h>  //ffmpeg

#include <stdarg.h>  //cmdline args
#include "logging.h"
#include "streamframes.h"


static int init_fn (struct videoArgs *newArgs) {
logging("init_fn called");

logging("init_fn init: %d", newArgs->initialised);
logging("init_fn 1 pCodecContext is %s", newArgs->pCodecContext);


  // initialise video grabbing if it isnt already
  if (newArgs->initialised < 0) {
    int initialise_av =
        streamframes(newArgs);
    if (initialise_av < 0) {
      logging("Initialising streamframes() falied.");
      newArgs->initialised = -1;
    }
  }  else { logging("***  Already initialised libAV "); };

logging("init_fn 2 init now: %d", newArgs->initialised);
logging("init_fn 3 pCodecContext is %d", newArgs->pCodecContext);


return 1;
}

static void broadcaster (void* incoming) {
logging("broadcaster called");
struct videoArgs *newerArgs = incoming;
logging("BEFORE this should be null or 0: %d", newerArgs->pCodecContext);

// logging("INSIDE BROADCASTER AVStream->time_base before open coded %d/%d",
// 			newerArgs->pFormatContext->streams[0]->time_base.num,
// 			newerArgs->pFormatContext->streams[0]->time_base.den);

if (newerArgs->initialised == 1) {
get_frames(newerArgs);

  logging("---- DECODED IMAGE SIZE: %d", newerArgs->imageDataSize);
  logging("AFTER this should be null or 0: %d", newerArgs->pCodecContext);

  if(newerArgs->imageDataSize > 0) {

    struct mg_mgr *mgr = (struct mg_mgr *) newerArgs->mgr;

    // Traverse over all websocket connections and broadcast binary data to  all connected websocket clients 
    
    for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next) {
      if (c->is_websocket) {
      logging("about  to  send websocket");
      mg_ws_send(c, newerArgs->imageDataBuffer,
      newerArgs->imageDataSize, WEBSOCKET_OP_BINARY);
      }
    }

      av_free(newerArgs->imageDataBuffer);
      newerArgs->imageDataBuffer = NULL; // Set the pointer to NULL to avoid dangling pointer 
      newerArgs->imageDataSize = 0;

  }
} else {
  logging("Not broadcasting as not initialised");
}
}

static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
   if (ev == MG_EV_OPEN) {
    //c->is_hexdumping = 1;
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    if (mg_http_match_uri(hm, "/websocket")) {
      // Upgrade to websocket. From now on, a connection is a full-duplex
      // Websocket connection, which will receive MG_EV_WS_MSG events.
      mg_ws_upgrade(c, hm, NULL);
    }
    else if (mg_http_match_uri(hm, "/api/hello")) {
      int count = 0;
      // Iterate over the timer list and count the timers

      for (struct mg_timer *timer = c->mgr->timers; timer != NULL;
           timer = timer->next) {
        count++;
      };

      mg_http_reply(c, 200, "", "{\"result\": %d}\n", count);

    } else if (mg_http_match_uri(hm, "/api/start")) {
     
      struct videoArgs *incomingArgs = c->fn_data;

      logging("start called and init in arg is: %d", incomingArgs->initialised);

      mg_http_reply(c, 200, "", "{\"init\": %d}\n", init_fn(incomingArgs));
      

    } else if (mg_http_match_uri(hm, "/api/stop")) {
     
      struct videoArgs *incomingArgs = c->fn_data;
      incomingArgs->initialised = -1; //THIS ISNT ENOUGH WE NEED TO RESET A FEW THINGS

      logging("stop called and turned init to: %d", incomingArgs->initialised);

      mg_http_reply(c, 200, "", "{\"init\": %d}\n", incomingArgs->initialised);
      

    } else if (mg_http_match_uri(hm, "/")) {

      // Create a buffer to store the extra headers,
      char extra_headers[256];
      // Copy the initial headers to the buffer (HTTP/1.1 200 OK and Content
      // Length auto included)
      strcpy(extra_headers,
             "Content-Type: text/html\r\nSecond Header: second\r\n");
      // Concatenate the value of hm->uri to the buffer
      strcat(extra_headers, "Url Path: ");
      strncat(extra_headers, hm->uri.ptr, hm->uri.len);
      strcat(extra_headers, "\r\n");

      FILE *file;
      char *url_to_html_file = "./public/index.html";
      char *buffer;
      long file_size;

      // Check if the file exists
      if (access(url_to_html_file, F_OK) == -1) {
        mg_http_reply(c, 200, extra_headers, "%s", "File does not exist.\n");
      } else {
        // Open the HTML file in read mode
        file = fopen("./public/index.html", "r");
        if (file == NULL) {
          mg_http_reply(c, 200, extra_headers, "%s",
                        "Failed to open the file.\n");
        } else {
          // Get the file size
          fseek(file, 0, SEEK_END);
          file_size = ftell(file);
          rewind(file);

          // Allocate memory for the buffer
          buffer = (char *) malloc(file_size + 1);
          if (buffer == NULL) {
            mg_http_reply(c, 200, extra_headers, "%s",
                          "Failed to allocate memory.\n");
            fclose(file);

          } else {
            // Read the file contents into the buffer
            if (fread(buffer, file_size, 1, file) != 1) {
              mg_http_reply(c, 200, extra_headers, "%s",
                            "Failed to read the file.\n");

              fclose(file);
              free(buffer);

            } else {
              // Null-terminate the buffer
              buffer[file_size] = '\0';
              mg_http_reply(c, 200, extra_headers, "%s", buffer);
            }
          }
        }
      }

      // Cleanup
      fclose(file);
      free(buffer);
    } else {
     c->is_closing = 1; //imediatley close  any http connections that dont match a path
    }
  } else if (ev == MG_EV_WS_MSG) {
    // Got websocket frame. Received data is wm->data. Echo it back!
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    logging("Received webso ket msg: %s", wm->data.ptr);
    mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
  }
  (void) fn_data;
}


int main(int argc, const char *argv[]) {
  const char *s_listen_on = "ws://localhost:8080";

  if (argc < 2) {
    logging("You need to specify a media file.\n");
    return -1;
  }
  logging("Your argument: %s", argv[1]);

  struct mg_mgr mgr;
  mg_mgr_init(&mgr);  // Initialise event manager
  logging("Starting WS listener on %s/websocket\n", s_listen_on);
  mg_log_set(MG_LL_DEBUG);  // Set log level
  struct videoArgs args = {
      &mgr,
      "rtsp://192.168.1.43:554/user=admin_password=21nv6srw_channel=1_stream=0.sdp?real_stream",
      -1,
      NULL,
      -1,
      NULL,
      NULL,
      0};
  mg_timer_add(&mgr, 7000, MG_TIMER_REPEAT, broadcaster, &args);  // add a timer to broadcast latest frame to all ws clients if already init

  mg_http_listen(&mgr, s_listen_on, fn, &args);  // Create HTTP listener
  for (;;) mg_mgr_poll(&mgr, 500);               // Infinite event loop
  mg_mgr_free(&mgr);
  return 0;
}
