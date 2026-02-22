cmake_minimum_required(VERSION 3.10)
project(RegistaDB)

set(CMAKE_CXX_STANDARD 17)

# Find RocksDB and Compression
find_library(ROCKSDB_LIB rocksdb REQUIRED)
find_library(Z_LIB z REQUIRED)
find_library(BZ2_LIB bz2 REQUIRED)
find_library(LZ4_LIB lz4 REQUIRED)
find_library(ZSTD_LIB zstd REQUIRED)
find_library(SNAPPY_LIB snappy REQUIRED)

# Find Protobuf & ZeroMQ
find_package(Protobuf REQUIRED)
find_library(ZMQ_LIB zmq REQUIRED)

# Find prometheus-cpp
find_package(prometheus-cpp CONFIG REQUIRED)

# Where to find your .proto file and where to put the generated code
set(PROTO_SRC "../proto/playbook.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC})

# Add the generated files to your executable
add_executable(registadb_engine 
    src/main.cpp
    src/StorageManager.cpp 
    src/RegistaServer.cpp
    src/MetricsExporter.cpp 
    ${PROTO_SRCS} 
    ${PROTO_HDRS}
)

# Include directories for the generated files
target_include_directories(registadb_engine PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link Protobuf along with RocksDB
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
    prometheus-cpp::core
    prometheus-cpp::pull
    pthread 
    dl
)

# --- GOOGLE TEST SETUP ---

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Create the test executable
add_executable(regista_tests 
    tests/unit/storage_test.cpp 
    tests/unit/regista_test.cpp
    src/StorageManager.cpp
    src/RegistaServer.cpp
    src/MetricsExporter.cpp 
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

target_include_directories(regista_tests PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link tests against GTest, RocksDB, and Protobuf
target_link_libraries(regista_tests PRIVATE
    gtest_main 
    ${ROCKSDB_LIB}
    ${Z_LIB} ${BZ2_LIB} ${LZ4_LIB} ${ZSTD_LIB} ${SNAPPY_LIB}
    ${Protobuf_LIBRARIES} ${ZMQ_LIB}
    prometheus-cpp::core
    prometheus-cpp::pull
    pthread dl
)

# This enables the 'make test' command and automatic test discovery
include(GoogleTest)
gtest_discover_tests(regista_tests)