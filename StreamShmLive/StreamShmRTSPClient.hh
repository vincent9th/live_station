#ifndef _STREAMSHM_RTSP_CLIENT_HH
#define _STREAMSHM_RTSP_CLIENT_HH

#include "StreamShmPlayCommon.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "RTSPClient.hh"

// Forward function definitions:
#if 0
void continueAfterClientCreation0(RTSPClient* client, Boolean requestStreamingOverTCP);
void continueAfterClientCreation1();
void continueAfterOPTIONS(RTSPClient* client, int resultCode, char* resultString);
#endif
void continueAfterDESCRIBE(RTSPClient* client, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* client, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* client, int resultCode, char* resultString);
void continueAfterTEARDOWN(RTSPClient* client, int resultCode, char* resultString);

void createOutputFiles(char const* periodicFilenameSuffix);
void createPeriodicOutputFiles();
void setupStreams();
void closeMediaSinks();
void subsessionAfterPlaying(void* clientData);
void subsessionByeHandler(void* clientData, char const* reason);
void sessionAfterPlaying(void* clientData = NULL);
void sessionTimerHandler(void* clientData);
void periodicFileOutputTimerHandler(void* clientData);
void shutdown(int exitCode = 1);
void signalHandlerShutdown(int sig);
void checkForPacketArrival(void* clientData);
void checkInterPacketGaps(void* clientData);
void checkSessionTimeoutBrokenServer(void* clientData);
void beginQOSMeasurement();

class StreamShmRTSPClientState
{
public:
  StreamShmRTSPClientState();
  virtual ~StreamShmRTSPClientState();
  
public:
  MediaSubsessionIterator* iter;
  MediaSession* session;
  MediaSubsession* subsession;
  TaskToken streamTimerTask;
  double duration;
  
  unsigned short desiredPortNum;
  Boolean createReceivers;
  int simpleRTPoffsetArg;
  char const* singleMedium;
  unsigned fileSinkBufferSize;
  unsigned socketInputBufferSize;
  Boolean streamUsingTCP;
  Boolean forceMulticastOnUnspecified;
  Boolean madeProgress;
};

class StreamShmRTSPClient : RTSPClient
{
public:
  static StreamShmRTSPClient* createNew(UsageEnvironment& env, char const* url,
                int verbosityLevel, portNumBits tunnelOverHTTPPortNum = 0, char const* applicationName = "StreamShmRTSP");

  int Start();
  void SetAuthenticator(char const* username, char const* password, Boolean passwordIsMD5 = False);
  Authenticator* GetAuthenticator();
  
protected:
  StreamShmRTSPClient(UsageEnvironment& env, char const* rtspURL,
			     int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
    // called only by createNew();
  virtual ~StreamShmRTSPClient();

public:
  StreamShmRTSPClientState scs;
  
private:
  Authenticator* authenticator;
  
};

#endif
