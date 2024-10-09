cmake_minimum_required(VERSION 3.10)
project(savon)

# Add the executable
add_executable(savon src/main.c src/client.c src/server.c)

# Set C++ standard
set_property(TARGET savon PROPERTY CXX_STANDARD 11)