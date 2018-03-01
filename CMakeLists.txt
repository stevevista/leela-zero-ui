#
# This is a CMake makefile.  You can find the cmake utility and
# information about it at http://www.cmake.org
#
## cmake .. -G "Visual Studio 14 2015 Win64" -T host=x64 
## cmake --build . --config Release

cmake_minimum_required(VERSION 2.8)
project(gtpfox)

option(NO_GUI_SUPPORT "Use GUI to monitor selfplay" OFF)


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
ADD_EXECUTABLE(gtpfox WIN32 src/fox.rc $<TARGET_OBJECTS:objs> 
                            src/spy.cpp
                            src/main.cpp
                            src/utils.cpp)
TARGET_LINK_LIBRARIES(gtpfox ${CMAKE_THREAD_LIBS_INIT})
endif()

SET(SELFPLAY_SRC
  src/selfplay/selfplay.cpp
  src/train_data/Board.cpp
  src/train_data/convert.cpp
  src/train_data/sgf.cpp
  src/train_data/utils.cpp)


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
               src/selfplay/image_loader/libpng/png.c
               src/selfplay/image_loader/libpng/pngerror.c
               src/selfplay/image_loader/libpng/pngget.c
               src/selfplay/image_loader/libpng/pngmem.c
               src/selfplay/image_loader/libpng/pngpread.c
               src/selfplay/image_loader/libpng/pngread.c
               src/selfplay/image_loader/libpng/pngrio.c
               src/selfplay/image_loader/libpng/pngrtran.c
               src/selfplay/image_loader/libpng/pngrutil.c
               src/selfplay/image_loader/libpng/pngset.c
               src/selfplay/image_loader/libpng/pngtrans.c
               src/selfplay/image_loader/zlib/adler32.c
               src/selfplay/image_loader/zlib/crc32.c
               src/selfplay/image_loader/zlib/infback.c
               src/selfplay/image_loader/zlib/inffast.c
               src/selfplay/image_loader/zlib/inflate.c
               src/selfplay/image_loader/zlib/inftrees.c
               src/selfplay/image_loader/zlib/trees.c
               src/selfplay/image_loader/zlib/zutil.c
               src/selfplay/image_loader/png_loader.cpp
               src/selfplay/threads/threaded_object_extension.cpp
              src/selfplay/threads/threads_kernel_1.cpp
                src/selfplay/threads/threads_kernel_2.cpp
                src/selfplay/threads/threads_kernel_shared.cpp
                src/selfplay/gui_core/gui_core_kernel_1.cpp
                src/selfplay/gui_core/gui_core_kernel_2.cpp
                src/selfplay/ui.cpp
            )
endif()

ADD_EXECUTABLE(selfplay $<TARGET_OBJECTS:objs> ${SELFPLAY_SRC})
TARGET_LINK_LIBRARIES(selfplay ${CMAKE_THREAD_LIBS_INIT} ${needed_libraries})


ADD_EXECUTABLE(gendata 
  src/train_data/gendata.cpp
  src/train_data/Board.cpp
  src/train_data/convert.cpp
  src/train_data/sgf.cpp
  src/train_data/utils.cpp)

