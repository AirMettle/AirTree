// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_IO_AIRTREEWRITER_HPP
#define AIRTREE_CORE_IO_AIRTREEWRITER_HPP


#include <string>
#include <vector>

namespace airtree::core::io {

class AirTreeWriter {
public:
  // Writes a AirTree serialized buffer on disk.
  static void Write(const std::vector<char> &buffer,
                    const std::string &filename);
};

} // namespace airtree::core::io


#endif // AIRTREE_CORE_IO_AIRTREEWRITER_HPP
