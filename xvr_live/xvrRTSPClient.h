#ifndef XVR_RTSP_CLIENT_H
#define XVR_RTSP_CLIENT_H

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include "xvr_buffer.hpp"

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP True

#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

class xvrStreamClientState {
public:
  xvrStreamClientState();
  virtual ~xvrStreamClientState();

public:
  MediaSubsessionIterator* iter;
  MediaSession* session;
  MediaSubsession* subsession;
  TaskToken streamTimerTask;
  double duration;

  //ServerMediaSession *speek_session_;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "xvrStreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "xvrStreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "xvrStreamClientState" field to the subclass:

class xvrRTSPClient: public RTSPClient {
public:
  static xvrRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
				  int verbosityLevel = 0,
				  char const* applicationName = NULL,
				  portNumBits tunnelOverHTTPPortNum = 0);

protected:
  xvrRTSPClient(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
    // called only by createNew();
  virtual ~xvrRTSPClient();

public:
  xvrStreamClientState scs;
  TaskToken checkStatusTimerTask;
  bool stream_recved_flag_;
  boost::shared_ptr<std::string> sdp_;
};


// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.

class xvrDummySink: public MediaSink {
public:
  static xvrDummySink* createNew(UsageEnvironment& env,
			      MediaSubsession& subsession, // identifies the kind of data that's being received
			      char const* streamId = NULL); // identifies the stream itself (optional)

private:
  xvrDummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
    // called only by "createNew()"
  virtual ~xvrDummySink();

  static void afterGettingFrame(void* clientData, unsigned frameSize,
                                unsigned numTruncatedBytes,
				struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
			 struct timeval presentationTime, unsigned durationInMicroseconds);

private:
  // redefined virtual functions:
  virtual Boolean continuePlaying();

public:
  //SPropRecord* video_extra_;
  std::vector<boost::shared_ptr<SPropRecord> > video_extra_;

private:
  //u_int8_t* fReceiveBuffer;
  boost::shared_ptr<sg_xvr::Buffer> recv_buffer_;
  MediaSubsession& fSubsession;
  char* fStreamId;
  void *streamshm_handle;
  uint16_t vframe_no_;
  uint16_t aframe_no_;
  uint8_t stream_valid;
#ifdef VIDEO_FILE_TEST //test
  //FILE* file;
  int fd;
#endif
};



void xvrRTSPClientOpenURL(UsageEnvironment& env, char const* progName, char const* rtspURL);
// Other event handler functions:
void xvrSubsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void xvrSubsessionByeHandler(void* clientData, char const* reason);

UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient);
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession);

unsigned addRtspClientCont();
unsigned delRtspClientCont();

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownXvrStream(RTSPClient* rtspClient, int exitCode = 1);

void xvrRTSPConnectStatusTimer(UsageEnvironment& env, xvrRTSPClient* rtspClient);

void xvrRTSPClientTaskStart();

#endif

