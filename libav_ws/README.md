
To do:

TODO: Iframes are  arriving at 150,000bytes and at  first coming out as 40,000 bytes jpegs, but  over time those jpegs grow  to be 150kb or bigger! Why is this? Is the raspi unable to keep up the compression standards? Or is data acumulating from faulty cleanup?

TODO:  limit the number  of websocket connections  allowed

TODO: make it so that decoding frames (timer_fn called) only happens when at least one  client is connected

TODO:  protect the whole mongoose server with  a Bearer token that only the CF worker will know

TODO: add an api endpoint that re-initialises the connection to the rtsp camera

1. Camera is ws server which sends a new frame every time it is requested by a ws client message (rate limited to 1fps and client limited to 1).
2. Durable Object is a ws client and server that waits for a browser ws client to connect
3. when browser ws client connects the DO starts asking camera for frames at 1fps, storing them to state (every 10th frame to storage in case state gets wiped) and then distributing them to all clients connected to it. 
4. If Durable Object cant  reach camera it will return 'camera disconnected' message while it retries with expo backoff
5. In this way, if no browser clients are connected then camera doesnt do any  work on frames, but if just one is connected and another connects, the durable object serves it from state rather than asking camera to do more work
6. At first all browser clients https ask  Durable Object how to get frames, response is the latest image in state (or storage if no state) with a LIVE_SRC header containing a wss  url or array of SDP Offers:
	- Is camera currently connected via websocket and sending in frames?
		- if yes then 
			- Are there are any webRTC datachannel peers you could get the stream off?
				- yes then an array of SDP Offers (including their proximity to your IP and number of already connected clients) for you to try and connect to
		- else wss url you can connect to as a websocket client (step 3) 
	- else 'camera disconnected' message

- Controller needs to store:
	- how many clients are streaming state images through the websocket server
	- when a client is able to give an SDP Offer (irrespective of websocket or peer webrtc src) and a counter when it connects or disconnects to other peers