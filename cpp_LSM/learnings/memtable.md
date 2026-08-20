Memtable
1. Service startups
2. First looks at the read logs to identify a crash mid memory
3. Now memtable is restored or new is created
4. We start writing to memtable (in-memory)
5. We start reading (into the in-memory map)
6. Memtable size is reached -->
    a. We perform sstable writing to a new segment. 
    b. This memtable now does not take anymore writes, but still may take reads from in-memory map.
    c. when the write is complete, the pointer to the storage engine should be utilized to free the passive_memtable.


Storage Engine  
1. Picks the last segment name from a folder of sstables or a counter essentially
2. Creates active_memtable (pass the new file_name)
3. Creates background_memtable
4. When active_memtable reaches the capacity, we need to switch the reference to background_memtable, create a new thread and led the write happen there. we need to have a signal which will free the pointer of passive_memtable as soon as sstable writing completes. (we need to pass the reference of storage engine so that we can set reset the passive pointer).
5. For reads: 
    a. first checks the (in-memory) map of active_memtable.
    b. then checks the passive_memtable for the memory.
    c. then go ahead with reading older sstables.

