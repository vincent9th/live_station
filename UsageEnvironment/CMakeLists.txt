
set (SRCS UsageEnvironment.cpp HashTable.cpp strDup.cpp)

include_directories(${LIVE_GROUPSOCK_INCLUDE})
include_directories(${LIVE_COMM_INCLUDE})
include_directories(${LIVE_USAGE_ENV_INCLUDE})

add_library(UsageEnvironment_a STATIC ${SRCS})
