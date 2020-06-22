#include "StreamShmRTSPClient.hh"

static void StreamShmSetupStreams(RTSPClient* rtspClient);

static void shutdownStreamShmClient(RTSPClient* rtspClient, int exitCode) {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamShmRTSPClient *streamShmClient = static_cast<StreamShmRTSPClient *>rtspClient;
    StreamShmRTSPClientState& scs = ((StreamShmRTSPClient*)rtspClient)->scs; // alias
#if 0
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

    if (--rtspClientCount == 0) {
    // The final stream has ended, so exit the application now.
    // (Of course, if you're embedding this code into your own application, you might want to comment this out,
    // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
    exit(exitCode);
    }
#endif
}

static void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamShmRTSPClient *streamShmClient = static_cast<StreamShmRTSPClient *>rtspClient;
    StreamShmRTSPClientState& scs = ((StreamShmRTSPClient*)rtspClient)->scs; // alias
    MediaSubsession *subsession = scs.subsession;
    
  if (resultCode == 0) {
      *env << "Setup \"" << subsession->mediumName()
	   << "/" << subsession->codecName()
	   << "\" subsession (";
      if (subsession->rtcpIsMuxed()) {
	*env << "client port " << subsession->clientPortNum();
      } else {
	*env << "client ports " << subsession->clientPortNum()
	     << "-" << subsession->clientPortNum()+1;
      }
      *env << ")\n";
      scs.madeProgress = True;
  } else {
    *env << "Failed to setup \"" << subsession->mediumName()
	 << "/" << subsession->codecName()
	 << "\" subsession: " << resultString << "\n";
  }
  delete[] resultString;

  //if (rtspClient != NULL) sessionTimeoutParameter = rtspClient->sessionTimeoutParameter();

  // Set up the next subsession, if any:
  StreamShmSetupStreams(rtspClient);
}

static void createStreamShmSink(RTSPClient* rtspClient, char const* periodicFilenameSuffix) {
  char outFileName[1000];

    // Create and start "FileSink"s for each subsession:
    madeProgress = False;
    MediaSubsessionIterator iter(*session);
    while ((subsession = iter.next()) != NULL) {
        if (subsession->readSource() == NULL) continue; // was not initiated

        // Create an output file for each desired stream:
        if (singleMedium == NULL || periodicFilenameSuffix[0] != '\0') {
            // Output file name is
            //     "<filename-prefix><medium_name>-<codec_name>-<counter><periodicFilenameSuffix>"
            static unsigned streamCounter = 0;
            snprintf(outFileName, sizeof outFileName, "%s%s-%s-%d%s",
                fileNamePrefix, subsession->mediumName(),
                subsession->codecName(), ++streamCounter, periodicFilenameSuffix);
        } else {
            // When outputting a single medium only, we output to 'stdout
            // (unless the '-P <interval-in-seconds>' option was given):
            sprintf(outFileName, "stdout");
        }

        FileSink* fileSink = NULL;
        Boolean createOggFileSink = False; // by default
        if (strcmp(subsession->mediumName(), "video") == 0) {
            if (strcmp(subsession->codecName(), "H264") == 0) {
                // For H.264 video stream, we use a special sink that adds 'start codes',
                // and (at the start) the SPS and PPS NAL units:
                fileSink = H264VideoFileSink::createNew(*env, outFileName,
                    subsession->fmtp_spropparametersets(),
                    fileSinkBufferSize, oneFilePerFrame);
            } else if (strcmp(subsession->codecName(), "H265") == 0) {
                // For H.265 video stream, we use a special sink that adds 'start codes',
                // and (at the start) the VPS, SPS, and PPS NAL units:
                fileSink = H265VideoFileSink::createNew(*env, outFileName,
                                subsession->fmtp_spropvps(),
                                subsession->fmtp_spropsps(),
                                subsession->fmtp_sproppps(),
                                fileSinkBufferSize, oneFilePerFrame);
            }
        } else if (strcmp(subsession->mediumName(), "audio") == 0) {
            ;
        }
        
        if (fileSink == NULL) {
            // Normal case:
            fileSink = FileSink::createNew(*env, outFileName,
                fileSinkBufferSize, oneFilePerFrame);
        }
        subsession->sink = fileSink;

        if (subsession->sink == NULL) {
            *env << "Failed to create FileSink for \"" << outFileName
            << "\": " << env->getResultMsg() << "\n";
        } else {
            if (singleMedium == NULL) {
                *env << "Created output file: \"" << outFileName << "\"\n";
            } else {
                *env << "Outputting data from the \"" << subsession->mediumName()
                    << "/" << subsession->codecName()
                    << "\" subsession to \"" << outFileName << "\"\n";
            }

            if (strcmp(subsession->mediumName(), "video") == 0 &&
                strcmp(subsession->codecName(), "MP4V-ES") == 0 &&
                subsession->fmtp_config() != NULL) {
                // For MPEG-4 video RTP streams, the 'config' information
                // from the SDP description contains useful VOL etc. headers.
                // Insert this data at the front of the output file:
                unsigned configLen;
                unsigned char* configData
                = parseGeneralConfigStr(subsession->fmtp_config(), configLen);
                struct timeval timeNow;
                gettimeofday(&timeNow, NULL);
                fileSink->addData(configData, configLen, timeNow);
                delete[] configData;
            }

            subsession->sink->startPlaying(*(subsession->readSource()),
                    subsessionAfterPlaying,
                    subsession);

            // Also set a handler to be called if a RTCP "BYE" arrives
            // for this subsession:
            if (subsession->rtcpInstance() != NULL) {
                subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, subsession);
            }

            madeProgress = True;
        }
    }
    
    if (!madeProgress) shutdown();
}

static void StreamShmSetupStreams(RTSPClient* rtspClient) {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamShmRTSPClient *streamShmClient = static_cast<StreamShmRTSPClient *>rtspClient;
    StreamShmRTSPClientState& scs = ((StreamShmRTSPClient*)rtspClient)->scs; // alias

    MediaSubsessionIterator* setupIter = scs.iter;
    while ((scs.subsession = setupIter->next()) != NULL) {
        // We have another subsession left to set up:
        MediaSubsession* subsession = scs.subsession;
        if (subsession->clientPortNum() == 0) continue; // port # was not set

        //setupSubsession(subsession, streamUsingTCP, forceMulticastOnUnspecified, continueAfterSETUP);
        rtspClient->sendSetupCommand(*subsession, continueAfterSETUP, False, scs.streamUsingTCP,
            scs.forceMulticastOnUnspecified, streamShmClient->authenticator);
        return;
    }
    
    if (!scs.madeProgress)
    {
        shutdownStreamShmClient(rtspClient);
        return;
    }

#if 0
    // Create output files:
    if (createReceivers) {
        if (fileOutputInterval > 0) {
            createPeriodicOutputFiles();
        } else {
            createOutputFiles("");
        }
    }
#else
    createStreamShmSink(rtspClient);
#endif

    // Finally, start playing each subsession, to start the data flow:
    if (duration == 0) {
        if (scale > 0) duration = session->playEndTime() - initialSeekTime; // use SDP end time
        else if (scale < 0) duration = initialSeekTime;
    }
    if (duration < 0) duration = 0.0;

    endTime = initialSeekTime;
    if (scale > 0) {
        if (duration <= 0) endTime = -1.0f;
        else endTime = initialSeekTime + duration;
    } else {
        endTime = initialSeekTime - duration;
        if (endTime < 0) endTime = 0.0f;
    }

    char const* absStartTime = initialAbsoluteSeekTime != NULL ? initialAbsoluteSeekTime : session->absStartTime();
    char const* absEndTime = initialAbsoluteSeekEndTime != NULL ? initialAbsoluteSeekEndTime : session->absEndTime();
    if (absStartTime != NULL) {
        // Either we or the server have specified that seeking should be done by 'absolute' time:
        startPlayingSession(session, absStartTime, absEndTime, scale, continueAfterPLAY);
    } else {
        // Normal case: Seek by relative time (NPT):
        startPlayingSession(session, initialSeekTime, endTime, scale, continueAfterPLAY);
    }
}

static void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamShmRTSPClient *streamShmClient = static_cast<StreamShmRTSPClient *>rtspClient;
    StreamShmRTSPClientState& scs = ((StreamShmRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
        //*env << "Failed to get a SDP description for the URL \"" << streamURL << "\": " << resultString << "\n";
        env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
        delete[] resultString;
        shutdownStreamShmClient(rtspClient);
        return;
    }

    char* sdpDescription = resultString;
    env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

    // Create a media session object from this SDP description:
    scs.session = MediaSession::createNew(*env, sdpDescription);
    delete[] sdpDescription;

    if (scs.session == NULL) {
        *env << "Failed to create a MediaSession object from the SDP description: " << env->getResultMsg() << "\n";
        shutdownStreamShmClient(rtspClient);
        return;
    }
    if (!scs.session->hasSubsessions()) {
        *env << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
        shutdownStreamShmClient(rtspClient);
        return;
    }

    // Then, setup the "RTPSource"s for the session:
    //MediaSubsessionIterator iter(*session);
    scs.iter = new MediaSubsessionIterator(*scs.session);
    
    MediaSubsession *subsession;
    Boolean madeProgress = False;
    char const* singleMediumToTest = scs.singleMedium;
    while ((subsession = scs.iter->next()) != NULL) {
        // If we've asked to receive only a single medium, then check this now:
        if (singleMediumToTest != NULL) {
            if (strcmp(subsession->mediumName(), singleMediumToTest) != 0) {
        *env << "Ignoring \"" << subsession->mediumName()
          << "/" << subsession->codecName()
          << "\" subsession, because we've asked to receive a single " << scs.singleMedium
          << " session only\n";
        continue;
            } else {
        // Receive this subsession only
        singleMediumToTest = "xxxxx";
        // this hack ensures that we get only 1 subsession of this type
            }
        }

        if (scs.desiredPortNum != 0) {
        subsession->setClientPortNum(scs.desiredPortNum);
        scs.desiredPortNum += 2;
        }

        if (scs.createReceivers) {
            if (!subsession->initiate(scs.simpleRTPoffsetArg)) {
                *env << "Unable to create receiver for \"" << subsession->mediumName()
                << "/" << subsession->codecName()
                << "\" subsession: " << env->getResultMsg() << "\n";
            } else {
                *env << "Created receiver for \"" << subsession->mediumName()
                << "/" << subsession->codecName() << "\" subsession (";
                if (subsession->rtcpIsMuxed()) {
                    *env << "client port " << subsession->clientPortNum();
                } else {
                    *env << "client ports " << subsession->clientPortNum()
                    << "-" << subsession->clientPortNum()+1;
                }
                *env << ")\n";
                madeProgress = True;

                if (subsession->rtpSource() != NULL) {
                    // Because we're saving the incoming data, rather than playing
                    // it in real time, allow an especially large time threshold
                    // (1 second) for reordering misordered incoming packets:
                    unsigned const thresh = 1000000; // 1 second
                    subsession->rtpSource()->setPacketReorderingThresholdTime(thresh);

                    // Set the RTP source's OS socket buffer size as appropriate - either if we were explicitly asked (using -B),
                    // or if the desired FileSink buffer size happens to be larger than the current OS socket buffer size.
                    // (The latter case is a heuristic, on the assumption that if the user asked for a large FileSink buffer size,
                    // then the input data rate may be large enough to justify increasing the OS socket buffer size also.)
                    int socketNum = subsession->rtpSource()->RTPgs()->socketNum();
                    unsigned curBufferSize = getReceiveBufferSize(*env, socketNum);
                    if (scs.socketInputBufferSize > 0 || scs.fileSinkBufferSize > curBufferSize) {
                        unsigned newBufferSize = scs.socketInputBufferSize > 0 ? scs.socketInputBufferSize : scs.fileSinkBufferSize;
                        newBufferSize = setReceiveBufferTo(*env, socketNum, newBufferSize);
                            if (scs.socketInputBufferSize > 0) { // The user explicitly asked for the new socket buffer size; announce it:
                            *env << "Changed socket receive buffer size for the \""
                                << subsession->mediumName()
                                << "/" << subsession->codecName()
                                << "\" subsession from "
                                << curBufferSize << " to "
                                << newBufferSize << " bytes\n";
                        }
                    }
                }
            }
        } else {
            if (subsession->clientPortNum() == 0) {
                *env << "No client port was specified for the \""
                    << subsession->mediumName()
                    << "/" << subsession->codecName()
                    << "\" subsession.  (Try adding the \"-p <portNum>\" option.)\n";
            } else {
                madeProgress = True;
            }
        }
    }
    
    if (!madeProgress)
    {
        shutdownStreamShmClient(rtspClient);
        return;
    }
    
    scs.iter->reset();
    // Perform additional 'setup' on each subsession, before playing them:
    StreamShmSetupStreams(rtspClient);
}

static void continueAfterOPTIONS(RTSPClient* rtspClient, int resultCode, char* resultString) {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamShmRTSPClient *streamShmClient = static_cast<StreamShmRTSPClient *>rtspClient;
    //StreamShmRTSPClientState& scs = ((StreamShmRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
        env << *rtspClient << " \"OPTIONS\" request failed: " << resultString << "\n";
        delete[] resultString;
        shutdownStream(rtspClient);
    }
    else
    {
        delete[] resultString;
        // Next, get a SDP description for the stream:
        rtspClient->sendDescribeCommand(continueAfterDESCRIBE, streamShmClient->GetAuthenticator());
    }
}

StreamShmRTSPClient::StreamShmRTSPClient(UsageEnvironment& env, char const* rtspURL,
			     int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, tunnelOverHTTPPortNum),
    authenticator(nullptr) {
}

StreamShmRTSPClient::~StreamShmRTSPClient() {
    delete authenticator;
}

StreamShmRTSPClient *StreamShmRTSPClient::createNew(UsageEnvironment & env, char const * url, int verbosityLevel, portNumBits tunnelOverHTTPPortNum, char const* applicationName)
{
    return new StreamShmRTSPClient(env, url, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

void StreamShmRTSPClient::SetAuthenticator(char const* username, char const* password, Boolean passwordIsMD5)
{
    authenticator = new Authenticator(username, password, passwordIsMD5);
}

Authenticator* StreamShmRTSPClient::GetAuthenticator()
{
    return authenticator;
}

StreamShmRTSPClient::Start(Boolean sendOptionsRequest, char* userAgent)
{
    setUserAgentString(userAgent);
  if (sendOptionsRequest) {
    // Begin by sending an "OPTIONS" command:
    sendOptionsCommand(continueAfterOPTIONS, authenticator);
  } else {
    continueAfterOPTIONS(NULL, 0, NULL);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StreamShmRTSPClientState::StreamShmRTSPClientState()
  : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0),
    desiredPortNum(0), createReceivers(True), simpleRTPoffsetArg(-1), singleMedium(nullptr),
    fileSinkBufferSize(100000), socketInputBufferSize(0), streamUsingTCP(False), forceMulticastOnUnspecified(False),
    madeProgress(False) {
}

StreamShmRTSPClientState::~StreamShmRTSPClientState() {
  delete iter;
  if (session != NULL) {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    UsageEnvironment& env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }

  delete[] singleMedium;
}

