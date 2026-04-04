cmake_minimum_required(VERSION 3.14)

project(rpc_lua)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(LUA_DIR ${PROJECT_SOURCE_DIR}/3rd/lua)
set(LUA_INC ${LUA_DIR})
set(LUA_LIBPATH ${LUA_INC})

set(LUNA_DIR ${PROJECT_SOURCE_DIR}/3rd/luna)
set(LUNA_INC ${LUNA_DIR})
set(LUNA_LIBPATH ${LUNA_DIR})

set(RPC_LUA_INC_BASE ${PROJECT_SOURCE_DIR}/core)

# aux_source_directory(${PROJECT_SOURCE_DIR}/core RPC_LUA_SRC_FILES)
set(SRC_FILE_DIR ${PROJECT_SOURCE_DIR}/core)

set(
    RPC_LUA_SRC_FILES
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

set(
    RPC_LUA_CORE
    ${RPC_LUA_SRC_FILES}
)

set(
    RPC_LUA_INC
    ${RPC_LUA_INC_BASE}
    ${LUA_INC}
    ${LUNA_INC}
)

add_library(rpc SHARED ${RPC_LUA_SRC_FILES})
# add_executable(rpc ${RPC_LUA_SRC_FILES})
add_dependencies(rpc lua_ext luna_ext)
set_target_properties(rpc PROPERTIES PREFIX "")
target_include_directories(rpc PRIVATE ${RPC_LUA_INC})
target_link_libraries(rpc PRIVATE ${LUA_LIBPATH}/liblua.a ${LUNA_LIBPATH}/libluna.a)