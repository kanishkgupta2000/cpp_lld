#ifndef INCLUDED_SSTABLE
#define INCLUDED_SSTABLE
#include <string>


namespace sstable{
    struct Record{
        std::string key;
        std::string value;
    };
    
    struct IndexEntry {
        std::string key;
        uint64_t offset;
    };
};

#endif