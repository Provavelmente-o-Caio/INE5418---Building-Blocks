cmake_minimum_required(VERSION 4.2)
project(distribuida_snapshot)

set(CMAKE_CXX_STANDARD 26)

add_executable(distribuida_snapshot main.cpp
        util/Message.cpp
        util/Message.h
        net/NetworkNode.cpp
        net/NetworkNode.h
        app/Conta.cpp
        app/Conta.h
        app/Agencia.cpp
        app/Agencia.h)
