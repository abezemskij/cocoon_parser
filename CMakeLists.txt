cmake_minimum_required(VERSION 3.0.0)
project(IDCM C)
# Identify wether the machine is 32 or 64 bit
SET(CMAKE_C_COMPILER g++)
SET(CMAKE_C_FLAGS "-Wall -lm")
add_executable(parser ./src/main.c)
