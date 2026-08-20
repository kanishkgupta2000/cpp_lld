#include "storage_engine.h"
#include<thread>
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
        auto active_result = active->read(key);
        if (active_result.status != memtable::NOT_FOUND)
        {
            return active_result;
        }
    
        // 2. Check immutable memtable if present
        if (immutable)
        {
            auto immutable_result = immutable->read(key);
    
            if (immutable_result.status != memtable::NOT_FOUND)
            {
                return immutable_result;
            }
        }
    
        // 3. Check SSTables from newest to oldest
        for (int i = tables.size() - 1; i >= 0; i--)
        {
            auto ss_table_result = tables[i].get_key(key);
    
            if (ss_table_result.key == key)
            {
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