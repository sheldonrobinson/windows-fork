cmake_minimum_required(VERSION 3.8)

project(forknt VERSION 1.0.0 LANGUAGES C CXX)

include(CTest)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(ENABLE_TESTS "Build tests using library" OFF)

if(MSVC)
	find_package(phnt REQUIRED)
else()
	message( FATAL_ERROR "Only MSVC supported" )
endif()

set(SRCS src/windows-fork.cpp)
if(BUILD_SHARED_LIBS)
	add_library(forknt SHARED ${SRCS})
else()
	add_library(forknt STATIC ${SRCS})
endif()
set_property(TARGET forknt PROPERTY C_STANDARD 11)
set_property(TARGET forknt PROPERTY CXX_STANDARD 17)

include_directories(include)
target_link_libraries(forknt phnt)
set_target_properties(forknt PROPERTIES LINKER_LANGUAGE CXX)



if(ENABLE_TESTS)
enable_testing()
find_package(GTest REQUIRED)
include(GoogleTest)
add_executable(fork_test "tests/fork_test.cpp")

target_link_libraries(fork_test forknt GTest::gtest GTest::gtest_main)

gtest_discover_tests(fork_test)
endif()

install(FILES ${PROJECT_SOURCE_DIR}/include/ashm.h DESTINATION include)
install(
      TARGETS forknt
	  EXPORT forknt-targets
      RUNTIME
        DESTINATION bin
      LIBRARY
        DESTINATION lib
      ARCHIVE
        DESTINATION lib
      INCLUDES
        DESTINATION include
    )

install(EXPORT forknt-targets
    FILE forknt-config.cmake
    DESTINATION lib/cmake/forknt
)