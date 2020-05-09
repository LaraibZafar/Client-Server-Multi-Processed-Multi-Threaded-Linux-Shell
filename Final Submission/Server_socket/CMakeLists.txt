cmake_minimum_required(VERSION 3.15)
project(Socket_server_parser)

set(CMAKE_CXX_STANDARD 14)

add_executable(Socket_server_parser main.cpp C_SocketServer.c)