cmake_minimum_required(VERSION 3.12)
project(compiler)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CPP_COMPILER /usr/bin/clang++)

include_directories(.)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/backend/inc)
include_directories(${CMAKE_SOURCE_DIR}/interface)
include_directories(${local}/include/C-Container)

file(GLOB_RECURSE DIR_SRC "src/*.c")
file(GLOB_RECURSE DIR_BACKEND_SRC "backend/src/*.c")
file(GLOB_RECURSE DIR_INTERFACE_SRC "interface/*.c")
file(GLOB_RECURSE DIR_STRUCT_DEMO "data_struct_demo/*.c")
#包含的头文件目录

find_package(BISON)
BISON_TARGET(MyParser gramtree.y ${CMAKE_CURRENT_BINARY_DIR}/gramtree.tab.c)

find_package(FLEX)
FLEX_TARGET(MyScanner gramtree.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)

#设置可执行文件的输出目录
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

#设置动态链接库的库文件的目录
link_directories(${PROJECT_SOURCE_DIR}/lib)

#添加编译选项
add_compile_options(-g)

#生成可执行文件
add_executable(${PROJECT_NAME} main.c  ${BISON_MyParser_OUTPUTS} ${FLEX_MyScanner_OUTPUTS}  ${DIR_SRC} ${DIR_BACKEND_SRC} ${DIR_STRUCT_DEMO} ${DIR_INTERFACE_SRC})
  

#链接动态库文件
target_link_libraries(${PROJECT_NAME} libcds.so libm.so)
