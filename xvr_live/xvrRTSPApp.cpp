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
#include "KULog.h"
#include "sepcam_ipc_api.h"
#include "KULog.h"
#include "ipcnet_struct_internal.h"
//using namespace std;
//using namespace boost;

// Forward function definitions:
static int gPipe[2] = {-1, -1};

int announce_rtsp(UsageEnvironment& env, char const* progName, char const* rtspURL, char *dsp);

void usage(UsageEnvironment& env, char const* progName) {
  env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
  env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

char eventLoopWatchVariable = 0;

/*void DevCheckTimerHandler(UsageEnvironment& env) {
}

void xvrDevConnectTimer(UsageEnvironment& env)
{
  env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)DevCheckTimerHandler, env);
}*/

static void ipc_broadcast_inform_callback(void *loop_handle, char *src_name, uint16_t msg_type, void *data, size_t datalen)
{
  if(IPC_STATION_ADD_RTSP_DEVICE_REQ == msg_type)
  {
    if(sizeof(IPCNetStationRtspDevice_st) != datalen)
    {
      LOGE((char*)"sizeof(IPCNetStationRtspDevice_st):%d != datalen:%d", sizeof(IPCNetStationRtspDevice_st), datalen);
      return;
    }
    
    IPCNetStationRtspDevice_st *rtsp_device = (IPCNetStationRtspDevice_st*)data;
    LOGI((char*)"rtsp channel num:%d", rtsp_device->__pri_ChannelCount);
    for(int i = 0; i < rtsp_device->__pri_ChannelCount; ++i)
    {
      LOGI((char*)"i:%d uri:%s", i, rtsp_device->Channel[i].Uri);
    }

    
  }
  else
  {
  }
}

static int __nonblock_fcntl(int fd, int set) {
  int flags;
  int r;

  do
    r = fcntl(fd, F_GETFL);
  while (r == -1 && errno == EINTR);

  if (r == -1)
    return -1;

  /* Bail out now if already set/clear. */
  if (!!(r & O_NONBLOCK) == !!set)
    return 0;

  if (set)
    flags = r | O_NONBLOCK;
  else
    flags = r & ~O_NONBLOCK;

  do
    r = fcntl(fd, F_SETFL, flags);
  while (r == -1 && errno == EINTR);

  if (r)
    return -1;

  return 0;
}

static void create_socketpair(int fds [ 2 ])
{
  SocketPipe(fds);
  __nonblock_fcntl(fds[0], 1);
  __nonblock_fcntl(fds[1], 1);
}

static void pipeIncomingDataHandler(void* instance, int mask)
{
}

int main(int argc, char** argv) {
#if 1
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);
  int r;
  
#if 1
  // We need at least one "rtsp://" URL argument:
  if (argc < 1) {
    usage(*env, argv[0]);
    return 1;
  }
  // There are argc-1 URLs: argv[1] through argv[argc-1].  Open and start streaming each one:
  for (int i = 1; i <= argc-1; ++i)
  {
    xvrRTSPClientOpenURL(*env, "live_station", argv[1], 0, 0);
  }
#endif

  create_socketpair(gPipe);

#if 0
  r = sepcam_ipc_init((char*)"rtsp_cli");
  if(r < 0)
  {
    //LOGE("sepcam_ipc_init FAIL");
    return -1;
  }

  ipcam_regist_broadcast_callback(ipc_broadcast_inform_callback);
#endif

  env->taskScheduler().setBackgroundHandling(gPipe[0], SOCKET_READABLE|SOCKET_EXCEPTION,
					  (TaskScheduler::BackgroundHandlerProc*)&pipeIncomingDataHandler, NULL);
					  
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

