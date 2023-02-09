cmake_minimum_required(VERSION 3.24)
project(trading-engine)

add_definitions(
    -UNDEBUG
    -DDTF_HEADER_ONLY
)

include_directories(binapi/include)
aux_source_directory(binapi/src SRC)

find_package(Boost 1.74 COMPONENTS thread REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(SRC ${SRC} src/main.cpp)

add_executable(trading-engine ${SRC})
target_link_libraries(
    ${PROJECT_NAME}
    Boost::thread
    z
    crypto
    ssl
    pthread
)
