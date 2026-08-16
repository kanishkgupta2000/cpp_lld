#include <cstdint>
#include <fstream>
#include <iostream>

int main() {
    std::ifstream in("data.bin", std::ios::binary);

    uint32_t a;
    uint32_t b;

    in.read(reinterpret_cast<char*>(&a), sizeof(a));
    in.read(reinterpret_cast<char*>(&b), sizeof(b));

    std::cout << a << "\n";
    std::cout << b << "\n";

    in.close();
}