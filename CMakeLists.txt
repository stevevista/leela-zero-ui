#
# This is a CMake makefile.  You can find the cmake utility and
# information about it at http://www.cmake.org
#
## cmake .. -G "Visual Studio 14 2015 Win64" -T host=x64 
## cmake --build . --config Release

cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

project(gtpfox)

option(NO_GUI "NOT Use GUI to monitor playing" OFF)
option(NO_GPU "NOT Use GPU" OFF)

FIND_PACKAGE(Threads REQUIRED)
find_package(ZLIB REQUIRED)

SET(Boost_MIN_VERSION "1.58.0")
set(Boost_USE_MULTITHREADED ON)

if (NOT MSVC)

FIND_PACKAGE(Boost 1.58.0 REQUIRED program_options)
FIND_PACKAGE(OpenCL)

if (NOT OpenCL_FOUND)
add_definitions(-DUSE_CPU_ONLY)
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


if (NOT MSVC)
  SET(GCC_COMPILE_FLAGS "-Wall -Wextra -ffast-math -flto -march=native")
  SET(GCC_DISABLED_WARNING_COMPILE_FLAGS "-Wno-ignored-attributes -Wno-maybe-uninitialized")
  SET(GCC_FLAGS "${GCC_COMPILE_FLAGS} ${GCC_DISABLED_WARNING_COMPILE_FLAGS}")
  SET(CMAKE_CXX_FLAGS_DEBUG "${GCC_FLAGS} -g -Og")
  SET(CMAKE_CXX_FLAGS_RELEASE "${GCC_FLAGS} -g -O3 -DNDEBUG")
  SET(CMAKE_EXE_LINKER_FLAGS "-flto -g")
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

include_directories(${ZLIB_INCLUDE_DIRS})

SET(COMMON_LIBS ${CMAKE_THREAD_LIBS_INIT} ${BLAS_LIBRARIES} ${ZLIB_LIBRARIES})


if(MSVC)
  add_definitions(/D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-std=c++14 -O4 -Wall -Wextra -DNDEBUG)
endif()

SET(TINY_PROC_SRC 
  src/tiny-process-library/process.cpp)

if(MSVC)
  SET(TINY_PROC_SRC ${TINY_PROC_SRC}
      src/tiny-process-library/process_win.cpp)
else()
  SET(TINY_PROC_SRC ${TINY_PROC_SRC}
      src/tiny-process-library/process_unix.cpp)
endif()


if(NO_GUI)
  add_definitions(-DNO_GUI_SUPPORT)
endif()

if(NO_GPU)
  add_definitions(-DFEATURE_USE_CPU_ONLY)
else()
    if (OpenCL_LIBRARIES)
	    SET(COMMON_LIBS ${COMMON_LIBS} ${OpenCL_LIBRARIES})
    endif()
endif()

SET(SOURCES 
  src/gtp_agent.cpp
  src/gtp_lz.cpp
  src/gtp_choice.cpp
  src/gtp_game.cpp
  src/tools.cpp
  src/board.cpp
  ${TINY_PROC_SRC})




SET(LZ_SRC src/lz/Network.cpp
            src/lz/Random.cpp
            src/lz/GTP.cpp
            src/lz/UCTSearch.cpp
            src/lz/UCTNode.cpp
            src/lz/UCTNodePointer.cpp
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
            src/lz/OpenCL.cpp
            src/lz/SGFParser.cpp
            src/lz/SGFTree.cpp
            src/lz/Training.cpp
            src/lz/fix/ladder.cpp)


ADD_LIBRARY(objs OBJECT ${SOURCES} ${LZ_SRC})



if (MSVC)
ADD_EXECUTABLE(gtpfox WIN32 src/foxwq/fox.rc $<TARGET_OBJECTS:objs>
                            src/foxwq/spy.cpp
                            src/foxwq/main.cpp
                            src/foxwq/utils.cpp)
TARGET_LINK_LIBRARIES(gtpfox ${COMMON_LIBS})
endif()


set (needed_libraries)

# we want to link to the right stuff depending on our platform.  
if (WIN32 AND NOT CYGWIN)
    if (NO_GUI)
        set (needed_libraries ws2_32 winmm)
    else()
        set (needed_libraries ws2_32 winmm comctl32 gdi32 imm32)
    endif()
else()
    if (NOT NO_GUI)
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
            add_definitions(-DNO_GUI_SUPPORT)
        endif()
    endif()
endif()


set(dlib_source_files 
            src/dlib/base64/base64_kernel_1.cpp
            src/dlib/entropy_decoder/entropy_decoder_kernel_1.cpp
            src/dlib/entropy_decoder/entropy_decoder_kernel_2.cpp
            src/dlib/entropy_encoder/entropy_encoder_kernel_1.cpp
            src/dlib/entropy_encoder/entropy_encoder_kernel_2.cpp
            src/dlib/tokenizer/tokenizer_kernel_1.cpp
            src/dlib/unicode/unicode.cpp
            src/dlib/dir_nav/dir_nav_kernel_1.cpp
            src/dlib/dir_nav/dir_nav_kernel_2.cpp
            src/dlib/misc_api/misc_api_kernel_1.cpp
            src/dlib/misc_api/misc_api_kernel_2.cpp
            src/dlib/threads/multithreaded_object_extension.cpp
            src/dlib/threads/threaded_object_extension.cpp
            src/dlib/threads/threads_kernel_1.cpp
            src/dlib/threads/threads_kernel_2.cpp
            src/dlib/threads/threads_kernel_shared.cpp
            src/dlib/threads/thread_pool_extension.cpp
            src/dlib/threads/async.cpp
            src/dlib/timer/timer.cpp
            src/dlib/stack_trace.cpp
            src/dlib/gui_widgets/fonts.cpp
            src/dlib/gui_widgets/widgets.cpp
            src/dlib/gui_widgets/drawable.cpp
            src/dlib/gui_widgets/canvas_drawing.cpp
            src/dlib/gui_widgets/style.cpp
            src/dlib/gui_widgets/base_widgets.cpp
            src/dlib/gui_core/gui_core_kernel_1.cpp
            src/dlib/gui_core/gui_core_kernel_2.cpp
   )

SET(GUI_SRC
  src/board_ui.cpp
  src/autogtpui.cpp)

if (NOT NO_GUI)
    set(GUI_SRC ${GUI_SRC} ${dlib_source_files})
endif()

ADD_EXECUTABLE(lzbot $<TARGET_OBJECTS:objs> ${GUI_SRC} src/play.cpp)
TARGET_LINK_LIBRARIES(lzbot ${needed_libraries} ${COMMON_LIBS})

