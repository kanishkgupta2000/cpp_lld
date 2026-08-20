# LSM Storage Engine

A lightweight Log-Structured Merge (LSM) based storage engine written in C++.

This project is being built from scratch to understand how modern storage engines work internally, with particular focus on:

- Log-structured storage
- MemTables
- SSTables
- Concurrent reads/writes
- Background flushing
- Crash recovery
- Read optimization
- Compaction

The design is inspired by concepts from *Designing Data-Intensive Applications (DDIA)* and real-world LSM-based databases.

---

## Current Architecture

At a high level, writes flow through the following pipeline:

```text
                    PUT / DELETE
                         |
                         v
                  +-------------+
                  | Active      |
                  | MemTable    |
                  +-------------+
                         |
                    MemTable full
                         |
                         v
                  +-------------+
                  | Immutable   |
                  | MemTable    |
                  +-------------+
                         |
                  Background Flush
                         |
                         v
                  +-------------+
                  |   SSTable   |
                  +-------------+
                         |
                         v
                     Disk
````

Reads search the newest data first:

```text
                 READ(key)
                    |
                    v
            +---------------+
            | Active        |
            | MemTable      |
            +---------------+
                    |
               NOT_FOUND
                    |
                    v
            +---------------+
            | Immutable     |
            | MemTable      |
            +---------------+
                    |
               NOT_FOUND
                    |
                    v
            +---------------+
            | SSTables      |
            | Newest → Oldest|
            +---------------+
                    |
               NOT_FOUND
                    |
                    v
                  NULL
```

---

# Implemented

## 1. MemTable

The in-memory write buffer is implemented as a MemTable.

Current functionality includes:

* `PUT`
* `DELETE`
* `READ`
* Tracking MemTable size
* MemTable rotation when capacity is reached

When the active MemTable reaches its configured limit, it is converted into an immutable MemTable and a new active MemTable is created.

---

## 2. Immutable MemTable

When the active MemTable is rotated:

```text
Active MemTable
       |
       v
Immutable MemTable
       |
       +---- background SSTable flush
```

The immutable MemTable is kept available for reads while it is being flushed to disk.

This prevents reads from being blocked while disk I/O is occurring.

---

## 3. SSTable Writer

MemTables can be serialized into SSTables on disk.

SSTables currently contain:

* Records
* Keys
* Values
* Deletion markers
* Sparse index information

SSTables are written asynchronously using background threads.

---

## 4. SSTable Reader

SSTables can be loaded from disk and queried.

The reader uses the sparse index to avoid scanning the entire SSTable.

Current lookup flow:

```text
SSTable
   |
   v
Sparse Index
   |
   v
Locate candidate region
   |
   v
Read records
   |
   v
Find key
```

---

## 5. Sparse Index

A sparse index is maintained for SSTables to reduce the amount of data that needs to be scanned during a lookup.

The reader uses binary search over the sparse index to locate the appropriate region of the SSTable.

---

## 6. Tombstones

Deletes are represented using deletion records rather than physically removing data immediately.

For example:

```text
PUT("a", "apple")

DELETE("a")
```

results in a tombstone-like record:

```text
key = a
is_deletion = true
```

This allows newer data to override older values in SSTables.

Physical removal of obsolete records will eventually be handled during compaction.

---

## 7. Multiple SSTables

The StorageEngine maintains multiple SSTables.

Reads search them from newest to oldest so that newer values take precedence over older values.

```text
SSTable N       ← newest
SSTable N - 1
SSTable N - 2
...
SSTable 1       ← oldest
```

---

## 8. Asynchronous SSTable Flushing

SSTable writes happen in background threads.

The StorageEngine maintains ownership of these threads and joins them during destruction.

This allows the foreground write path to continue while an immutable MemTable is being written to disk.

Current model:

```text
PUT
 |
 +-- MemTable rotation
 |
 +-- Start background flush
 |
 +-- Return immediately
 |
 +----------------------+
                        |
                 background thread
                        |
                        v
                  write SSTable
```

---

## 9. SSTable File Naming

SSTables use monotonically increasing identifiers:

```text
data_000001.sst
data_000002.sst
data_000003.sst
...
```

The next SSTable identifier is determined from the existing SSTables during startup.

The identifier is allocated when a MemTable becomes immutable, rather than when the background flush actually begins.

This prevents multiple concurrent flushes from selecting the same filename.

---

## 10. Concurrent Reads and Writes

The StorageEngine currently uses mutexes to protect shared state involved in MemTable rotation and SSTable management.

The goal is to avoid holding locks during expensive disk I/O.

For example:

```cpp
// Lock
// Swap active -> immutable
// Unlock

// Disk I/O happens without the lock

immutable->writeSSTable();
```

This allows other operations to continue while an SSTable is being written.

---

# Example

A basic usage example:

```cpp
storage_engine::StorageEngine engine(
    "./ss_tables_folder",
    8,
    4
);

engine.put("a", "apple");
engine.put("b", "ball");
engine.put("c", "cat");

auto result = engine.read("a");

if (result.status == memtable::FOUND)
{
    std::cout << result.record.value << '\n';
}
```

---

# Roadmap

The following components are planned.

## 1. Write-Ahead Log (WAL)

### Status: Planned

Currently, data stored only in the MemTable can be lost if the process crashes before the MemTable is flushed to an SSTable.

The WAL will provide crash recovery.

Write path will become:

```text
PUT
 |
 v
WAL
 |
 v
MemTable
 |
 v
Background SSTable Flush
```

On startup:

```text
WAL
 |
 v
Replay operations
 |
 v
Reconstruct MemTable
```

The WAL should support:

* Append-only writes
* PUT records
* DELETE records
* Recovery after crash
* WAL rotation
* Truncation/deletion after successful SSTable persistence

---

## 2. Bloom Filters

### Status: Planned

Currently, a lookup may require checking multiple SSTables:

```text
SSTable 5
   ↓
SSTable 4
   ↓
SSTable 3
   ↓
SSTable 2
   ↓
SSTable 1
```

Bloom filters will allow the engine to quickly determine whether an SSTable **definitely does not contain a key**.

```text
READ(key)
   |
   v
Bloom Filter
   |
   +---- Definitely absent → skip SSTable
   |
   +---- Maybe present → search SSTable
```

This should significantly reduce unnecessary disk reads as the number of SSTables grows.

---

## 3. Concurrency Correctness

### Status: In Progress

The current implementation introduces multiple concurrent operations and therefore needs further synchronization analysis.

Areas to investigate:

* Concurrent reads vs SSTable insertion
* Concurrent MemTable rotation
* Multiple simultaneous flushes
* Ordering of completed SSTables
* Lifetime of immutable MemTables
* `shared_ptr` ownership
* Thread-safe access to `sstables`
* Thread-safe SSTable ID allocation
* Shutdown while background flushes are running
* Avoiding deadlocks
* Avoiding data races

A particularly important issue is that flush completion order may differ from SSTable creation order:

```text
Flush A → data_000004.sst
Flush B → data_000005.sst

B finishes first
A finishes later
```

Naively doing:

```cpp
sstables.push_back(...)
```

on completion could produce:

```text
data_000005
data_000004
```

even though `000004` is older.

The SSTable ordering invariant must therefore be explicitly maintained.

---

## 4. Compaction

### Status: Planned

The number of SSTables will continuously increase without compaction.

Compaction will:

* Merge SSTables
* Remove overwritten values
* Remove obsolete tombstones
* Reduce the number of SSTables
* Improve read performance
* Reclaim disk space

Eventually the architecture should look like:

```text
SSTable 1 ─┐
SSTable 2 ─┤
SSTable 3 ─┼──> Compaction ──> New SSTable
SSTable 4 ─┤
SSTable 5 ─┘
```

A leveled or size-tiered compaction strategy can be evaluated later.

---

## 5. Manifest / Metadata Management

### Status: Planned

The engine currently discovers SSTables by scanning the SSTable directory.

A metadata/manifest file should eventually track:

* SSTables belonging to the database
* SSTable IDs
* File paths
* Compaction state
* Database version/state

This becomes especially important when compaction is introduced.

---

## 6. Atomic SSTable Creation

### Status: Planned

SSTable writes should eventually use temporary files:

```text
data_000006.sst.tmp
        |
        v
complete write
        |
        v
atomic rename
        |
        v
data_000006.sst
```

This prevents partially-written SSTables from being treated as valid files after a crash.

---

## 7. Checksums

### Status: Planned

Checksums should be added to SSTable blocks and/or records.

This will allow the engine to detect:

* Corrupted records
* Partial writes
* Disk corruption
* Invalid SSTables

---

## 8. Block-Based SSTables

### Status: Planned

The current SSTable implementation is intentionally simple.

A more scalable design would organize records into blocks:

```text
SSTable

+----------------+
| Data Block     |
+----------------+
| Data Block     |
+----------------+
| Data Block     |
+----------------+
| Sparse Index   |
+----------------+
| Bloom Filter   |
+----------------+
| Metadata       |
+----------------+
```

This will make it easier to introduce:

* Block caching
* Compression
* Bloom filters
* Efficient random reads

---

## 9. Compression

### Status: Planned

Compression can be introduced at the SSTable block level.

Potential options include:

* Snappy
* LZ4
* Zstandard

The goal is to reduce disk usage and potentially improve I/O throughput.

---

## 10. Benchmarking

### Status: Planned

The engine should eventually have benchmarks for:

* Sequential writes
* Random writes
* Sequential reads
* Random reads
* Read-after-write
* Delete workloads
* Large datasets
* Different MemTable sizes
* Different SSTable sizes
* Bloom filter effectiveness
* Compaction overhead

Metrics should include:

```text
Throughput
Latency
p50
p95
p99
Disk usage
Memory usage
CPU usage
```

---

# Long-Term Architecture

The eventual storage engine is intended to evolve toward:

```text
                         Client
                           |
                           v
                    +-------------+
                    | Storage     |
                    | Engine      |
                    +-------------+
                           |
              +------------+------------+
              |                         |
              v                         v
            WAL                    MemTable
              |                         |
              |                    MemTable Full
              |                         |
              |                         v
              |                   Immutable
              |                         |
              |                  Background Flush
              |                         |
              |                         v
              |                    +---------+
              +------------------->| SSTable |
                                   +---------+
                                        |
                              +---------+---------+
                              |         |         |
                              v         v         v
                            Bloom     Index     Data
                            Filter
                                        |
                                        v
                                   Compaction
```

---

# Learning Goals

This project is not intended to compete with production databases.

The primary goal is to understand the engineering problems behind data-intensive storage systems.

Key concepts being explored:

* LSM trees
* Write amplification
* Read amplification
* Space amplification
* Crash recovery
* Durability
* Concurrency
* Locking
* Background work
* Immutable data structures
* File formats
* Indexing
* Bloom filters
* Compaction
* Storage-engine performance

---

# Current Status

### Implemented

* [x] MemTable
* [x] PUT
* [x] DELETE
* [x] READ
* [x] Immutable MemTable
* [x] MemTable rotation
* [x] SSTable writer
* [x] SSTable reader
* [x] Sparse index
* [x] Tombstones
* [x] Multiple SSTables
* [x] SSTable discovery on startup
* [x] Asynchronous SSTable flushing
* [x] Background flush thread ownership
* [x] Basic concurrency protection

### In Progress / Planned

* [ ] Thorough concurrency correctness
* [ ] Write-Ahead Log
* [ ] Crash recovery
* [ ] Bloom filters
* [ ] Compaction
* [ ] Manifest / metadata management
* [ ] Atomic SSTable creation
* [ ] Checksums
* [ ] Block-based SSTables
* [ ] Compression
* [ ] Block cache
* [ ] Benchmarks
* [ ] Fault-injection testing
* [ ] Performance tuning

---

# Project Philosophy

The implementation is intentionally incremental.

Instead of trying to build a complete database immediately, the project is being developed by first implementing the simplest working version and then introducing the problems that appear at scale:

```text
Simple storage
      ↓
Persistence
      ↓
Concurrency
      ↓
Crash recovery
      ↓
Read optimization
      ↓
Compaction
      ↓
Performance
      ↓
Production-style reliability
```

The goal is to understand **why** each component exists, not just how to implement it.

```
