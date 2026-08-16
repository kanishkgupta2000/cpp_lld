#include <fstream>
#include <iostream>

int main() {
    std::ofstream out("data.bin", std::ios::binary);

    const char data[] = "hello";

    out.write(data, 5);

    out.close();
}