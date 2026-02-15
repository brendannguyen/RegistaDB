cmake_minimum_required(VERSION 3.10)
project(RegistaDB)

set(CMAKE_CXX_STANDARD 17)

# 1. Find RocksDB and Compression (Keeping your previous working setup)
find_library(ROCKSDB_LIB rocksdb REQUIRED)
find_library(Z_LIB z REQUIRED)
find_library(BZ2_LIB bz2 REQUIRED)
find_library(LZ4_LIB lz4 REQUIRED)
find_library(ZSTD_LIB zstd REQUIRED)
find_library(SNAPPY_LIB snappy REQUIRED)

# 2. Find Protobuf
find_package(Protobuf REQUIRED)

# 2b. Find ZeroMQ
find_library(ZMQ_LIB zmq REQUIRED)

# 3. Tell CMake where to find your .proto file and where to put the generated code
set(PROTO_SRC "../proto/playbook.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC})

# 4. Add the generated files to your executable
add_executable(registadb_engine 
    src/main.cpp
    src/StorageManager.cpp 
    src/RegistaServer.cpp 
    ${PROTO_SRCS} 
    ${PROTO_HDRS}
)

# 5. Include directories for the generated files
target_include_directories(registadb_engine PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# 6. Link Protobuf along with RocksDB
target_link_libraries(registadb_engine 
    PRIVATE 
    ${ROCKSDB_LIB}
    ${Z_LIB}
    ${BZ2_LIB}
    ${LZ4_LIB}
    ${ZSTD_LIB}
    ${SNAPPY_LIB}
    ${Protobuf_LIBRARIES}
    ${ZMQ_LIB}
    pthread 
    dl
)