cmake_minimum_required(VERSION 3.15)
project(YKlog)

file(GLOB SRC ${PROJECT_SOURCE_DIR}/*.cpp)

link_libraries(YKlog)
link_directories(${PATHLIB})
link_libraries(pthread)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

SET(testname functionalTest)
add_executable(${testname} ${SRC})