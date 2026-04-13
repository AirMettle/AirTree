#ifndef AIRTREE_MERGE_AIRTREEMERGE_HPP
#define AIRTREE_MERGE_AIRTREEMERGE_HPP

#include <vector>
#include <string>

namespace airtree::merge {

/**
 * @brief Merge two AirTree histogram buffers into a single buffer
 * @param buffer1 First AirTree histogram buffer
 * @param buffer2 Second AirTree histogram buffer
 * @return Merged AirTree histogram as a new buffer
 * @throws std::runtime_error if buffers have incompatible configurations or
 * merge fails
 */
std::vector<char> mergeAirTree(const std::vector<char> &buffer1,
                               const std::vector<char> &buffer2);

/**
 * @brief Merge two AirTree histogram buffers and save the result to a file
 * @param buffer1 First AirTree histogram buffer
 * @param buffer2 Second AirTree histogram buffer
 * @param output_path Path to save the merged histogram
 * @throws std::runtime_error if merge fails or file cannot be written
 */
void mergeAirTree(const std::vector<char> &buffer1,
                  const std::vector<char> &buffer2,
                  const std::string &output_path);

} // namespace airtree::merge

#endif // AIRTREE_MERGE_AIRTREEMERGE_HPP
