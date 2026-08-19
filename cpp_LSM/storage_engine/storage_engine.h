#ifndef INCLUDED_STORAGE_ENGINE
#define INCLUDED_STORAGE_ENGINE

#include "../memtable/memtable.h"
#include "../sstable/sstable.h"

#include <iostream>
#include <memory>
namespace storage_engine{
    class StorageEngine {
        private:
            StorageEngine(std::string ss_table_folder_path) : m_sstable_folder(ss_table_folder_path), sstables()
            {
                load_ss_tables();
                load_startup_memtables();

            }
            std::shared_ptr<memtable::MemTable> active_memtable;
            std::shared_ptr<memtable::MemTable> immutable_memtable;
        
            std::vector<sstable::SSTable> sstables;
        
            std::mutex memtable_mutex;

            private:
            std::string m_sstable_folder;
            void load_ss_tables()
            {

            }

            std::string get_candidate_file_path()
            {
                // reads the ss_table_folder_path, has some segment table format parsing and autoincrement logic
                // for example data_1.sstable etc.
            }
            void load_startup_memtables()
            {
                // use recovery logs to build memtable or fresh one.
                // for now building fresh mem_table
                active_memtable = std::make_shared<memtable::MemTable>(get_candidate_file_path(), 6);
                immutable_memtable = nullptr;
            }
            
        };
}
#endif