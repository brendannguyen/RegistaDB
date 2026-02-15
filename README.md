# RegistaDB
A high-performance C++ middleware engine using RocksDB to orchestrate data ingestion streams from Java applications.


## Guide

### Setup RocksDB

1. Install dependencies

```
sudo apt update
sudo apt install librocksdb-dev libsnappy-dev libzstd-dev liblz4-dev libbz2-dev zlib1g-dev
sudo apt install build-essential gdb
```

2. Create C++ build files

```
mkdir regista_db/build
mkdir regista_db/data
cd regista_db/build

cmake ..
make
```

3. Run the engine

```
./registadb_engine
```