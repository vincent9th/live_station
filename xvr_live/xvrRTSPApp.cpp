/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2019, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/

#include "xvrRTSPClient.h"

//using namespace std;
//using namespace boost;

// Forward function definitions:

int announce_rtsp(UsageEnvironment& env, char const* progName, char const* rtspURL, char *dsp);

void usage(UsageEnvironment& env, char const* progName) {
  env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
  env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

char eventLoopWatchVariable = 0;

int main(int argc, char** argv) {
#if 1
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

  // We need at least one "rtsp://" URL argument:
  if (argc < 1) {
    usage(*env, argv[0]);
    return 1;
  }

  // There are argc-1 URLs: argv[1] through argv[argc-1].  Open and start streaming each one:
  //for (int i = 1; i <= argc-1; ++i)
  {
    xvrRTSPClientOpenURL(*env, "live_station", argv[1]);
  }

  // All subsequent activity takes place within the event loop:
  env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
    // This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.
#else
    char *dsp = "v=0\r\n"
"o=freesbell 3805625600 3805625600 IN IP4 192.168.3.100\r\n"
"s=Session \r\n"
"e=NONE\r\n"
"c=IN IP4 0.0.0.0\r\n"
"t=0 0\r\n"
"a=range:npt=now-\r\n"
"a=control:*\r\n"
"m=audio 8888 RTP/AVP 8\r\n"
"a=rtpmap:8 PCMA/8000\r\n"
"a=control:trackID=1\r\n";
    announce_rtsp(*env, "live_station", argv[1], dsp);
#endif

    return 0;
}

