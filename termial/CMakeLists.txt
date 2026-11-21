cmake_minimum_required(VERSION 3.15)
project(Termial2ESP32 LANGUAGES C CXX )

add_executable(termial
    "src/main.cpp"
    "src/console/serial.cpp"
)
include_directories(termial ${CMAKE_CURRENT_SOURCE_DIR}/include)