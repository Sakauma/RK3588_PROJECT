# WinSim/CMakeLists.txt - RK3588 FPGA-preprocessed video encoder application

cmake_minimum_required(VERSION 3.10)
project(RK3588Test VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_RKMPP "Enable Rockchip MPP hardware encoder support" ON)
option(ENABLE_RGA "Search for Rockchip librga for future accelerated preprocessing" ON)
option(BUILD_OFFLINE_ENCODER_TEST "Build synthetic gray10le16/NV12-to-MPP encoder test" ON)
option(BUILD_FRAME_ASSEMBLER_SMOKE_TEST "Build PCIe packet-to-frame assembler smoke test" ON)

if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross compiling for ${CMAKE_SYSTEM_PROCESSOR}")
else()
    message(STATUS "Native build")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fpermissive")
if(CMAKE_CROSSCOMPILING)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

set(WINSIM_SOURCES
    ConsoleApplication1/ConsoleApplication1.cpp
    encoder/frame_assembler.cpp
    encoder/raw16_preprocess.cpp
    encoder/osd_overlay.cpp
    encoder/rk_mpp_encoder.cpp
    encoder/encoder_pipeline.cpp
)

add_executable(RK3588Test ${WINSIM_SOURCES})

set_target_properties(RK3588Test PROPERTIES
    OUTPUT_NAME "RK3588Test"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
)

target_include_directories(RK3588Test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ConsoleApplication1
    ${CMAKE_CURRENT_SOURCE_DIR}/ConsoleApplication1/shell
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder
)

set(RK3588API_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../RK3588API"
    CACHE PATH "RK3588API header directory")
set(RK3588API_LIBRARY
    "${CMAKE_CURRENT_SOURCE_DIR}/../libRK3588.so"
    CACHE FILEPATH "RK3588 SDK shared library")

if(EXISTS "${RK3588API_INCLUDE_DIR}")
    target_include_directories(RK3588Test PRIVATE "${RK3588API_INCLUDE_DIR}")
    message(STATUS "RK3588API include dir: ${RK3588API_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "RK3588API include dir not found: ${RK3588API_INCLUDE_DIR}")
endif()

if(EXISTS "${RK3588API_LIBRARY}")
    target_link_libraries(RK3588Test PRIVATE "${RK3588API_LIBRARY}")
    message(STATUS "RK3588API library: ${RK3588API_LIBRARY}")
else()
    message(FATAL_ERROR "RK3588API library not found: ${RK3588API_LIBRARY}")
endif()

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
target_link_libraries(RK3588Test PRIVATE Threads::Threads)

if(ENABLE_RKMPP)
    find_path(MPP_INCLUDE_DIR
        NAMES rk_mpi.h
        PATH_SUFFIXES rockchip)
    find_library(MPP_LIBRARY
        NAMES rockchip_mpp mpp)

    if(MPP_INCLUDE_DIR AND MPP_LIBRARY)
        target_compile_definitions(RK3588Test PRIVATE HAVE_RKMPP)
        target_include_directories(RK3588Test PRIVATE "${MPP_INCLUDE_DIR}")
        target_link_libraries(RK3588Test PRIVATE "${MPP_LIBRARY}")
        message(STATUS "Rockchip MPP include: ${MPP_INCLUDE_DIR}")
        message(STATUS "Rockchip MPP library: ${MPP_LIBRARY}")
    else()
        message(WARNING "Rockchip MPP not found; encoder builds with runtime stub only")
    endif()
endif()

if(ENABLE_RGA)
    find_path(RGA_INCLUDE_DIR
        NAMES rga.h
        PATH_SUFFIXES rga rockchip)
    find_library(RGA_LIBRARY
        NAMES rga)

    if(RGA_INCLUDE_DIR AND RGA_LIBRARY)
        target_compile_definitions(RK3588Test PRIVATE HAVE_RGA)
        target_include_directories(RK3588Test PRIVATE "${RGA_INCLUDE_DIR}")
        target_link_libraries(RK3588Test PRIVATE "${RGA_LIBRARY}")
        message(STATUS "Rockchip RGA include: ${RGA_INCLUDE_DIR}")
        message(STATUS "Rockchip RGA library: ${RGA_LIBRARY}")
    else()
        message(STATUS "Rockchip RGA not found; CPU grayscale/NV12 input preparation will be used")
    endif()
endif()

if(BUILD_OFFLINE_ENCODER_TEST)
    add_executable(offline_encode_test
        tools/offline_encode_test.cpp
        encoder/raw16_preprocess.cpp
        encoder/osd_overlay.cpp
        encoder/rk_mpp_encoder.cpp
    )
    target_include_directories(offline_encode_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/encoder
    )
    if(MPP_INCLUDE_DIR AND MPP_LIBRARY)
        target_compile_definitions(offline_encode_test PRIVATE HAVE_RKMPP)
        target_include_directories(offline_encode_test PRIVATE "${MPP_INCLUDE_DIR}")
        target_link_libraries(offline_encode_test PRIVATE "${MPP_LIBRARY}")
    endif()
endif()

if(BUILD_FRAME_ASSEMBLER_SMOKE_TEST)
    add_executable(frame_assembler_smoke_test
        tools/frame_assembler_smoke_test.cpp
        encoder/frame_assembler.cpp
    )
    target_include_directories(frame_assembler_smoke_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/encoder
    )
endif()
