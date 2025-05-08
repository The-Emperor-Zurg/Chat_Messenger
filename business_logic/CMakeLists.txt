file(GLOB_RECURSE BUSINESS_LOGIC_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.h
)

add_library(business_logic_lib ${BUSINESS_LOGIC_SOURCES})

target_include_directories(business_logic_lib
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/lib
            ${Boost_INCLUDE_DIRS}
)

#target_link_libraries(business_logic_lib
#        PRIVATE
#        Boost::system
#        Boost::thread
#        Boost::program_options
#)

target_link_libraries(business_logic_lib
        PRIVATE
        ${Boost_LIBRARIES}
)
