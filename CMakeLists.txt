#
# This is a CMake makefile.  You can find the cmake utility and
# information about it at http://www.cmake.org
#
## cmake .. -G "Visual Studio 14 2015 Win64" -T host=x64 
## cmake --build . --config Release

cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

project(gtpfox)

option(NO_GUI_SUPPORT "Use GUI to monitor playing" OFF)


FIND_PACKAGE(Threads REQUIRED)


SET(Boost_MIN_VERSION "1.58.0")
set(Boost_USE_MULTITHREADED ON)

if (NOT MSVC)

FIND_PACKAGE(Boost 1.58.0 REQUIRED program_options)
FIND_PACKAGE(OpenCL)

if (NOT OpenCL_FOUND)
add_definitions(-DFEATURE_USE_CPU_ONLY)
endif()

FIND_PACKAGE(BLAS REQUIRED)
find_path(BLAS_INCLUDE_DIRS openblas_config.h
  /usr/include
  /usr/local/include
  /usr/include/openblas
  /opt/OpenBLAS/include
  /usr/include/x86_64-linux-gnu
  $ENV{BLAS_HOME}/include)

else()

SET(Boost_INCLUDE_DIRS packages/boost.1.65.1.0/lib/native/include)
SET(BLAS_INCLUDE_DIRS packages/opencl-nug.0.777.12/build/native/include)
SET(OpenCL_INCLUDE_DIRS packages/OpenBLAS.0.2.14.1/lib/native/include)
SET(BLAS_LIBRARIES ${CMAKE_SOURCE_DIR}/packages/OpenBLAS.0.2.14.1/lib/native/lib/x64/libopenblas.dll.a)
SET(OpenCL_LIBRARIES ${CMAKE_SOURCE_DIR}/packages/opencl-nug.0.777.12/build/native/lib/x64/OpenCL.lib)

endif()


INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
if (OpenCL_INCLUDE_DIRS)
INCLUDE_DIRECTORIES(${OpenCL_INCLUDE_DIRS})
endif()
if((UNIX AND NOT APPLE) OR WIN32)
    INCLUDE_DIRECTORIES(${BLAS_INCLUDE_DIRS})
endif()
if(APPLE)
    INCLUDE_DIRECTORIES("/System/Library/Frameworks/Accelerate.framework/Versions/Current/Headers")
endif()


SET(COMMON_LIBS ${CMAKE_THREAD_LIBS_INIT} ${BLAS_LIBRARIES})
if (OpenCL_LIBRARIES)
	SET(COMMON_LIBS ${COMMON_LIBS} ${OpenCL_LIBRARIES})
endif()

if(MSVC)
  add_definitions(/D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-std=c++14 -O4 -Wall -Wextra)
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
            src/lz/NNCache.cpp
            src/lz/Tuner.cpp
            src/lz/OpenCLScheduler.cpp
            src/lz/OpenCL.cpp)


ADD_LIBRARY(objs OBJECT ${SOURCES} ${LZ_SRC})



if (MSVC)
ADD_EXECUTABLE(gtpfox WIN32 src/fox.rc $<TARGET_OBJECTS:objs>
                            src/spy.cpp
                            src/main.cpp
                            src/utils.cpp)
TARGET_LINK_LIBRARIES(gtpfox ${COMMON_LIBS})
endif()

SET(SELFPLAY_SRC
  src/board.cpp)


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

ADD_EXECUTABLE(play $<TARGET_OBJECTS:objs> ${SELFPLAY_SRC} src/play.cpp)
TARGET_LINK_LIBRARIES(play ${needed_libraries} ${COMMON_LIBS})


