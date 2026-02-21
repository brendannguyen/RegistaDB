# RegistaDB
<img width="150" height="150" alt="registaDB logo" src="https://github.com/user-attachments/assets/5e3c8d1b-c3a1-414e-bc03-6bddf0ecc30c" />

A high-performance C++ middleware engine/DBMS using RocksDB to orchestrate data ingestion streams from applications.

## Features

- Uses RocksDB for database storage on SSD.
- Create, Read, Update, Delete
- Index and data column families using reversed big-endian keys (16-byte primary composite key, 8-byte index key)
- Two tunnels: performance & smart (PUSH/PULL & REQ/REP)
- Smart tunnel for verified ingest, read and delete
- Performance tunnel for non verified ingest
- Supports basic scalar values, basic lists, string maps, json and bytes.

### Proposed Features

- docker-compose deployment
- Dashboard (Grafana + Prometheus) + control panel
- RAM optimised mode
- Batching mode
- REST support
- Scalability support
- Small app/backend example

## Guide

### JAVA Client Usage

#### Creating entries:

```
client.create(value);
client.create(id, value);
client.create(value, metadata);
client.create(id, value, metadata);
```

```
int testId = 999;
String content = "Hello RegistaDB!";
EntryValue value = EntryValueBuilder.ofString(content);

Response createResp = client.create(testId, value);
```

```
int testId = 1000;
String content = "This is a non-verified ingest test.";
var value = EntryValueBuilder.ofString(content);

client.createNoReply(testId, value);
```

#### Reading entries:

```
Response postIngestResp = client.read(testId);
EntryValue storedValue = postIngestResp.getEntry().getData();
EntryValueReader.read(storedValue)
```

#### Updating entries:

```
Entry entry = new EntryBuilder()
        .setId(id) # id to update
        .setMetadata(metadata)
        .setValue(value)
        .build();

Response updateResp = client.update(entry)
```

#### Deleting entries:

```
Response deleteResp = client.delete(testId);
```

### Setup RocksDB & RegistaDB Engine

1. Install dependencies

```
sudo apt update
sudo apt install librocksdb-dev libsnappy-dev libzstd-dev liblz4-dev libbz2-dev zlib1g-dev rocksdb-tools
sudo apt install build-essential gdb
sudo apt install protobuf-compiler
sudo apt install libzmq3-dev
```

2. Create C++ build files

```
mkdir data
mkdir regista_db/build
cd regista_db/build

cmake ..
make
```

3. Run the engine

```
./registadb_engine
```

### Setup java client

1. Install dependenices

```
sudo apt install maven
```

2. Compile

```
cd java_client
mvn compile
```

3. Run Java Producer

```
mvn exec:java -Dexec.mainClass="com.registadb.Producer"
mvn exec:java -Dexec.mainClass="com.registadb.Producer" -Dexec.args="500"
```

## Development Guide

### Playbook changes (proto)

1. Rebuild registaDB engine

```
cd regista_db/build
cmake ..
make
```

### Engine Changes (src, c++)

1. Compile & run

```
cd regista_db/build
make && ./registadb_engine
```

### Client changes (java)

1. Compile

```
cd java_client
mvn compile
```

2. Run Producer

```
mvn exec:java -Dexec.mainClass="com.registadb.Producer"
```

## Testing

### C++ Testing

1. Compile (if not already)

```
cd regista_db/build
make
```

2. Run tests

```
./regista_tests
```

### Java Testing (RegistaDB Server)

1. Run JUnit Tests

```
cd java_client

mvn test
```