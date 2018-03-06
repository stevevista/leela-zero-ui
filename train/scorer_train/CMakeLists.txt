cmake_minimum_required(VERSION 2.8)
project(aq)


if(MSVC)
  add_definitions(/D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-std=c++11 -Wall -Wextra -O3 -msse4.2 -DNDEBUG)
endif()

ADD_EXECUTABLE(aq 
    board.cpp
    playout.cpp
    main.cpp
    pattern3x3.cpp
    distance.cpp
    zobrist.cpp)
