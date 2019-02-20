/* Converts a file into a .h and a .cpp file containing an array with the
 * binary data of the file.
 */
#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    using namespace std::string_literals;

    if (argc != 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <input file> <output file basename> <array identifier>\n";
        return EXIT_FAILURE;
    }

    const std::string input_name = argv[1];
    const std::string output_name_h = argv[2] + ".h"s;
    const std::string output_name_cpp = argv[2] + ".cpp"s;
    const std::string array_name = argv[3];
    std::array<char, 1048576> buf;
    int total_bytes = 0;

    std::ifstream infile(input_name, std::ios_base::binary);
    if (not infile.is_open()) {
        std::cerr << "error: failed to open input file\n";
        return EXIT_FAILURE;
    }

    std::ofstream outfile(output_name_cpp);
    if (not outfile.is_open()) {
        std::cerr << "error: failed to create " << output_name_cpp << '\n';
        return EXIT_FAILURE;
    }

    outfile << "#include \"" << output_name_h << "\"\n\nconst unsigned char " << array_name
            << "[] = {";

    do {
        infile.read(buf.data(), buf.size());
        for (long i = 0; i < infile.gcount(); ++i) {
            if (i % 20 == 0) {
                outfile << "\n";
            }
            outfile << (unsigned)(uint8_t)buf[i] << ",";
        }
        total_bytes += infile.gcount();
    } while (infile.gcount() == buf.size());
    outfile << "\n};\n";

    outfile.close();
    outfile.open(output_name_h);
    if (not outfile.is_open()) {
        std::cerr << "error: failed to create " << output_name_h << '\n';
        return EXIT_FAILURE;
    }

    outfile << "#pragma once\nextern const unsigned char " << array_name << "["
            << total_bytes << "];\n";
    outfile.close();
}
