cmake_minimum_required(VERSION 3.16)
project(amqp_cpp_sockets_example CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(AMQP-CPP)

set(SOURCES
    unix_socket_handler/src/unix_socket_handler.cpp
)

add_executable(${PROJECT_NAME} example.cpp ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE amqpcpp)

target_include_directories(${PROJECT_NAME}
                           PRIVATE AMQP-CPP/include
                                   unix_socket_handler/include)
