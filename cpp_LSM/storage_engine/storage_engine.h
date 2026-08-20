#ifndef INCLUDED_STORAGE_ENGINE
#define INCLUDED_STORAGE_ENGINE

#include "../memtable/memtable.h"
#include "../sstable/sstable.h"

#include <iostream>
#include <memory>
#include<filesystem>
#include<vector>
#include <algorithm>
#include <regex>
#include <format>
#include <thread>
#include <mutex>

namespace storage_engine{
    class StorageEngine {
            public:
            StorageEngine(std::string ss_table_folder_path, uint32_t data_size, uint32_t sstable_block_size) : m_sstable_folder(ss_table_folder_path),
            sstables(), m_data_size(data_size), m_sstable_block_size(sstable_block_size), next_sstable_id(get_last_sstable_fileid())
            {
                load_ss_tables(); // sstables loaded
                load_startup_memtables();  // memtables created with latest file
            }

            ~StorageEngine()
            {
                for (auto& thread : flush_threads)
                {
                    if (thread.joinable())
                        thread.join();
                }
            }

            memtable::MemTableReadResult read(std::string key);
            void put(std::string key, std::string value);
            void delete_key(std::string key);

            void wait_for_flushes()
            {
                for (auto& thread : flush_threads)
                {
                    if (thread.joinable())
                        thread.join();
                }
            }

        private:
            std::vector<std::thread> flush_threads;
            uint32_t m_sstable_block_size;
            uint32_t m_data_size;
            std::string m_sstable_folder;
            std::shared_ptr<memtable::MemTable> active_memtable;
            std::shared_ptr<memtable::MemTable> immutable_memtable;
            std::vector<sstable::SSTable> sstables;
            std::mutex memtable_mutex;
            std::mutex mutex;
            uint32_t next_sstable_id;

            void swapActiveMemtable (){
                 // Move the full active memtable into the immutable slot.
                immutable_memtable = active_memtable;

                // Create a fresh active memtable for new writes.
                active_memtable = std::make_shared<memtable::MemTable>(allocate_sstable_path(), m_data_size, m_sstable_block_size);
            }


            void flushSSTable(const std::shared_ptr<memtable::MemTable> &immutable)
            {
                // Don't hold the mutex while doing disk I/O.
                flush_threads.emplace_back(std::thread([this, immutable]()
                {
                    immutable->writeSSTable();
                                                    
                    std::lock_guard<std::mutex> lock(memtable_mutex);
                    sstables.push_back(sstable::SSTable(immutable->get_file_path()));
                    // Only clear it if this is still the immutable memtable
                    // that this thread flushed.
                    if (immutable_memtable == immutable)
                        immutable_memtable.reset(); 
                }));
            }

            std::vector<std::string> get_sstable_files()
            {

                std::vector<std::string> files;
            
                for (const auto& entry : std::filesystem::directory_iterator(m_sstable_folder))
                {
                    if (entry.is_regular_file())
                    {
                        files.push_back(entry.path().string());
                    }
                }
                return files;
            }
            
            void load_ss_tables()
            {
                std::vector<std::string> files = get_sstable_files();
                sort(files.begin(), files.end());
                for(auto file : files)
                {
                    std::cout<<"Initializing SSTable for file: "<<file<<std::endl;
                    sstables.push_back(sstable::SSTable(file));
                }
            }

            uint32_t get_last_sstable_fileid()
            {                
                // reads the ss_table_folder_path, has some segment table format parsing and autoincrement logic
                // for example data_1.sstable etc.
                std::vector<std::string> files = get_sstable_files();
                if (files.size() == 0)
                {
                    return 1;
                }
                sort(files.begin(), files.end());
                std::regex pattern(R"(data_(\d+)\.sst)");
                std::smatch match;

                std::string last_file = files.back();
                std::cout<<"last file is:" <<last_file<<std::endl;
                if (std::regex_search(last_file, match, pattern))
                {
                    int number = std::stoi(match[1].str());
                    return number+1;
                }
                else
                {
                    std::cout<<"no match"<<std::endl;
                    return  1;
                }
            }

            std::string allocate_sstable_path()
            {
                return std::format(
                    "./ss_tables_folder/data_{:06}.sst",
                    next_sstable_id++
                );
            }

            void load_startup_memtables()
            {
                // use recovery logs to build memtable or fresh one.
                // for now building fresh mem_table
                active_memtable = std::make_shared<memtable::MemTable>(allocate_sstable_path(), m_data_size, m_sstable_block_size); // threshold and blocksize
                immutable_memtable = nullptr;
            }
        };
}
#endif