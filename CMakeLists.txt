project(chat_example)
cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 17)

add_executable(chat_server server.cpp)
add_executable(chat_client client.cpp)

install (TARGETS chat_server chat_client DESTINATION ${CMAKE_INSTALL_BINDIR})
