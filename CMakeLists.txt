#
# This is a CMake makefile.  You can find the cmake utility and
# information about it at http://www.cmake.org
#
## cmake .. -G "Visual Studio 14 2015 Win64" -T host=x64 
## cmake --build . --config Release

cmake_minimum_required(VERSION 2.8)
project(gtpfox)

option(NO_GUI_SUPPORT "Use GUI to monitor playing" OFF)


FIND_PACKAGE(Threads REQUIRED)

FIND_PACKAGE(BLAS REQUIRED)
find_path(BLAS_INCLUDE_DIRS openblas_config.h
  /usr/include
  /usr/local/include
  /usr/include/openblas
  /opt/OpenBLAS/include
  /usr/include/x86_64-linux-gnu
  $ENV{BLAS_HOME}/include)

INCLUDE_DIRECTORIES(${BLAS_INCLUDE_DIRS})

if(MSVC)
  add_definitions(/D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-std=c++14 -Wall -Wextra)
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


add_definitions(-DFEATURE_USE_CPU_ONLY)
SET(LZ_SRC src/lz/Network.cpp
            src/lz/Random.cpp
            src/lz/GTP.cpp
            src/lz/UCTSearch.cpp
            src/lz/UCTNode.cpp
            src/lz/UCTNodeRoot.cpp
            src/lz/SMP.cpp
            src/lz/Utils.cpp
            src/lz/FastBoard.cpp
            src/lz/FullBoard.cpp
            src/lz/FastState.cpp
            src/lz/KoState.cpp
            src/lz/GameState.cpp
            src/lz/Zobrist.cpp
            src/lz/TimeControl.cpp
            src/lz/Timing.cpp
            src/lz/NNCache.cpp)

ADD_LIBRARY(lz_objs OBJECT ${LZ_SRC})


if (MSVC)
ADD_EXECUTABLE(gtpfox WIN32 src/fox.rc $<TARGET_OBJECTS:objs> $<TARGET_OBJECTS:lz_objs>
                            src/spy.cpp
                            src/main.cpp
                            src/utils.cpp)
TARGET_LINK_LIBRARIES(gtpfox ${CMAKE_THREAD_LIBS_INIT} openblas)
endif()

SET(SELFPLAY_SRC
  src/train_data/board.cpp)


if(NO_GUI_SUPPORT)
  add_definitions(-DNO_GUI_SUPPORT)
endif()

set (needed_libraries)

# we want to link to the right stuff depending on our platform.  
if (WIN32 AND NOT CYGWIN)
    if (NO_GUI_SUPPORT)
        set (needed_libraries ws2_32 winmm)
    else()
        set (needed_libraries ws2_32 winmm comctl32 gdi32 imm32)
    endif()
else()
    if (NOT NO_GUI_SUPPORT)
        include(FindX11)
        if (X11_FOUND)
            include_directories(${X11_INCLUDE_DIR})
            set (needed_libraries ${needed_libraries} ${X11_LIBRARIES})
        else()
            message(" *****************************************************************************")
            message(" *** DLIB GUI SUPPORT DISABLED BECAUSE X11 DEVELOPMENT LIBRARIES NOT FOUND ***")
            message(" *** Make sure libx11-dev is installed if you want GUI support.            ***")
            message(" *** On Ubuntu run: sudo apt-get install libx11-dev                        ***")
            message(" *****************************************************************************")
            set(NO_GUI_SUPPORT ON CACHE STRING ${DLIB_NO_GUI_SUPPORT_STR} FORCE )
            enable_preprocessor_switch(NO_GUI_SUPPORT)
        endif()
    endif()
endif()


if (NOT NO_GUI_SUPPORT)
         set(SELFPLAY_SRC ${SELFPLAY_SRC}
               src/gui/threads/threaded_object_extension.cpp
               src/gui/threads/threads_kernel_1.cpp
                src/gui/threads/threads_kernel_2.cpp
                src/gui/threads/threads_kernel_shared.cpp
                src/gui/gui_core_kernel_1.cpp
                src/gui/gui_core_kernel_2.cpp
                src/gui/ui.cpp
            )
endif()

ADD_EXECUTABLE(play $<TARGET_OBJECTS:objs> $<TARGET_OBJECTS:lz_objs> ${SELFPLAY_SRC} src/play.cpp)
TARGET_LINK_LIBRARIES(play ${CMAKE_THREAD_LIBS_INIT} ${needed_libraries} openblas)


ADD_EXECUTABLE(gendata 
  src/train_data/gendata.cpp
  src/train_data/board.cpp
  src/train_data/archive.cpp
  src/train_data/sgf.cpp
  src/train_data/utils.cpp)

