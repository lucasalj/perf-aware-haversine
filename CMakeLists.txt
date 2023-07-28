find_program(CCACHE_FOUND ccache)

if(CCACHE_FOUND)
  message("Found ccache!")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments -fcolor-diagnostics")
  endif()
endif(CCACHE_FOUND)

cmake_minimum_required(VERSION 3.20)
project(Haversine VERSION 1.0)

add_executable(haversine_generator_release haversine_generator_release/main.cc)
target_include_directories(haversine_generator_release PRIVATE haversine_generator_release)
