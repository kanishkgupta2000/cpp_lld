#include <fstream>
#include <iostream>

int main() {
    std::ifstream in("data.bin", std::ios::binary);

    char buffer[5];

    in.read(buffer, 5);

    for (int i = 0; i < 5; i++) {
        std::cout << buffer[i];
    }

    std::cout << "\n";
}