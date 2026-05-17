# WinSim/CMakeLists.txt - RK3588 V7 PCIe/SRIO live camera test

cmake_minimum_required(VERSION 3.10)
project(RK3588Test VERSION 1.0.0 LANGUAGES C CXX)

# 检测是否为交叉编译
if(CMAKE_CROSSCOMPILING)
    message(STATUS "交叉编译模式 - 目标架构: ${CMAKE_SYSTEM_PROCESSOR}")
    set(TARGET_ARCH "arm64")
else()
    message(STATUS "本地编译模式")
    set(TARGET_ARCH "x86_64")
endif()

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fpermissive")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

# 在交叉编译时可能需要额外的标志
if(CMAKE_CROSSCOMPILING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

# 源文件
set(WINSIM_SOURCES
    ConsoleApplication1/ConsoleApplication1.cpp
)

# 创建可执行文件
add_executable(RK3588Test ${WINSIM_SOURCES})

# 设置运行时库搜索路径为可执行文件所在目录
set_target_properties(RK3588Test PROPERTIES
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
)


# 包含目录
target_include_directories(RK3588Test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ConsoleApplication1
    ${CMAKE_CURRENT_SOURCE_DIR}/ConsoleApplication1/shell
)

# RK3588API头文件路径，默认使用当前SDK目录内的RK3588API
set(RK3588API_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../RK3588API CACHE PATH "RK3588API头文件目录")
if(EXISTS ${RK3588API_INCLUDE_DIR})
    target_include_directories(RK3588Test PRIVATE ${RK3588API_INCLUDE_DIR})
    message(STATUS "找到RK3588API头文件目录: ${RK3588API_INCLUDE_DIR}")
else()
    message(WARNING "RK3588API头文件目录不存在: ${RK3588API_INCLUDE_DIR}")
endif()

# 查找pthread和libjpeg库
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# HTTP MJPEG输出使用libjpeg做灰度JPEG编码，不再依赖opencv-mobile/httpjpg。
find_package(JPEG QUIET)
if(NOT JPEG_FOUND)
    find_library(JPEG_LIBRARY NAMES jpeg)
    if(NOT JPEG_LIBRARY)
        message(FATAL_ERROR "libjpeg not found. Install libjpeg-dev on RK3588.")
    endif()
    set(JPEG_LIBS ${JPEG_LIBRARY})
else()
    if(TARGET JPEG::JPEG)
        set(JPEG_LIBS JPEG::JPEG)
    else()
        set(JPEG_LIBS ${JPEG_LIBRARIES})
        target_include_directories(RK3588Test PRIVATE ${JPEG_INCLUDE_DIRS})
    endif()
endif()

# RK3588库路径，默认使用当前SDK目录内的libRK3588.so
set(RK3588_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../libRK3588.so CACHE FILEPATH "RK3588库文件路径")

if(EXISTS ${RK3588_LIB_PATH})
    message(STATUS "找到RK3588库: ${RK3588_LIB_PATH}")
    target_link_libraries(RK3588Test PRIVATE ${RK3588_LIB_PATH} ${JPEG_LIBS} Threads::Threads)
else()
    # 如果库不存在，给出明确的编译指令
    message(FATAL_ERROR "
    RK3588库不存在: ${RK3588_LIB_PATH}

    请先确认RK3588库存在:

    1. 确认库文件已生成或拷贝到SDK目录:
       ls -la ${RK3588_LIB_PATH}

    2. 然后再重新编译测试程序:
       cd WinSim/build_arm64
       make clean && cmake .. && make
    ")
endif()

# 设置输出属性
set_target_properties(RK3588Test PROPERTIES
    OUTPUT_NAME "RK3588Test"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 安装规则（可选）
# install(TARGETS RK3588Test DESTINATION bin)
