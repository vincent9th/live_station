#include "uv.h"
#include "Base64.hh"
#include "frameshm_manager.h"
#include "xvrRTSPClient.h"

static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.
static char xvrRTSPClientventLoop = 0;

// RTSP 'response handlers':
static void continueAfterXvrDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
static void continueAfterXvrSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
static void continueAfterXvrPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

 // called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);
  // called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")
void StatusTimerHandler(void* clientData);

// Used to iterate through each stream's 'subsessions', setting up each one:
static void setupNextXvrSubsession(RTSPClient* rtspClient);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
  return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
  return env << subsession.mediumName() << "/" << subsession.codecName();
}

unsigned addRtspClientCont()
{
   return ++rtspClientCount;
}

unsigned delRtspClientCont()
{
   return --rtspClientCount;
}

void xvrRTSPConnectStatusTimer(UsageEnvironment& env, xvrRTSPClient* rtspClient)
{
  unsigned uSecsToDelay = (unsigned)10*1000000;
  rtspClient->stream_recved_flag_ = 1;
  rtspClient->checkStatusTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)StatusTimerHandler, rtspClient);
}

void xvrRTSPClientOpenURL(UsageEnvironment& env, char const* progName, char const* rtspURL) {
  // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
  // to receive (even if more than stream uses the same "rtsp://" URL).
  xvrRTSPClient* rtspClient = xvrRTSPClient::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
  if (rtspClient == NULL) {
    env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
    return;
  }

  addRtspClientCont();
  
  // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
  // Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
  // Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
  rtspClient->sendDescribeCommand(continueAfterXvrDESCRIBE); 
}


// Implementation of the RTSP 'response handlers':

void continueAfterXvrDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    xvrStreamClientState& scs = ((xvrRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
      delete[] resultString;
      break;
    }

    char* const sdpDescription = resultString;
    env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

    // Create a media session object from this SDP description:
    scs.session = MediaSession::createNew(env, sdpDescription);
    delete[] sdpDescription; // because we don't need it anymore
    if (scs.session == NULL) {
      env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    } else if (!scs.session->hasSubsessions()) {
      env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }

    // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
    // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
    // (Each 'subsession' will have its own data source.)
    scs.iter = new MediaSubsessionIterator(*scs.session);
    setupNextXvrSubsession(rtspClient);
    return;
  } while (0);

  // An unrecoverable error occurred with this stream.
  shutdownXvrStream(rtspClient);
}

void setupNextXvrSubsession(RTSPClient* rtspClient) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  xvrStreamClientState& scs = ((xvrRTSPClient*)rtspClient)->scs; // alias
  
  scs.subsession = scs.iter->next();
  if (scs.subsession != NULL) {
    if (!scs.subsession->initiate()) {
      env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
      setupNextXvrSubsession(rtspClient); // give up on this subsession; go to the next one
    } else {
      env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
      if (scs.subsession->rtcpIsMuxed()) {
	env << "client port " << scs.subsession->clientPortNum();
      } else {
	env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
      }
      env << ")\n";

      // Continue setting up this subsession, by sending a RTSP "SETUP" command:
      rtspClient->sendSetupCommand(*scs.subsession, continueAfterXvrSETUP, False, REQUEST_STREAMING_OVER_TCP);
    }
    return;
  }

  // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
  if (scs.session->absStartTime() != NULL) {
    // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
    rtspClient->sendPlayCommand(*scs.session, continueAfterXvrPLAY, scs.session->absStartTime(), scs.session->absEndTime());
  } else {
    scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
    rtspClient->sendPlayCommand(*scs.session, continueAfterXvrPLAY);
  }
}

unsigned parseSPropParameterSets(std::vector<boost::shared_ptr<SPropRecord> > & video_extra, char const* sPropParameterSetsStr) {
  unsigned numSPropRecords = 0;
  
  // Make a copy of the input string, so we can replace the commas with '\0's:
  char* inStr = strDup(sPropParameterSetsStr);
  if (inStr == NULL) {
    numSPropRecords = 0;
    return numSPropRecords;
  }

  // Count the number of commas (and thus the number of parameter sets):
  numSPropRecords = 1;
  char* s;
  for (s = inStr; *s != '\0'; ++s) {
    if (*s == ',') {
      ++numSPropRecords;
      *s = '\0';
    }
  }

  s = inStr;
  for (unsigned i = 0; i < numSPropRecords; ++i) {
    boost::shared_ptr<SPropRecord> prop_param(new SPropRecord);
    prop_param->sPropBytes = base64Decode(s, prop_param->sPropLength);
    video_extra.push_back(prop_param);
    s += strlen(s) + 1;
  }

  delete[] inStr;
  return numSPropRecords;
}

static void continueAfterXvrSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    xvrStreamClientState& scs = ((xvrRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
      break;
    }

    env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
    if (scs.subsession->rtcpIsMuxed()) {
      env << "client port " << scs.subsession->clientPortNum();
    } else {
      env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
    }
    env << ")\n";

    // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
    // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
    // after we've sent a RTSP "PLAY" command.)

    xvrDummySink *sink = xvrDummySink::createNew(env, *scs.subsession, rtspClient->url());
    scs.subsession->sink = sink;
    
      // perhaps use your own custom "MediaSink" subclass instead
    if (scs.subsession->sink == NULL) {
      env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
	  << "\" subsession: " << env.getResultMsg() << "\n";
      break;
    }

    env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
    scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession 
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
				       xvrSubsessionAfterPlaying, scs.subsession);
    // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
    if (scs.subsession->rtcpInstance() != NULL) {
      scs.subsession->rtcpInstance()->setByeWithReasonHandler(xvrSubsessionByeHandler, scs.subsession);
    }
  } while (0);
  delete[] resultString;

  // Set up the next subsession, if any:
  setupNextXvrSubsession(rtspClient);
}

void continueAfterXvrPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
  Boolean success = False;

  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    xvrStreamClientState& scs = ((xvrRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
      break;
    }

    // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
    // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
    // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
    // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
    if (scs.duration > 0) {
      unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
      scs.duration += delaySlop;
      unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
      scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
    }

    env << *rtspClient << "Started playing session";
    if (scs.duration > 0) {
      env << " (for up to " << scs.duration << " seconds)";
    }
    env << "...\n";

    success = True;
  } while (0);
  delete[] resultString;

  if (!success) {
    // An unrecoverable error occurred with this stream.
    shutdownXvrStream(rtspClient);
  }
}


// Implementation of the other event handlers:

void xvrSubsessionAfterPlaying(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

  // Begin by closing this subsession's stream:
  Medium::close(subsession->sink);
  subsession->sink = NULL;

  // Next, check whether *all* subsessions' streams have now been closed:
  MediaSession& session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL) {
    if (subsession->sink != NULL) return; // this subsession is still active
  }

  // All subsessions' streams have now been closed, so shutdown the client:
  shutdownXvrStream(rtspClient);
}

void xvrSubsessionByeHandler(void* clientData, char const* reason) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
  UsageEnvironment& env = rtspClient->envir(); // alias

  env << *rtspClient << "Received RTCP \"BYE\"";
  if (reason != NULL) {
    env << " (reason:\"" << reason << "\")";
    delete[] reason;
  }
  env << " on \"" << *subsession << "\" subsession\n";

  // Now act as if the subsession had closed:
  xvrSubsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
  xvrRTSPClient* rtspClient = static_cast<xvrRTSPClient*>(clientData);
  xvrStreamClientState& scs = rtspClient->scs; // alias
  scs.streamTimerTask = NULL;
  shutdownXvrStream(rtspClient);
}

void StatusTimerHandler(void* clientData) {
  xvrRTSPClient* rtspClient = static_cast<xvrRTSPClient*>(clientData);
  UsageEnvironment& env = rtspClient->envir(); // alias

  if(rtspClient->stream_recved_flag_)
  {
    rtspClient->stream_recved_flag_ = 0;
    
    unsigned uSecsToDelay = (unsigned)5*1000000;
    rtspClient->checkStatusTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
  }
  else
  {
    env << "stream revced timeout and shutdown\n";
    shutdownXvrStream(rtspClient);
  }
}

void shutdownXvrStream(RTSPClient* rtspClient, int exitCode) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  xvrStreamClientState& scs = ((xvrRTSPClient*)rtspClient)->scs; // alias

  // First, check whether any subsessions have still to be closed:
  if (scs.session != NULL) { 
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.session);
    MediaSubsession* subsession;

    while ((subsession = iter.next()) != NULL) {
      if (subsession->sink != NULL) {
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	if (subsession->rtcpInstance() != NULL) {
	  subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
	}

	someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive) {
      // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
      // Don't bother handling the response to the "TEARDOWN".
      rtspClient->sendTeardownCommand(*scs.session, NULL);
    }
  }

  env << *rtspClient << "Closing the stream.\n";
  Medium::close(rtspClient);
    // Note that this will also cause this stream's "xvrStreamClientState" structure to get reclaimed.

  if (delRtspClientCont() == 0) {
    // The final stream has ended, so exit the application now.
    // (Of course, if you're embedding this code into your own application, you might want to comment this out,
    // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
    exit(exitCode);
  }
}


// Implementation of "xvrRTSPClient":

xvrRTSPClient* xvrRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
					int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
  return new xvrRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

xvrRTSPClient::xvrRTSPClient(UsageEnvironment& env, char const* rtspURL,
			     int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {
}

xvrRTSPClient::~xvrRTSPClient() {
}


// Implementation of "xvrStreamClientState":

xvrStreamClientState::xvrStreamClientState()
  : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

xvrStreamClientState::~xvrStreamClientState() {
  delete iter;
  if (session != NULL) {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    UsageEnvironment& env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }
}


// Implementation of "xvrDummySink":

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE (3*1024*1024/8)

xvrDummySink* xvrDummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId) {
  return new xvrDummySink(env, subsession, streamId);
}

xvrDummySink::xvrDummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
  : MediaSink(env)
    , fSubsession(subsession)
    , stream_valid(0)
    , vframe_no_(0)
    , aframe_no_(0) {
  fStreamId = strDup(streamId);
  //fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
  recv_buffer_.reset(new sg_xvr::Buffer(DUMMY_SINK_RECEIVE_BUFFER_SIZE, 1024));
    envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << "\n";
    if(!::strcasecmp("video", fSubsession.mediumName())){
        if(!::strcasecmp("H265", fSubsession.codecName())){
#ifdef VIDEO_FILE_TEST
            fd = ::open("stream.265", O_CREAT | O_RDWR, 666);
#endif
        	  parseSPropParameterSets(video_extra_, fSubsession.fmtp_spropvps());
        	  parseSPropParameterSets(video_extra_, fSubsession.fmtp_spropsps());
        	  parseSPropParameterSets(video_extra_, fSubsession.fmtp_sproppps());
        }else{
#ifdef VIDEO_FILE_TEST
            fd = ::open("stream.H264", O_CREAT | O_RDWR, 666);
#endif
            unsigned numSPropRecords = parseSPropParameterSets(video_extra_, fSubsession.fmtp_spropparametersets());
        }
    }
    
#ifdef STREAM_SHM_ENABLE
    //if(!streamshm_handle)
    {
      envir() << "create_frameshm_handle\n";
      streamshm_handle = create_frameshm_handle(STREAM_SHM_SERV_NAME, "av0_0", DUMMY_SINK_RECEIVE_BUFFER_SIZE*3, WRITE_COVER_MODE);
    }
#endif
}

xvrDummySink::~xvrDummySink() {
  //delete[] fReceiveBuffer;
  delete[] fStreamId;
#ifdef STREAM_SHM_ENABLE
  if(streamshm_handle) release_frameshm_handle(streamshm_handle);
#endif
#ifdef VIDEO_FILE_TEST
  if(fd > 0)
    ::close(fd);
#endif
}

void xvrDummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds) {
  xvrDummySink* sink = (xvrDummySink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1

void xvrDummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
    struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
  // We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
  if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
  char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
#ifdef DEBUG_PRINT_NPT
  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
  envir() << "\n";
#endif

  if(!frameSize)
  {
    envir() << "frameSize is 0\n";
    return;
  }

  recv_buffer_->hasWritten(frameSize);

  STREAM_FRAME_INFO_t frame_info;
  STREAM_FRAME_DATA_t frame_data;
  STREAM_FRAME_PACK_t pkt[3];
  uint8_t is_key = 0;
  int r;
  
  ::memset(&frame_info, 0, sizeof(STREAM_FRAME_INFO_t));
  ::memset(&frame_data, 0, sizeof(STREAM_FRAME_DATA_t));
  ::memset(pkt, 0, sizeof(pkt));

  if(!::strcasecmp("video", fSubsession.mediumName()))
  {
    unsigned char const start_code[4] = {0x00, 0x00, 0x00, 0x01};
    const char nal_type = *recv_buffer_->peek() & 0x1f;

    envir() << "nal_type: " << nal_type << " frameSize: " << frameSize << "\n";
    recv_buffer_->prepend(start_code, sizeof(start_code));

#if 1
    if(!::strcasecmp("H265", fSubsession.codecName()) && nal_type > 19
      || !::strcasecmp("H264", fSubsession.codecName()) && nal_type >= 4)
    {
      const char *nal_hdr = recv_buffer_->peek();
      for(int i = 0; i < 5; i++)
      {
        ::fprintf(stderr, "%02x ", nal_hdr[i]);
      }
      ::fprintf(stderr, "\n");

      if(!::strcasecmp("H265", fSubsession.codecName()) && (nal_type == 32 || nal_type == 33 || nal_type == 34)
        || !::strcasecmp("H264", fSubsession.codecName()) && (nal_type == 7 || nal_type == 8))
      {
        envir() << "nal_type: " << nal_type << " skip and continuePlaying" << "\n";
        recv_buffer_->retrieveAll();
        continuePlaying();
        return;
      }

      is_key = 1;
      
      for (std::vector<boost::shared_ptr<SPropRecord> >::iterator iter = video_extra_.begin();
      iter != video_extra_.end(); iter++) {
        boost::shared_ptr<SPropRecord> tmp = *iter;

        recv_buffer_->prepend(tmp->sPropBytes, tmp->sPropLength);
        recv_buffer_->prepend(start_code, sizeof(start_code));

        envir() << "sPropLength:" << tmp->sPropLength << "\n";
      }
    }
#endif

#ifdef VIDEO_FILE_TEST
    envir() << "write len:" << recv_buffer_->readableBytes() << "\n";
    ::write(fd, recv_buffer_->peek(), recv_buffer_->readableBytes());
    //fwrite(recv_buffer_->peek(), recv_buffer_->readableBytes(), 1, file);
#endif

#ifdef STREAM_SHM_ENABLE
    frame_info.vench = 0;
    frame_info.payload = 96;
    frame_info.frame_type = nal_type;
    frame_info.frame_no = vframe_no_++;
    frame_info.frame_size = recv_buffer_->readableBytes();
    frame_info.key = nal_type > 4 ? 1 : 0;
    frame_info.timestamp = presentationTime.tv_usec + presentationTime.tv_sec*1000*1000;

    frame_data.pkt_num = 1;
    frame_data.pkt = pkt;
    pkt[0].addr = (uint8_t *)recv_buffer_->peek();
    pkt[0].pkt_size = recv_buffer_->readableBytes();
    r = write_frameshm_by_handle(streamshm_handle, &frame_info, &frame_data);
    envir() << "write_frameshm_by_handle key:" << frame_info.key << " r:" << r << "\n";
#endif
  }
  else
  {
  }
  // Then continue, to request the next frame of data:

  xvrRTSPClient* rtspClient = static_cast<xvrRTSPClient*>(fSubsession.miscPtr);
  rtspClient->stream_recved_flag_ = 1;
  recv_buffer_->retrieveAll();

  continuePlaying();
  }

Boolean xvrDummySink::continuePlaying() {
  if (fSource == NULL) return False; // sanity check (should not happen)

  // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
#if 0
  fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);
#endif
  fSource->getNextFrame((unsigned char*)recv_buffer_->peek(), recv_buffer_->writableBytes(),
                        afterGettingFrame, this,
                        onSourceClosure, this);
  return True;
}

void xvrRTSPClientTask(void *n)
{
    // Begin by setting up our usage environment:
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

    xvrRTSPClientOpenURL(*env, "live_station", "rtsp://192.168.3.100");
    
    env->taskScheduler().doEventLoop(&xvrRTSPClientventLoop);
}

void xvrRTSPClientTaskStart()
{
    uv_thread_t thread;
    uv_thread_create(&thread, xvrRTSPClientTask, NULL);
}

