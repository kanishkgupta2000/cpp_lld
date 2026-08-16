#include <fstream>
#include <iostream>
#include <string>

int main() {
    std::ofstream out("test.txt");

    out << "hello\n";
    out << "world\n";

    out.close();

    std::ifstream in("test.txt");

    std::string line;

    while (std::getline(in, line)) {
        std::cout << line << "\n";
    }

    in.close();
}