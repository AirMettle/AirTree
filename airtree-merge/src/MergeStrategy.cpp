#include <airtree/merge/MergeStrategy.hpp>
#include <airtree/merge/Logger.hpp>
#include <fstream>

using namespace airtree::merge;

trie_header
airtree::merge::MergeStrategy::mergeHeaders(trie_header &header1,
                                            const trie_header &header2) {
  SPDLOG_LOGGER_INFO(logger(), "Entering mergeHeaders");
  header1.pos_inf_count = header1.pos_inf_count + header2.pos_inf_count;
  header1.neg_inf_count = header1.neg_inf_count + header2.neg_inf_count;
  header1.pos_zero_count = header1.pos_zero_count + header2.pos_zero_count;
  header1.neg_zero_count = header1.neg_zero_count + header2.neg_zero_count;
  header1.nan_count = header1.nan_count + header2.nan_count;
  SPDLOG_LOGGER_INFO(logger(), "mergeHeaders completed successfully");
  return header1;
}

void MergeStrategy::add_EOF(std::vector<char> &buffer) {
  SPDLOG_LOGGER_INFO(logger(), "Entering add_EOF");
  int32_t endOfFileMarker = -1;
  auto marker_bytes = reinterpret_cast<const char *>(&endOfFileMarker);
  buffer.insert(
      buffer.end(), marker_bytes, marker_bytes + sizeof(endOfFileMarker));
  SPDLOG_LOGGER_INFO(logger(), "add_EOF completed successfully");
}

std::unique_ptr<TrieNode_16_Level1> MergeStrategy::mergeTrieNode16Level1(
    std::unique_ptr<TrieNode_16_Level1> node1,
    std::unique_ptr<TrieNode_16_Level1> node2) {
  SPDLOG_LOGGER_INFO(logger(), "Entering mergeTrieNode16Level1");
  if (!node1) {
    SPDLOG_LOGGER_INFO(logger(), "node1 is nullptr, returning node2");
    return node2;
  }
  if (!node2) {
    SPDLOG_LOGGER_INFO(logger(), "node2 is nullptr, returning node1");
    return node1;
  }
  for (size_t i = 0; i < BINS_256; ++i) {
    SPDLOG_LOGGER_INFO(logger(), "Merging Trie16 Level1 index {}: {} + {}", i,
                       node1->counts[i], node2->counts[i]);
    node1->counts[i] += node2->counts[i];
  }
  return node1;
}

std::unique_ptr<TrieNode_16>
MergeStrategy::mergeTrieNode16(std::unique_ptr<TrieNode_16> node1,
                               std::unique_ptr<TrieNode_16> node2) {
  SPDLOG_LOGGER_INFO(logger(), "Entering mergeTrieNode16");
  if (!node1) {
    SPDLOG_LOGGER_INFO(logger(), "node1 is nullptr, returning node2");
    return node2;
  }
  if (!node2) {
    SPDLOG_LOGGER_INFO(logger(), "node2 is nullptr, returning node1");
    return node1;
  }

  for (size_t i = 0; i < BINS_256; ++i) {
    SPDLOG_LOGGER_INFO(logger(), "Merging Trie16 Level0 index {}", i);
    // Merge the counts at this index.
    node1->counts[i] += node2->counts[i];
    // Set the populated flag if either node has it.
    if (node1->populated.test(i) || node2->populated.test(i)) {
      node1->populated.set(i);
    }
    // Do not merge the child pointers here.
  }
  return node1;
}

void MergeStrategy::saveMergedBinaryFile_buffer(const std::vector<char> &buffer,
                                                const std::string &outputFile) {
  SPDLOG_LOGGER_INFO(
      logger(), "Starting serialization to file: {}", outputFile);
  SPDLOG_LOGGER_INFO(logger(), "Serialized trie structure, buffer size now: {}",
                     buffer.size());

  std::ofstream outFile(outputFile, std::ios::binary);
  if (!outFile) {
    throw std::runtime_error("Failed to open output file: " + outputFile);
  }
  outFile.write(buffer.data(), buffer.size());
  outFile.close();
  SPDLOG_LOGGER_INFO(logger(), "Merged binary file saved as {}", outputFile);
}
