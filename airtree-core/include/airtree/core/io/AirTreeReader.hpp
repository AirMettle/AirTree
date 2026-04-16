#ifndef AIRTREE_CORE_IO_AIRTREEREADER_HPP
#define AIRTREE_CORE_IO_AIRTREEREADER_HPP

#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/AirTreeType.hpp>
#include <vector>
#include <memory>

using namespace airtree::core::common;

namespace airtree::core::io {

class AirTreeReader {
public:
  AirTreeReader() = default;

  int getDims() const noexcept;
  int getBitLength() const noexcept;
  AirTreeType getType() noexcept;
  const AirTreeHeader &getHeader() const noexcept;

  void read(const std::vector<char> &buffer);

private:
  int dims_ = 0;
  int bit_length_ = 0;
  AirTreeHeader header_;
  AirTreeType type_;
};

} // namespace airtree::core::io

#endif // AIRTREE_CORE_IO_AIRTREEREADER_HPP
