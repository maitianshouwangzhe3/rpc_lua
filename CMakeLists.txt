cmake_minimum_required(VERSION 3.14)

project(rpc_lua C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(LUA_DIR ${PROJECT_SOURCE_DIR}/3rd/lua)
set(LUNA_DIR ${PROJECT_SOURCE_DIR}/3rd/luna)

set(SRC_FILE_DIR ${PROJECT_SOURCE_DIR}/core)

set(RPC_LUA_SRC_FILES
    ${SRC_FILE_DIR}/socket.cc
    ${SRC_FILE_DIR}/server.cc
    ${SRC_FILE_DIR}/rpc_lua.cc
    ${SRC_FILE_DIR}/poller.cc
    ${SRC_FILE_DIR}/pb.c
    ${SRC_FILE_DIR}/connect_server.cc
    ${SRC_FILE_DIR}/connect_listen.cc
    ${SRC_FILE_DIR}/connect_client.cc
    ${SRC_FILE_DIR}/chain_buffer.cc
    ${SRC_FILE_DIR}/client.cc
    ${SRC_FILE_DIR}/lfs.c
)

add_library(rpc SHARED ${RPC_LUA_SRC_FILES})

set_target_properties(rpc PROPERTIES PREFIX "")

target_include_directories(rpc PRIVATE
    ${SRC_FILE_DIR}
    ${LUA_DIR}
    ${LUNA_DIR}
)

if(WIN32)
    target_compile_definitions(rpc PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _WIN32_WINNT=0x0601
    )
    target_link_libraries(rpc PRIVATE ws2_32 mswsock)
else()
    find_library(DL_LIB dl)
    if(DL_LIB)
        target_link_libraries(rpc PRIVATE ${DL_LIB})
    endif()
endif()

if(LUA_LIBRARIES)
    target_link_libraries(rpc PRIVATE ${LUA_LIBRARIES})
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
    set_target_properties(rpc PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endif()
