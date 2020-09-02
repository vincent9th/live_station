#include "xvrRTSPClient.h"

static void setupNextXvrShmSubsession(RTSPClient* rtspClient);

class xvrSpeekClientState {
public:
  xvrSpeekClientState();
  virtual ~xvrSpeekClientState();

public:
  //TaskToken streamTimerTask;
#if 0
  PassiveServerMediaSubsession *subsession_;
  RTPSink *sink_;
#else
#endif
  ServerMediaSession *servSession_;
  ServerMediaSubsession *Servubsession_;
  MediaSession *mediaSession_;
  MediaSubsession *mediaSubsession_;
};

xvrSpeekClientState::xvrSpeekClientState()
  : mediaSession_(NULL), mediaSubsession_(NULL) {
}

xvrSpeekClientState::~xvrSpeekClientState() {
#if 0
  if (session != NULL) {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    UsageEnvironment& env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }
#endif
}

class xvrRTSPAnnounceClient: public RTSPClient {
public:
  static xvrRTSPAnnounceClient* createNew(UsageEnvironment& env, char const* rtspURL,
				  int verbosityLevel = 0,
				  char const* applicationName = NULL,
				  portNumBits tunnelOverHTTPPortNum = 0);

protected:
  xvrRTSPAnnounceClient(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
    // called only by createNew();
  virtual ~xvrRTSPAnnounceClient();

public:
  xvrSpeekClientState scs;
  TaskToken checkStatusTimerTask;
  bool stream_recved_flag_;
  boost::shared_ptr<std::string> sdp_;
};

xvrRTSPAnnounceClient* xvrRTSPAnnounceClient::createNew(UsageEnvironment& env, char const* rtspURL,
					int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
  return new xvrRTSPAnnounceClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

xvrRTSPAnnounceClient::xvrRTSPAnnounceClient(UsageEnvironment& env, char const* rtspURL,
			     int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {
}

xvrRTSPAnnounceClient::~xvrRTSPAnnounceClient() {
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ShmFrameSink: public MultiFramedRTPSink {
public:
  static ShmFrameSink* createNew(UsageEnvironment& env,
				    Groupsock* RTPgs = NULL,
				    unsigned char rtpPayloadFormat = 19,
				    Boolean sourceIsWideband = False,
				    unsigned numChannelsInSource = 0);

protected:
  ShmFrameSink(UsageEnvironment& env, Groupsock* RTPgs,
		  unsigned char rtpPayloadFormat,
		  Boolean sourceIsWideband, unsigned numChannelsInSource);
	// called only by createNew()

  virtual ~ShmFrameSink();

private:
};

ShmFrameSink*
ShmFrameSink::createNew(UsageEnvironment& env, Groupsock* RTPgs,
			   unsigned char rtpPayloadFormat,
			   Boolean sourceIsWideband,
			   unsigned numChannelsInSource) {
  return new ShmFrameSink(env, RTPgs, rtpPayloadFormat,
			     sourceIsWideband, numChannelsInSource);
}

ShmFrameSink
::ShmFrameSink(UsageEnvironment& env, Groupsock* RTPgs,
		  unsigned char rtpPayloadFormat,
		  Boolean sourceIsWideband, unsigned numChannelsInSource)
  : MultiFramedRTPSink(env, RTPgs, rtpPayloadFormat,
		 sourceIsWideband ? 16000 : 8000, "PCMA", numChannelsInSource) {

}

ShmFrameSink::~ShmFrameSink() {
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ShmFrameSource : public FramedSource {
public:
  static ShmFrameSource* createNew(UsageEnvironment& env,
				       void *frameshm_handle);

private:
  ShmFrameSource(UsageEnvironment& env, void *frameshm_handle);
	// called only by createNew()

  virtual ~ShmFrameSource();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();

private:
  void *frameshm_handle;
};

ShmFrameSource
::ShmFrameSource(UsageEnvironment& env, void *frameshm_handle)
  : FramedSource(env) {
}

ShmFrameSource::~ShmFrameSource() {
}

ShmFrameSource*
 ShmFrameSource::createNew(UsageEnvironment& env, void *frameshm_handle)
{
  return new ShmFrameSource(env, frameshm_handle);
}

void ShmFrameSource::doGetNextFrame() {
  fprintf(stderr, "%s:%s(%d)yyyyyyyyyyyyyyyyyyyy\n", __FILE__, __FUNCTION__, __LINE__);

#if 0
  // Begin by reading the 1-byte frame header (and checking it for validity)
  while (1) {
    if (fread(&fLastFrameHeader, 1, 1, fFid) < 1) {
    handleClosure();
    return;
    }
    if ((fLastFrameHeader&0x83) != 0) {
#ifdef DEBUG
        fprintf(stderr, "Invalid frame header 0x%02x (padding bits (0x83) are not zero)\n", fLastFrameHeader);
#endif
    } else {
      unsigned char ft = (fLastFrameHeader&0x78)>>3;
      fFrameSize = fIsWideband ? frameSizeWideband[ft] : frameSize[ft];
      if (fFrameSize == FT_INVALID) {
#ifdef DEBUG
      fprintf(stderr, "Invalid FT field %d (from frame header 0x%02x)\n",
      ft, fLastFrameHeader);
#endif
      } else {
      // The frame header is OK
#ifdef DEBUG
      fprintf(stderr, "Valid frame header 0x%02x -> ft %d -> frame size %d\n", fLastFrameHeader, ft, fFrameSize);
#endif
      break;
      }
    }
  }

  // Next, read the frame-block into the buffer provided:
  fFrameSize *= fNumChannels; // because multiple channels make up a frame-block
  if (fFrameSize > fMaxSize) {
    fNumTruncatedBytes = fFrameSize - fMaxSize;
    fFrameSize = fMaxSize;
  }
  fFrameSize = fread(fTo, 1, fFrameSize, fFid);

  // Set the 'presentation time':
  if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
    // This is the first frame, so use the current time:
    gettimeofday(&fPresentationTime, NULL);
  } else {
    // Increment by the play time of the previous frame (20 ms)
    unsigned uSeconds	= fPresentationTime.tv_usec + 20000;
    fPresentationTime.tv_sec += uSeconds/1000000;
    fPresentationTime.tv_usec = uSeconds%1000000;
  }
#endif

  const char str[] = "12345678901234567890";
  strcpy((char*)fTo, str);
  fFrameSize = sizeof(str);
  
  fDurationInMicroseconds = 20000; // each frame is 20 ms

  // Switch to another task, and inform the reader that he has data:
  nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
      (TaskFunc*)FramedSource::afterGetting, this);
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void continueAfterXvrShmPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {

}

static void xvrShmAfterPlaying(void* clientData) {
  ShmFrameSink *sink = static_cast<ShmFrameSink *>(clientData);

}

static void continueAfterXvrShmSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    xvrRTSPAnnounceClient *xvrClient = (xvrRTSPAnnounceClient*)rtspClient;
    xvrSpeekClientState& scs = xvrClient->scs; // alias

    env << *rtspClient << " SETUP resultString:" << resultString << "\n";

    if (resultCode != 0) {
    env << *rtspClient << "Failed to set up \n";
    break;
    }

    env << *rtspClient << __LINE__ << " sendPlayCommand\n";
    struct in_addr dummyDestAddress;
    dummyDestAddress.s_addr = 0;
    Groupsock* rtpGroupsock = new Groupsock(env, dummyDestAddress, 0, 0);
    ShmFrameSink *sink = ShmFrameSink::createNew(env, rtpGroupsock, 19);

    ShmFrameSource *source = ShmFrameSource::createNew(env, NULL);

    sink->setPacketSizes(10, 1024);
    sink->setStreamSocket(rtspClient->socketNum(), 0);
    sink->startPlaying(*source, xvrShmAfterPlaying, sink);

    xvrClient->sendPlayCommand(*scs.mediaSession_, continueAfterXvrShmPLAY);
  } while (0);

  delete[] resultString;
}


static void continueAfterANNOUNCE(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    xvrRTSPAnnounceClient *xvrClient = (xvrRTSPAnnounceClient*)rtspClient;
    xvrSpeekClientState& scs = xvrClient->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "continueAfterANNOUNCE fail: " << resultString << "\n";
      delete[] resultString;
      break;
    }

    scs.mediaSession_ = MediaSession::createNew(env, xvrClient->sdp_->c_str());
    if (scs.mediaSession_ == NULL) break;

    MediaSubsessionIterator iter(*scs.mediaSession_);
    while ((scs.mediaSubsession_ = iter.next()) != NULL) {
      if (!scs.mediaSubsession_->initiate()) break;

      rtspClient->sendSetupCommand(*scs.mediaSubsession_, continueAfterXvrShmSETUP, False, REQUEST_STREAMING_OVER_TCP);
      break;
    }

    delete[] resultString;
    return;
  } while (0);

  // An unrecoverable error occurred with this stream.
  shutdownXvrStream(rtspClient);
}

int announce_rtsp(UsageEnvironment& env, char const* progName, char const* rtspURL, char *dsp) {
  xvrRTSPAnnounceClient* rtspClient = xvrRTSPAnnounceClient::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
  if (rtspClient == NULL) {
    env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
    return -1;
  }

  addRtspClientCont();
  //xvrRTSPConnectStatusTimer(env, rtspClient);

  rtspClient->sdp_ = boost::shared_ptr<std::string>(new std::string(dsp));
  rtspClient->sendAnnounceCommand(dsp, continueAfterANNOUNCE);
  return 0;
}


