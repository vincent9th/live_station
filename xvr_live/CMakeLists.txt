
set (LIVE_STATION_SRCS xvrRTSPClient.cpp)

include_directories(${COMM_LIB_PATH}/boost)
include_directories(${LIVE_GROUPSOCK_INCLUDE})
include_directories(${LIVE_COMM_INCLUDE})
include_directories(${LIVE_USAGE_ENV_INCLUDE})
include_directories(${LIVE_BASE_ENV_INCLUDE})
include_directories(${LIVE_MEDIA_SERV_INCLUDE})
include_directories(${LIVE_LIVEMEDIA_INCLUDE})

set (LIVE_LIBS liveMediaServer_a liveMedia_a BasicUsageEnvironment_a UsageEnvironment_a live_socket_a)

add_executable (live_station ${LIVE_STATION_SRCS})
target_link_libraries (live_station ${LIVE_LIBS})