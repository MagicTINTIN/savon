cmake_minimum_required(VERSION 3.10)
project(savon)

# Add the executable
add_executable(savon src/main.c src/network/client.c src/network/server.c src/cameras/incam.c src/cameras/outcam.c src/cameras/virtual.c)

# Set C++ standard
set_property(TARGET savon PROPERTY CXX_STANDARD 11)