cmake_minimum_required(VERSION 3.15...4.0)

# make shall print the commands it is executing
set(CMAKE_VERBOSE_MAKEFILE on)

# set default build type to Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# tweak compiler flags
set(CMAKE_C_FLAGS "-Wall -Wextra  -D _GNU_SOURCE")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

# Define our project
project(rtl-wmbus-ng VERSION 0.1
  DESCRIPTION "Decoder for wmbus messages"
  LANGUAGES C)


# Find Git executable
find_package(Git)
if(GIT_FOUND)
  # Get the current branch
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # Get the HEAD revision (short SHA)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # Get the tag name (if it exists)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --exact-match
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE GIT_TAG_RESULT
  )

  # Check for uncommitted changes
  execute_process(
    COMMAND ${GIT_EXECUTABLE} status --porcelain
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_STATUS
  )

  # If there are uncommitted changes, append "(modified)"
  if(GIT_STATUS)
      set(GIT_MODIFIED "(modified)")
  else()
      set(GIT_MODIFIED "")
  endif()

  # If a tag exists, use it; otherwise, use the branch name
  if(GIT_TAG_RESULT EQUAL 0 AND GIT_TAG)
    set(GIT_VERSION "${GIT_TAG}:${GIT_REVISION}${GIT_MODIFIED}")
  else()
    set(GIT_VERSION "${GIT_BRANCH}:${GIT_REVISION}${GIT_MODIFIED}")
  endif()

  # Define the macro for the C compiler
  add_definitions(-DGIT_VERSION="${GIT_VERSION}")
else()
  message(WARNING "Git not found, GIT_VERSION macro will not be defined")
  add_definitions(-DGIT_VERSION="unknown")
endif()

# Define the macro for the C compiler
add_definitions(-DGIT_VERSION="${GIT_VERSION}")

# add source files
add_executable(rtl_wmbus
  src/dump.c
  src/buffers.c
  src/fifo.c
  src/main.c
  src/queue.c
  src/rtl_wmbus_util.c
  src/t1_c1_decimator.c
  src/t1_c1_decoder.c
  src/t1_c1_polar_discriminator.c
  src/config.c
  src/logging.c
)

# add include directories
target_include_directories(rtl_wmbus PUBLIC include)

# We rely on the math library for floating point fuctions
target_link_libraries(rtl_wmbus m)

# Enable saving of preprocessed files
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -save-temps")
