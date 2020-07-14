cmake_minimum_required (VERSION 2.5)

project(live_station)

set (LIVE_STATION_PATH ${XVR_PROJ_PATH}/live_station)
set (LIVE_COMM_INCLUDE ${LIVE_STATION_PATH}/include)
set (LIVE_USAGE_ENV_INCLUDE ${LIVE_STATION_PATH}/UsageEnvironment/include)
set (LIVE_GROUPSOCK_INCLUDE ${LIVE_STATION_PATH}/groupsock/include)
set (LIVE_LIVEMEDIA_INCLUDE ${LIVE_STATION_PATH}/liveMedia/include)
set (LIVE_BASE_ENV_INCLUDE ${LIVE_STATION_PATH}/BasicUsageEnvironment/include)
set (LIVE_MEDIA_SERV_INCLUDE ${LIVE_STATION_PATH}/mediaServer/include)

add_subdirectory(groupsock)
add_subdirectory(liveMedia)
add_subdirectory(UsageEnvironment)
add_subdirectory(BasicUsageEnvironment)
add_subdirectory(mediaServer)
add_subdirectory(xvr_live)
