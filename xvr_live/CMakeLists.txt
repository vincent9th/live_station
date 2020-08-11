
set (LIVE_STATION_SRCS
	xvrRTSPClient.cpp xvrRTSPApp.cpp xvrRTSPAnnounce.cpp)

include_directories(${COMM_LIB_PATH}/boost)
include_directories(${COMM_LIB_PATH}/libuv/include)
include_directories(${COMM_LIB_PATH}/xvr_util/include)
include_directories(${COMM_LIB_PATH}/streamshmgr/include)
include_directories(${LIVE_GROUPSOCK_INCLUDE})
include_directories(${LIVE_COMM_INCLUDE})
include_directories(${LIVE_USAGE_ENV_INCLUDE})
include_directories(${LIVE_BASE_ENV_INCLUDE})
include_directories(${LIVE_MEDIA_SERV_INCLUDE})
include_directories(${LIVE_LIVEMEDIA_INCLUDE})

set (LIVE_LIBS liveMediaServer_a liveMedia_a BasicUsageEnvironment_a UsageEnvironment_a live_socket_a streamshm_a uv_a pthread)

add_executable (live_station ${LIVE_STATION_SRCS})
target_link_libraries (live_station ${LIVE_LIBS})

