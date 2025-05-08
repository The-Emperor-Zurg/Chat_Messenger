cmake_minimum_required(VERSION 3.30)
project(web)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE WEB_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_library(web_lib STATIC ${WEB_SOURCES})

target_include_directories(web_lib
        PUBLIC
        ${CMAKE_SOURCE_DIR}/business_logic/lib
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(web_lib
        PRIVATE
            business_logic_lib
            Boost::system
            Boost::thread
            Boost::program_options
)