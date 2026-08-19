#include "memtable.h"
#include "../sstable/sstable.h"
#include<optional>
using namespace sstable;

namespace memtable
{
    MemTable::MemTable(std::string file_path, uint32_t threshold, uint32_t sstable_block_size): m_threshold(threshold), m_map(), ss_table_writer(sstable_block_size), m_file_path(file_path)
    {

    }
    
    MemTable::~MemTable()
    {
    }


    void MemTable::put(std::string key, std::string value)
    {
        m_map[std::string(key)] = sstable::Record(key, value, false);
        size++;
    }

    void MemTable::deleteKey(std::string key)
    {
        m_map[std::string(key)] = sstable::Record(key, NULL, true);
        size++;
    }

    MemTableReadResult MemTable::read(std::string key)
    {
        if (m_map.find(key) != m_map.end())
        {
            sstable::Record readResult = m_map[key];
            if(readResult.is_deletion)
            {
                return MemTableReadResult(readResult, DELETED);
            }
            else
            {
                return MemTableReadResult(readResult, FOUND);
            }
        }
        else
        {
            return MemTableReadResult(sstable::Record(), NOT_FOUND);
        }
    }

    void MemTable::writeSSTable()
    {
        // we need to write the sstable with
        // [key_size][value_size][key][value][record type 0/1]
        std::ofstream out(m_file_path, std::ios::binary);
        for (auto it = m_map.begin(); it != m_map.end(); it++)
        {
            ss_table_writer.write(out,it->first, it->second.value, it->second.is_deletion);
        }
        ss_table_writer.writeIndexAndHeader(out);
    }
};

int main(){}