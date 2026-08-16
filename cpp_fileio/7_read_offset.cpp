#include<iostream>
#include<fstream>

int main()
{
    std::ofstream out("data.bin", std::ios::binary);
    const std::string s =  "hello_world_12345";
    out.write(s.data(), s.size());

    std::ifstream in("data.bin", std::ios::binary);
    in.seekg(6);
    char buffer[5];
    in.read(buffer, 5);
    std::cout.write(buffer, 5);
}