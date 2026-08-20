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

namespace storage_engine{
    class StorageEngine {
            public:
            StorageEngine(std::string ss_table_folder_path, uint32_t data_size, uint32_t sstable_block_size) : m_sstable_folder(ss_table_folder_path),
            sstables(), m_data_size(data_size), m_sstable_block_size(sstable_block_size)
            {
                load_ss_tables(); // sstables loaded
                load_startup_memtables();  // memtables created with latest file
            }

            memtable::MemTableReadResult read(std::string key);
            void put(std::string key, std::string value);
            void delete_key(std::string key);
            

            private:
            uint32_t m_sstable_block_size;
            uint32_t m_data_size;
            std::string m_sstable_folder;
            std::shared_ptr<memtable::MemTable> active_memtable;
            std::shared_ptr<memtable::MemTable> immutable_memtable;
            std::vector<sstable::SSTable> sstables;
            std::mutex memtable_mutex;
            std::mutex mutex;


            std::vector<std::string> get_sstable_files()
            {
                std::vector<std::string> files;
            
                for (const auto& entry : std::filesystem::directory_iterator(m_sstable_folder))
                {
                    if (entry.is_regular_file())
                    {
                        files.push_back(entry.path().filename().string());
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
                    sstables.push_back(sstable::SSTable(file));
                }
            }

            std::string get_candidate_file_path()
            {
                std::string file_format = "data_{:06}.sst";
                // reads the ss_table_folder_path, has some segment table format parsing and autoincrement logic
                // for example data_1.sstable etc.
                std::vector<std::string> files = get_sstable_files();

                if (files.size() == 0)
                {
                    return std::format(file_format, 1);
                }
                sort(files.begin(), files.end());
                std::regex pattern(R"(data_(\d+)\.sst)");
                std::smatch match;

                if (std::regex_match(files.back(), match, pattern))
                {
                    int number = std::stoi(match[1].str());
                    return std::format(file_format, number);
                }
                else
                {
                    return std::format(file_format, 1);
                }
            }

            void load_startup_memtables()
            {
                // use recovery logs to build memtable or fresh one.
                // for now building fresh mem_table
                active_memtable = std::make_shared<memtable::MemTable>(get_candidate_file_path(), m_data_size, m_sstable_block_size); // threshold and blocksize
                immutable_memtable = nullptr;
            }
        };
}
#endif