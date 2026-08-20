```cpp 
g++ -std=c++23 \      
    storage_engine.cpp \
    ../memtable/memtable.cpp \
    -I../memtable/memtable.h \
    -I../sstable/sstable.h \
    -o storage_engine_write
```