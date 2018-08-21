cmake_minimum_required(VERSION 3.2.0)
project(easyvpn)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_FLAGS "-std=c11") 

include_directories(include)
include_directories(/usr/include)
include_directories(/usr/local/include)
link_directories(/usr/lib)
link_directories(/usr/local/lib)

file(GLOB SOURCES "src/*.c")

add_executable(easyvpn ${SOURCES})

target_link_libraries(easyvpn sqlite3 msgpackc)
