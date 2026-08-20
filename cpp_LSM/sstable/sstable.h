#ifndef INCLUDED_SSTABLE
#define INCLUDED_SSTABLE
#include<iostream>
#include <string>
#include<vector>


namespace sstable{
    struct Record{
        std::string key;
        std::string value;
        bool is_deletion;
    };
    
    struct IndexEntry {
        std::string key;
        uint64_t offset;
    };

    class SSTable{
        public:
        // reader does not need blocksize
        SSTable(std::string file_path){
            m_file_path = file_path;
            extract_sparse_index();
        }

        ~SSTable()
        {
        }
        
        Record get_key(std::string_view key)
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
            std::ifstream in(m_file_path, std::ios::binary);

            in.seekg(sparse_index[best].offset);
            for(int i=0;i< 2; i++) // we loop twice as 2 is the blocksize (defined by the cadence of your sparse index)
            {
                Record record = readRecord(in);
                if (record.key == key)
                {
                    return record;
                }
            }
        
            return Record("", NULL);
        }        private:
        std::string m_file_path;
        std::vector<IndexEntry> sparse_index;

              
        Record readRecord(std::ifstream &in)
        {
            uint32_t key_size;
            uint32_t value_size;
            uint16_t is_deletion;
            in.read(reinterpret_cast<char*>(&key_size), sizeof(key_size));
            in.read(reinterpret_cast<char*>(&value_size), sizeof(value_size));
            std::string key(key_size, '\0');
            std::string value(value_size, '\0');
            in.read(key.data(), key_size);
            in.read(value.data(), value_size);
            in.read(reinterpret_cast<char*>(&is_deletion), sizeof(is_deletion));
            return Record{std::move(key), std::move(value), is_deletion ==1 ? true: false};
        }
            
        IndexEntry readIndexEntry(std::ifstream &in)
        {
            uint32_t key_size;
            uint32_t value_size;
            in.read(reinterpret_cast<char*>(&key_size), sizeof(key_size));
            in.read(reinterpret_cast<char*>(&value_size), sizeof(value_size));
        
            std::string key(key_size, '\0');
            uint64_t offset;
            in.read(key.data(), key_size);
            in.read(reinterpret_cast<char*>(&offset), sizeof(offset));
        
            return IndexEntry(std::move(key), offset);
        }
    
        void extract_sparse_index()
        {
            std::ifstream in(m_file_path, std::ios::binary);
            in.seekg(-16, std::ios::end);

            uint64_t index_offset;
            uint64_t index_size;

            in.read(reinterpret_cast<char*>(&index_offset), sizeof(index_offset));
            in.read(reinterpret_cast<char*>(&index_size), sizeof(index_size));

            sparse_index.clear();

            in.seekg(index_offset);
            for(int i=0; i< index_size; i++)
            {
                IndexEntry x = readIndexEntry(in);
                sparse_index.push_back(x);
            }
        }
        

    };

    class SSTableWriter{
        public:
        // Writer needs block size
        SSTableWriter(int block_size): m_block_size(block_size), m_size(0){}
        
        void write(std::ofstream &out, std::string_view key, std::string_view value, uint16_t is_deletion)
        {
            if (m_size%m_block_size ==0)
            {
                // write the index
                sparse_index.push_back(IndexEntry(std::string(key), out.tellp()));
            }
            m_size++;
    
            writeRecord(out, key, value,  is_deletion);
        }

        void writeIndexAndHeader(std::ofstream &out)
        {
            // now i have the key-value records already written in the file, i have the sparse index containing key, offset
            uint64_t index_offset = out.tellp();
            uint64_t index_size = sparse_index.size();
            for(int i=0; i < sparse_index.size(); i++)
            {
                writeIndex(out, sparse_index[i].key, sparse_index[i].offset);
            }
            std::cout << "WRITER index_offset = " << index_offset << '\n';
            std::cout << "WRITER index_size   = " << index_size << '\n';
            out.write(reinterpret_cast<char*>(&index_offset), sizeof(index_offset));
            out.write(reinterpret_cast<char*>(&index_size), sizeof(index_size));
            out.flush();
            out.close(); 
        }

        private:
        int m_block_size;
        int m_size;            
        std::vector<IndexEntry> sparse_index;
                
        void writeIndex(std::ofstream &out, std::string_view key, uint64_t offset_value)
        {
            // [keysize][valuesize][key][value]
            uint32_t key_size = key.size();
            uint32_t offset_size = sizeof(offset_value);
            out.write(reinterpret_cast<char*>(&key_size), sizeof(key_size));
            out.write(reinterpret_cast<char*>(&offset_size), sizeof(offset_size));
            out.write(key.data(), key_size);
            out.write(reinterpret_cast<char*>(&offset_value), sizeof(offset_value));
        }

        void writeRecord(std::ofstream &out, std::string_view key, std::string_view value, uint16_t is_deletion)
        {
            // [keysize][valuesize][key][value][deletion?]
            uint32_t key_size = key.size();
            uint32_t value_size = value.size();
            out.write(reinterpret_cast<char*>(&key_size), sizeof(key_size));
            out.write(reinterpret_cast<char*>(&value_size), sizeof(value_size));
            out.write(key.data(), key_size);
            out.write(value.data(), value_size);
            out.write(reinterpret_cast<char*>(&is_deletion), sizeof(is_deletion));
        }
    };

};

#endif