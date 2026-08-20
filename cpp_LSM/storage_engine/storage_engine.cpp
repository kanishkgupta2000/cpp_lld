#include "storage_engine.h"
#include<thread>
#include<iostream>
#include <chrono>
namespace storage_engine{
        
    memtable::MemTableReadResult StorageEngine::read(std::string key)
    {
        // Take a snapshot of the current storage state.
        std::shared_ptr<memtable::MemTable> active;
        std::shared_ptr<memtable::MemTable> immutable;
        std::vector<sstable::SSTable> tables;
    
        {
            std::lock_guard<std::mutex> lock(mutex);
    
            active = active_memtable;
            immutable = immutable_memtable;
            tables = sstables;
        }
    
        // 1. Check active memtable
        std::cout<<"searching active_memtable for key: "<<key<<std::endl;
        auto active_result = active->read(key);
        if (active_result.status != memtable::NOT_FOUND)
        {
            return active_result;
        }
    
        // 2. Check immutable memtable if present
        std::cout<<"searching immutable_memtable for key: "<<key<<std::endl;

        if (immutable)
        {
            auto immutable_result = immutable->read(key);
    
            if (immutable_result.status != memtable::NOT_FOUND)
            {
                return immutable_result;
            }
        }
    
        std::cout<<"searching sstables for key: "<<key<<std::endl;

        // 3. Check SSTables from newest to oldest
        for (int i = tables.size() - 1; i >= 0; i--)
        {
            auto ss_table_result = tables[i].get_key(key);
    
            if (ss_table_result.key == key)
            {
                std::cout<<"found in ss_table: "<<tables[i].get_file_path()<<std::endl;
                return memtable::MemTableReadResult(
                    ss_table_result,
                    ss_table_result.is_deletion
                        ? memtable::DELETED
                        : memtable::FOUND
                );
            }
        }
    
        return memtable::MemTableReadResult(
            sstable::Record(),
            memtable::NOT_FOUND
        );
    }
    
    void StorageEngine::put(std::string key, std::string value)
    {
        std::shared_ptr<memtable::MemTable> immutable;

        {
            std::lock_guard<std::mutex> lock(memtable_mutex);

            active_memtable->put(key, value);

            if (!active_memtable->isFull())
            {
                return;
            }

            swapActiveMemtable();
            // Keep our own reference for the background thread.
            immutable = immutable_memtable;
        }
        flushSSTable(immutable);
    }
    
    void StorageEngine::delete_key(std::string key)
    {
        std::shared_ptr<memtable::MemTable> immutable;
        {
            std::lock_guard<std::mutex> lock(memtable_mutex);

            active_memtable->deleteKey(key);

            if (!active_memtable->isFull())
                return;

            swapActiveMemtable();
            // Keep our own reference for the background thread.
            immutable = immutable_memtable;
        }
        flushSSTable(immutable);
    }
}

void print_read_result(
    const std::string& source,
    const memtable::MemTableReadResult& result)
{
    std::cout << source << " read: ";

    switch (result.status)
    {
        case memtable::FOUND:
            std::cout << "FOUND"
                      << " key=" << result.record.key
                      << " value=" << result.record.value
                      << '\n';
            break;

        case memtable::DELETED:
            std::cout << "DELETED"
                      << " key=" << result.record.key
                      << '\n';
            break;

        case memtable::NOT_FOUND:
            std::cout << "NOT_FOUND\n";
            break;
    }
}

int main()
{
    storage_engine::StorageEngine engine("./ss_tables_folder", 8, 4);

    // Fill active MemTable
    engine.put("a", "apple");
    engine.put("b", "ball");
    engine.put("c", "cat");
    engine.put("d", "dog");
    engine.put("e", "elephant");
    engine.put("f", "football");
    engine.put("g", "game");
    engine.put("h", "hand");

    // Trigger rotation.
    // a-h -> immutable
    // new empty MemTable -> active
    engine.put("i", "india");

    // -----------------------------
    // Read from ACTIVE MemTable
    // -----------------------------
    auto active_result = engine.read("i");
    print_read_result("Active", active_result);


    // -----------------------------
    // Read from IMMUTABLE MemTable
    // -----------------------------
    auto immutable_result = engine.read("a");
    print_read_result("Immutable", immutable_result);


    // Add enough entries to trigger another rotation
    engine.put("j", "joker");
    engine.put("k", "king");
    engine.put("l", "lion");
    engine.put("m", "monkey");
    engine.put("n", "night");
    engine.put("o", "orange");
    engine.put("p", "python");
    engine.put("q", "queen");
    engine.put("r", "rabbit");


    // Wait for background SSTable flushes
    engine.wait_for_flushes();


    // -----------------------------
    // Read from SSTable
    // -----------------------------
    auto sstable_result = engine.read("a");
    print_read_result("SSTable", sstable_result);
}