#ifndef INCLUDED_MEMTABLE
#define INCLUDED_MEMTABLE
#include<fstream>
#include "../sstable/sstable.h"
#include<map>

namespace memtable{
    enum ReadStatus{
        FOUND=0,
        NOT_FOUND=1,
        DELETED=2
    };

    struct MemTableReadResult{
        sstable::Record record;
        ReadStatus status;
    };

    class MemTable
    {   
        public:
        MemTable(std::string file_path, uint32_t threshold, uint32_t sstable_block_size);
        ~MemTable();
        void put(std::string key, std::string value);
        void deleteKey(std::string key);
        MemTableReadResult read(std::string key);
        void writeSSTable();
        bool MemTable::isFull(){
            return m_size >= m_threshold;
        }

        private:
        sstable::SSTableWriter ss_table_writer;
        std::string m_file_path;
        uint32_t m_threshold;
        std::map<std::string, sstable::Record> m_map;
        uint32_t m_size;
    };
};

#endif