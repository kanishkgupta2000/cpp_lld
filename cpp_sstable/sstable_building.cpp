#include<iostream>
#include<fstream>
#include<vector>
#include "sstable.h"

using namespace sstable;

void writeRecord(std::ofstream &file, std::string_view key, std::string_view value)
{
    // [keysize][valuesize][key][value]
    uint32_t key_size = key.size();
    uint32_t value_size = value.size();
    file.write(reinterpret_cast<char*>(&key_size), sizeof(key_size));
    file.write(reinterpret_cast<char*>(&value_size), sizeof(value_size));
    file.write(key.data(), key_size);
    file.write(value.data(), value_size);
}

void writeIndex(std::ofstream &file, std::string_view key, uint64_t offset_value)
{
    // [keysize][valuesize][key][value]
    uint32_t key_size = key.size();
    uint32_t offset_size = sizeof(offset_value);
    file.write(reinterpret_cast<char*>(&key_size), sizeof(key_size));
    file.write(reinterpret_cast<char*>(&offset_size), sizeof(offset_size));
    file.write(key.data(), key_size);
    file.write(reinterpret_cast<char*>(&offset_value), sizeof(offset_value));
}

int main()
{
    std::vector<IndexEntry> sparse_index;
    {
        std::ofstream out("data.bin", std::ios::binary);

        sparse_index.push_back(IndexEntry("apple", out.tellp()));
        writeRecord(out, "apple", "doctor sent away");

        writeRecord(out, "ball", "catch");

        sparse_index.push_back(IndexEntry("cat", out.tellp()));
        writeRecord(out, "cat", "meoww?");

        writeRecord(out, "dog", "bowwow");

        sparse_index.push_back(IndexEntry("mouse", out.tellp()));
        writeRecord(out, "mouse", "jerry");


        // now i have the key-value records already written in the file, i have the sparse index containing key, offset
        uint64_t index_offset = out.tellp();
        uint64_t index_size = sparse_index.size();
        for(int i=0; i < sparse_index.size(); i++)
        {
            writeIndex(out, sparse_index[i].key, sparse_index[i].offset);
        }
        out.write(reinterpret_cast<char*>(&index_offset), sizeof(index_offset));
        out.write(reinterpret_cast<char*>(&index_size), sizeof(index_size));
    }
    
    std::ifstream in("data.bin", std::ios::binary);

}