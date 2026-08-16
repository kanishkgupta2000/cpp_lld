#include <cstdint>
#include <fstream>
#include <iostream>

int main()
{
    std::ofstream out("data.bin", std::ios::binary);

std::cout << out.tellp() << "\n";

out.write("hello", 5);

std::cout << out.tellp() << "\n";

out.write("world", 5);

std::cout << out.tellp() << "\n";

out.write("!!!!!", 5);

std::cout << out.tellp() << "\n";
}