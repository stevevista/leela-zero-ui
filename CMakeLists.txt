#
# This is a CMake makefile.  You can find the cmake utility and
# information about it at http://www.cmake.org
#
## cmake .. -G "Visual Studio 14 2015 Win64" -T host=x64 
## cmake --build . --config Release

cmake_minimum_required(VERSION 2.8)
project(gtpfox)

FIND_PACKAGE(Threads REQUIRED)


if(MSVC)
  add_definitions(/D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-std=c++11 -Wall -Wextra)
endif()


SET(SOURCES 
  src/gtp_agent.cpp
  src/tiny-process-library/process.cpp)

if(MSVC)
  SET(SOURCES ${SOURCES}
      src/tiny-process-library/process_win.cpp)
else()
  SET(SOURCES ${SOURCES}
      src/tiny-process-library/process_unix.cpp)
endif()



ADD_LIBRARY(objs OBJECT ${SOURCES})

ADD_EXECUTABLE(gtpagent $<TARGET_OBJECTS:objs> src/main_con.cpp)
TARGET_LINK_LIBRARIES(gtpagent ${CMAKE_THREAD_LIBS_INIT})

if (MSVC)
ADD_EXECUTABLE(gtpfox WIN32 src/fox.rc ${SOURCES})
endif()


