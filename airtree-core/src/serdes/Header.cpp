#include <airtree/core/serdes/Header.hpp>
#include <iostream>
#include <airtree/core/Logger.hpp>

using namespace airtree::core;

void serializeTrieHeader(const trie_header &header, std::vector<char> &buffer) {
  // Append type_code (9 bytes)
  buffer.insert(buffer.end(), header.type_code, header.type_code + 9);

  // Append version (4 bytes)
  buffer.insert(
      buffer.end(), reinterpret_cast<const char *>(&header.version),
      reinterpret_cast<const char *>(&header.version) + sizeof(header.version));

  // Append m_width (4 bytes)
  buffer.insert(
      buffer.end(), reinterpret_cast<const char *>(&header.m_width),
      reinterpret_cast<const char *>(&header.m_width) + sizeof(header.m_width));

  // Append precision_bits (4 bytes)
  buffer.insert(buffer.end(),
                reinterpret_cast<const char *>(&header.precision_bits),
                reinterpret_cast<const char *>(&header.precision_bits)
                    + sizeof(header.precision_bits));

  // Append node_width (4 bytes)
  buffer.insert(buffer.end(),
                reinterpret_cast<const char *>(&header.node_width),
                reinterpret_cast<const char *>(&header.node_width)
                    + sizeof(header.node_width));

  // Append type (4 bytes)
  buffer.insert(buffer.end(), header.type, header.type + 4);

  // Append Config (4 bytes)
  buffer.insert(buffer.end(), header.config, header.config + 4);

  // Append mode (4 bytes)
  buffer.insert(
      buffer.end(), reinterpret_cast<const char *>(&header.mode),
      reinterpret_cast<const char *>(&header.mode) + sizeof(header.mode));

  // Append pos_inf_count (4 bytes)
  buffer.insert(buffer.end(),
                reinterpret_cast<const char *>(&header.pos_inf_count),
                reinterpret_cast<const char *>(&header.pos_inf_count)
                    + sizeof(header.pos_inf_count));

  // Append neg_inf_count (4 bytes)
  buffer.insert(buffer.end(),
                reinterpret_cast<const char *>(&header.neg_inf_count),
                reinterpret_cast<const char *>(&header.neg_inf_count)
                    + sizeof(header.neg_inf_count));

  // Append pos_zero_count (4 bytes)
  buffer.insert(buffer.end(),
                reinterpret_cast<const char *>(&header.pos_zero_count),
                reinterpret_cast<const char *>(&header.pos_zero_count)
                    + sizeof(header.pos_zero_count));

  // Append neg_zero_count (4 bytes)
  buffer.insert(buffer.end(),
                reinterpret_cast<const char *>(&header.neg_zero_count),
                reinterpret_cast<const char *>(&header.neg_zero_count)
                    + sizeof(header.neg_zero_count));

  // Append nan_count (4 bytes)
  buffer.insert(buffer.end(), reinterpret_cast<const char *>(&header.nan_count),
                reinterpret_cast<const char *>(&header.nan_count)
                    + sizeof(header.nan_count));

  // Append trie_root_ref (4 bytes)
  buffer.insert(buffer.end(),
                reinterpret_cast<const char *>(&header.trie_root_ref),
                reinterpret_cast<const char *>(&header.trie_root_ref)
                    + sizeof(header.trie_root_ref));
}

trie_header deserializeTrieHeader(const std::vector<char> &buffer,
                                  size_t &offset) {
  trie_header header;

  // Ensure there's enough data in the buffer
  // 61 is the maximum size of the header , it will never be less than this
  // or more than 61 under any circumstances
  if (buffer.size() < 61) {
    throw std::runtime_error(
        "Buffer too small to contain a complete trie_header");
    SPDLOG_LOGGER_ERROR(
        logger(), "Buffer too small to contain a complete trie_header");
  }

  // Read type_code (9 bytes)
  std::memcpy(header.type_code, buffer.data() + offset, 9);
  offset += 9;

  // Read version (4 bytes)
  std::memcpy(&header.version, buffer.data() + offset, sizeof(header.version));
  offset += sizeof(header.version);

  // Read m_width (4 bytes)
  std::memcpy(&header.m_width, buffer.data() + offset, sizeof(header.m_width));
  offset += sizeof(header.m_width);

  // Read precision_bits (4 bytes)
  std::memcpy(&header.precision_bits, buffer.data() + offset,
              sizeof(header.precision_bits));
  offset += sizeof(header.precision_bits);

  // Read node_width (4 bytes)
  std::memcpy(
      &header.node_width, buffer.data() + offset, sizeof(header.node_width));
  offset += sizeof(header.node_width);

  // Read type (4 bytes)
  std::memcpy(header.type, buffer.data() + offset, 4);
  offset += 4;

  // Read Config (4 bytes)
  std::memcpy(header.config, buffer.data() + offset, 4);
  offset += 4;

  // Read mode (4 bytes)
  std::memcpy(&header.mode, buffer.data() + offset, sizeof(header.mode));
  offset += sizeof(header.mode);

  // Read pos_inf_count (4 bytes)
  std::memcpy(&header.pos_inf_count, buffer.data() + offset,
              sizeof(header.pos_inf_count));
  offset += sizeof(header.pos_inf_count);

  // Read neg_inf_count (4 bytes)
  std::memcpy(&header.neg_inf_count, buffer.data() + offset,
              sizeof(header.neg_inf_count));
  offset += sizeof(header.neg_inf_count);

  // Read pos_zero_count (4 bytes)
  std::memcpy(&header.pos_zero_count, buffer.data() + offset,
              sizeof(header.pos_zero_count));
  offset += sizeof(header.pos_zero_count);

  // Read neg_zero_count (4 bytes)
  std::memcpy(&header.neg_zero_count, buffer.data() + offset,
              sizeof(header.neg_zero_count));
  offset += sizeof(header.neg_zero_count);

  // Read nan_count (4 bytes)
  std::memcpy(
      &header.nan_count, buffer.data() + offset, sizeof(header.nan_count));
  offset += sizeof(header.nan_count);

  // Read trie_root_ref (4 bytes)
  std::memcpy(&header.trie_root_ref, buffer.data() + offset,
              sizeof(header.trie_root_ref));
  offset += sizeof(header.trie_root_ref);

  return header;
}

trie_header deserializeTrieHeader(const std::vector<char> &buffer) {
  trie_header t_header;
  if (buffer.size() >= sizeof(trie_header)) {
    memcpy(&t_header, buffer.data(), sizeof(trie_header));

    //        // Print statements for each field
    //        std::cout << "Type Code: " << t_header.type_code << std::endl;
    //        std::cout << "Version: " << t_header.version << std::endl;
    //        std::cout << "M Width: " << t_header.m_width << std::endl;
    //        std::cout << "Precision Bits: " << t_header.precision_bits <<
    //        std::endl; std::cout << "Node Width: " << t_header.node_width <<
    //        std::endl; std::cout << "Type: " << t_header.type << std::endl;
    //        std::cout << "Mode: " << t_header.mode << std::endl;
    //        std::cout << "Positive Infinity Count: " << t_header.pos_inf_count
    //        << std::endl; std::cout << "Negative Infinity Count: " <<
    //        t_header.neg_inf_count << std::endl; std::cout << "Positive Zero
    //        Count: " << t_header.pos_zero_count << std::endl; std::cout <<
    //        "Negative Zero Count: " << t_header.neg_zero_count << std::endl;
    //        std::cout << "NaN Count: " << t_header.nan_count << std::endl;
    //        std::cout << "Positive-Positive Count: " << t_header.pos_pos <<
    //        std::endl; std::cout << "Positive-Negative Count: " <<
    //        t_header.pos_neg << std::endl; std::cout << "Negative-Positive
    //        Count: " << t_header.neg_pos << std::endl; std::cout <<
    //        "Negative-Negative Count: " << t_header.neg_neg << std::endl;
    //        std::cout << "Trie Root Reference: " << t_header.trie_root_ref <<
    //        std::endl;
  } else {
    std::cout << "Buffer is too small to contain a valid trie header."
              << std::endl;

    // Handle the error (e.g., set default values or throw an exception)
  }

  return t_header;
}