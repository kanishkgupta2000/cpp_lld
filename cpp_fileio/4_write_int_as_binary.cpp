#include <cstdint>
#include <fstream>
#include <iostream>

int main() {
    std::ofstream out("data.bin", std::ios::binary);

    uint32_t a = 100;
    uint32_t b = 200;

    out.write(
        reinterpret_cast<const char*>(&a),
        sizeof(a)
    );

    out.write(
        reinterpret_cast<const char*>(&b),
        sizeof(b)
    );

    out.close();
}