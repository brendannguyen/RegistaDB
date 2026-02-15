cmake_minimum_required(VERSION 3.10)
project(RegistaDB)

set(CMAKE_CXX_STANDARD 17)

# RocksDB usually requires these compression libraries on Linux
find_library(ROCKSDB_LIB rocksdb REQUIRED)
find_library(Z_LIB z REQUIRED)
find_library(BZ2_LIB bz2 REQUIRED)
find_library(LZ4_LIB lz4 REQUIRED)
find_library(ZSTD_LIB zstd REQUIRED)

add_executable(registadb_engine src/main.cpp)

# Link everything together
target_link_libraries(registadb_engine 
    PRIVATE 
    ${ROCKSDB_LIB} 
    ${Z_LIB} 
    ${BZ2_LIB} 
    ${LZ4_LIB} 
    ${ZSTD_LIB} 
    pthread   # Required for multi-threading in Linux
    dl        # Required for dynamic loading
)