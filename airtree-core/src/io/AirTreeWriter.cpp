// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/io/AirTreeWriter.hpp>
#include <fstream>
#include <iostream>

namespace airtree::core::io {

    void AirTreeWriter::Write(const std::vector<char> &buffer, const std::string &filename) {
        std::ofstream file(filename, std::ios::binary);
        if (file.is_open()) {
            file.write(buffer.data(), buffer.size());
            file.close();
        } else {
            std::cerr << "Unable to open file for writing." << std::endl;
        }
}

} // namespace airtree::core::io