#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include "sstable.h"

using namespace sstable;

Record readRecord(std::ifstream& file)
{
    uint32_t key_size;
    uint32_t value_size;

    file.read(reinterpret_cast<char*>(&key_size), sizeof(key_size));
    file.read(reinterpret_cast<char*>(&value_size), sizeof(value_size));
    std::string key(key_size, '\0');
    std::string value(value_size, '\0');
    file.read(key.data(), key_size);
    file.read(value.data(), value_size);

    return Record{std::move(key), std::move(value)};
}

IndexEntry readIndexEntry(std::ifstream &file)
{
    uint32_t key_size;
    uint32_t value_size;
    file.read(reinterpret_cast<char*>(&key_size), sizeof(key_size));
    file.read(reinterpret_cast<char*>(&value_size), sizeof(value_size));

    std::string key(key_size, '\0');
    uint64_t offset;
    file.read(key.data(), key_size);
    file.read(reinterpret_cast<char*>(&offset), sizeof(offset));

    return IndexEntry(std::move(key), offset);
}


Record readRecordAt(std::ifstream &file, uint64_t offset)
{
    file.seekg(offset);
    return readRecord(file);
}

//apple ball cat dog mouse
// index: apple cat mouse
std::string get_key(std::ifstream &file, std::vector<IndexEntry> &sparse_index, std::string_view key)
{
    int lo = 0;
    int hi = sparse_index.size()-1; //2

    int best = -1;

    while (lo <= hi)
    {
        int mid = lo + (hi - lo) / 2;

        if (sparse_index[mid].key <= key)
        {
            best = mid;
            lo = mid + 1;
        }
        else
        {
            hi = mid - 1;
        }
    }

    // lo maintains the index of the closest sparse_index
    // you can start reading from here until you reach 2 entries (as configured by us)

    file.seekg(sparse_index[best].offset);
    for(int i=0;i< 2; i++) // we loop twice as 2 is the blocksize (defined by the cadence of your sparse index)
    {
        Record record = readRecord(file);
        if (record.key == key)
        {
            return record.value;
        }
    }

    return "not_found";
}

int main()
{
    std::ifstream in("data.bin", std::ios::binary);
    // first get the index
    in.seekg(-16, std::ios::end);

    uint64_t index_offset;
    uint64_t index_size;

    in.read(reinterpret_cast<char*>(&index_offset), sizeof(index_offset));
    in.read(reinterpret_cast<char*>(&index_size), sizeof(index_size));

    std::cout<<index_offset<<" " <<index_size<<std::endl;

    in.seekg(index_offset);
    std::vector<IndexEntry> sparse_index;
    for(int i=0; i< index_size; i++)
    {
        IndexEntry x = readIndexEntry(in);
        sparse_index.push_back(x);
    }
    std::cout<<"starting search"<<std::endl;

    std::string val = get_key(in, sparse_index, "ball");

    std::cout<<"value: "<<val<<std::endl;

}