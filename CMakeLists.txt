cmake_minimum_required(VERSION 3.15...4.0)

# make shall print the commands it is executing
set( CMAKE_VERBOSE_MAKEFILE on )

# set default build type to Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# tweak compiler flags
set(CMAKE_C_FLAGS "-Wall -Wextra  -D _GNU_SOURCE")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

# Define our project
project(rtl-wmbus-ng  VERSION 0.1
                      DESCRIPTION "Decoder for wmbus messages"
                      LANGUAGES C)

# add source files
add_executable(rtl-wmbus
  src/buffers.c
  src/fifo.c
  src/main.c
  src/queue.c
  src/rtl_wmbus_util.c
  src/t1_c1_decimator.c
  src/t1_c1_decoder.c
  src/t1_c1_polar_discriminator.c
)

# add include directories
# TODO: Move the fixedpoint library to somewhere else
target_include_directories(rtl-wmbus PUBLIC include)

# We rely on the math library for floating point fuctions
target_link_libraries(rtl-wmbus m)
