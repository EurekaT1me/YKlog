cmake_minimum_required(VERSION 3.15)
project(YKlog)

include_directories(${PROJECT_SOURCE_DIR}/include)

SET(PATHLIB ${PROJECT_SOURCE_DIR}/depoly/lib)
SET(LIBRARY_OUTPUT_PATH ${PATHLIB})

add_subdirectory(test)
add_subdirectory(src)
