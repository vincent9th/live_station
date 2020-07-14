
set (SRCS BasicUsageEnvironment0.cpp BasicUsageEnvironment.cpp 
        BasicTaskScheduler0.cpp BasicTaskScheduler.cpp 
        DelayQueue.cpp BasicHashTable.cpp)

include_directories(${LIVE_GROUPSOCK_INCLUDE})
include_directories(${LIVE_COMM_INCLUDE})
include_directories(${LIVE_USAGE_ENV_INCLUDE})
include_directories(${LIVE_BASE_ENV_INCLUDE})

add_library(BasicUsageEnvironment_a STATIC ${SRCS})
