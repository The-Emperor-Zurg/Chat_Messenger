cmake_minimum_required(VERSION 3.30)
project(PriSecChat)

set(CMAKE_CXX_STANDARD 20)

set(Boost_ROOT "C:/Boost/boost_1_88_0")
set(Boost_DIR "C:/Program Files/boost_1_88_0/stage/x64/lib/cmake/Boost-1.88.0")

find_package(Boost REQUIRED COMPONENTS system thread program_options)

add_subdirectory(business_logic)
add_subdirectory(web)
add_subdirectory(tests)

add_executable(PriSecChat main.cpp)

target_link_libraries(PriSecChat
        PRIVATE
        web_lib
        business_logic_lib
        Boost::system
        Boost::thread
        Boost::program_options
)