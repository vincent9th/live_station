
set (MEDIA_SERVER_SRCS live555MediaServer.cpp DynamicRTSPServer.cpp)

include_directories(${LIVE_GROUPSOCK_INCLUDE})
include_directories(${LIVE_COMM_INCLUDE})
include_directories(${LIVE_USAGE_ENV_INCLUDE})
include_directories(${LIVE_BASE_ENV_INCLUDE})
include_directories(${LIVE_MEDIA_SERV_INCLUDE})
include_directories(${LIVE_LIVEMEDIA_INCLUDE})

add_library(liveMediaServer_a STATIC ${MEDIA_SERVER_SRCS})
