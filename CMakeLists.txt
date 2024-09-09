cmake_minimum_required(VERSION 3.10)
project(savon)

# Find the GStreamer libraries
find_package(PkgConfig)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0)

# Include GStreamer headers
include_directories(${GSTREAMER_INCLUDE_DIRS})
link_directories(${GSTREAMER_LIBRARY_DIRS})

# Add the executable
add_executable(savon src/main.cc)

# Link the GStreamer libraries
target_link_libraries(savon ${GSTREAMER_LIBRARIES} ${GSTREAMER_VIDEO_LIBRARIES})

# Set C++ standard
set_property(TARGET savon PROPERTY CXX_STANDARD 11)