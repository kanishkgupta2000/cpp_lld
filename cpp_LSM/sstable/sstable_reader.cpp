#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include "sstable.h"


using namespace sstable;

int main()
{
    std::cout<<"starting search"<<std::endl;
    SSTable ss_table("data.bin");
    Record result = ss_table.get_key("ball");
    std::cout<<"value: "<<result.value<<std::endl;

}