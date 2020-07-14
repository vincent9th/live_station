
set (GROUPSOCK_SRC GroupsockHelper.cpp GroupEId.cpp inet.c Groupsock.cpp NetInterface.cpp NetAddress.cpp IOHandlers.cpp)

include_directories(${LIVE_GROUPSOCK_INCLUDE})
include_directories(${LIVE_COMM_INCLUDE})
include_directories(${LIVE_USAGE_ENV_INCLUDE})

add_library(live_socket_a STATIC ${GROUPSOCK_SRC})
