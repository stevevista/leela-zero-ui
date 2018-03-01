
include_directories(../)

file(GLOB SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
  ./*.cpp)

list(REMOVE_ITEM SOURCES ./gendata.cpp)
list(REMOVE_ITEM SOURCES ./train.cpp)
list(REMOVE_ITEM SOURCES ./verify.cpp)

add_library(train_data STATIC ${SOURCES})
if (NOT MSVC)
  target_compile_options(train_data PRIVATE "-std=c++11" "-Wall" "-Wextra")
endif()

add_executable(gendata ./gendata.cpp)
target_link_libraries(gendata train_data)
target_compile_options(gendata PRIVATE "-std=c++11" "-Wall" "-Wextra")
