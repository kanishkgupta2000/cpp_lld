#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include "sstable.h"


namespace sstable
{

}


using namespace sstable;

int main()
{
    std::cout<<"starting search"<<std::endl;
    SSTable ss_table("data.bin");
    std::string val = ss_table.get_key("ball");
    std::cout<<"value: "<<val<<std::endl;

}