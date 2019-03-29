#cmake 最低版本号要求

cmake_minimum_required (VERSION 2.5)

# 项目信息

project (HolleWorld)
AUX_SOURCE_DIRECTORY(. DIR_SRCS)
# 指定生成目标

add_executable(bin ${DIR_SRCS})
