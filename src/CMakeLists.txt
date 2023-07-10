cmake_minimum_required(VERSION 3.15)
project(YKlog)


file(GLOB SRC ${PROJECT_SOURCE_DIR}/*.cpp)

add_library(YKlog STATIC ${SRC})
