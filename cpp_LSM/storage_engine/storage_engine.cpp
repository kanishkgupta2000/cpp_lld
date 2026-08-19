#include "storage_engine.h"
#include<thread>
namespace storage_engine{
        
    memtable::MemTableReadResult StorageEngine::read(std::string key)
    {
        // check active memtable
        // check immutable memtable if present 
        // start checking sstables recursively down to the last one
    }
    void StorageEngine::put(std::string key, std::string value)
    {
        std::shared_ptr<memtable::MemTable> immutable;

        {
            std::lock_guard<std::mutex> lock(memtable_mutex);

            active_memtable->put(key, value);

            if (!active_memtable->isFull())
                return;

            // Move the full active memtable into the immutable slot.
            immutable_memtable = active_memtable;

            // Create a fresh active memtable for new writes.
            active_memtable =
                std::make_shared<memtable::MemTable>(
                    get_candidate_file_path(),
                    m_data_size,
                    m_sstable_block_size
                );

            // Keep our own reference for the background thread.
            immutable = immutable_memtable;
        }

        // Don't hold the mutex while doing disk I/O.
        std::thread([this, immutable]() {
            immutable->writeSSTable();

            std::lock_guard<std::mutex> lock(memtable_mutex);

            // Only clear it if this is still the immutable memtable
            // that this thread flushed.
            if (immutable_memtable == immutable)
                immutable_memtable.reset();
        }).detach();
    }
    
    void StorageEngine::delete_key(std::string key)
    {
        std::shared_ptr<memtable::MemTable> immutable;
        {
            std::lock_guard<std::mutex> lock(memtable_mutex);

            active_memtable->deleteKey(key);

            if (!active_memtable->isFull())
                return;

            // Move the full active memtable into the immutable slot.
            immutable_memtable = active_memtable;

            // Create a fresh active memtable for new writes.
            active_memtable =
                std::make_shared<memtable::MemTable>(
                    get_candidate_file_path(),
                    m_data_size,
                    m_sstable_block_size
                );

            // Keep our own reference for the background thread.
            immutable = immutable_memtable;
        }

        // Don't hold the mutex while doing disk I/O.
        std::thread([this, immutable]() {
            immutable->writeSSTable();

            std::lock_guard<std::mutex> lock(memtable_mutex);

            // Only clear it if this is still the immutable memtable
            // that this thread flushed.
            if (immutable_memtable == immutable)
                immutable_memtable.reset();
        }).detach();
    }

}

int main()
{

}