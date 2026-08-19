# Memtable basics

1. When server starts, memtable instance is created. (Or recovered from a crash using Write log)
2. All writes added to this memtable(in-memory), along with entry in the write log.
3. When a certain memtable threshold is reached. We start writing the memtable into a new segment (a fresh SSTable file)
4. As soon as we start writing the memtable to disk, a new memtable starts taking writes.
5. Once the memtable writing is complete, we free the original memtable's memory.
6. In background merging and compacting process