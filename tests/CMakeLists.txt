# Google Test
include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Boost.Test
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

# Test files
add_executable(BusinessLogicTests test_business_logic.cpp)
add_executable(WebTests web_tests.cpp)

#Link
target_link_libraries(BusinessLogicTests
        PRIVATE
        business_logic_lib
        gtest_main
)

target_link_libraries(WebTests
        PRIVATE
        business_logic_lib
        web_lib
        Boost::unit_test_framework
)

target_compile_definitions(WebTests PRIVATE BOOST_TEST_DYN_LINK)

# ctest
enable_testing()
add_test(NAME BusinessLogicTests COMMAND BusinessLogicTests)
add_test(NAME WebTests COMMAND WebTests)
