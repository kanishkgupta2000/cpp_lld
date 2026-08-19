#include<iostream>
#include<fstream>
#include<vector>
#include "sstable.h"


using namespace sstable;

int main()
{
    sstable::SSTableWriter ss_table(2);
    std::ofstream out("data.bin", std::ios::binary);
    ss_table.write(out, "apple", "doctor sent away", 0);
    ss_table.write(out, "ball", "catch", 0);
    ss_table.write(out, "cat", "meoww?", 0);
    ss_table.write(out, "dog", "bowwow", 0);
    ss_table.write(out, "mouse", "jerry", 0);
    ss_table.writeIndexAndHeader(out);

}