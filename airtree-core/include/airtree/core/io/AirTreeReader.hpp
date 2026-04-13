#ifndef AIRTREE_CORE_IO_AIRTREEREADER_HPP
#define AIRTREE_CORE_IO_AIRTREEREADER_HPP

#include <airtree/core/common/AirTreeType.hpp>
#include <airtree/core/common/TrieHeader.hpp>
#include <optional>
#include <vector>
#include <memory>
#include <string>

using namespace airtree::core::common;


namespace airtree::core::io {

class AirTreeReader {
public:
  AirTreeReader() = default;

  int getDims() const noexcept;
  int getBitLength() const noexcept;
  AirTreeType getType() noexcept;
  const trie_header &getHeader() const noexcept;

  void read(const std::vector<char> &buffer);


private:
  int dims_ = 0;
  int bit_length_ = 0;
  trie_header header_;
  AirTreeType type_;
};


} // namespace airtree::core::io


#endif // AIRTREE_CORE_IO_AIRTREEREADER_HPP