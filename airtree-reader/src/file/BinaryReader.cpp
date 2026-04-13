#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>

#include <airtree/reader/file/BinaryReader.hpp>
#include <airtree/reader/Logger.hpp>

using namespace airtree::reader::file;
using namespace airtree::reader;

// Define destructor
BinaryFileParser::~BinaryFileParser() {
  // Destructor implementation (can be empty)
}

template <typename T>
std::vector<std::vector<T>>
readBinaryDataCLI(const std::string &binary_file_path) {
  std::vector<T> data_vector;
  std::ifstream binaryFile(binary_file_path, std::ios::binary);

  if (!binaryFile) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Failed to open binary data file: {}", binary_file_path);
    return {}; // Return empty vector if file opening fails
  }


  binaryFile.seekg(0, std::ios::end);
  std::streamsize file_size = binaryFile.tellg();
  binaryFile.seekg(0, std::ios::beg);

  size_t num_elements = file_size / sizeof(T);
  data_vector.reserve(num_elements);

  // Read entire file at once
  data_vector.resize(num_elements);
  binaryFile.read(reinterpret_cast<char *>(data_vector.data()), file_size);

  // Move instead of copy
  std::vector<std::vector<T>> result;
  result.emplace_back(std::move(data_vector));
  // const unsigned seed = 123456789;
  // std::mt19937 g1(seed);
  // std::mt19937 g2(seed);
  // std::mt19937 g3(seed);

  // Create 3 additional data vectors with shuffled values
  // std::vector<T> data_vector1 = data_vector;
  // std::vector<T> data_vector2 = data_vector;
  // std::vector<T> data_vector3 = data_vector;
  //
  // std::shuffle(data_vector1.begin(), data_vector1.end(), g1);
  // std::shuffle(data_vector2.begin(), data_vector2.end(), g2);
  // std::shuffle(data_vector3.begin(), data_vector3.end(), g3);

  // Return the data as a vector of vectors
  // return {data_vector, data_vector1, data_vector2, data_vector3};
  return result;
}

// Define the parse method
InputDataVector BinaryFileParser::parse(const std::string &file_path,
                                        const SUPPORTED_DATA_TYPE data_type) {
  InputDataVector input_data_vector;

  if (data_type == SUPPORTED_DATA_TYPE::INT32) {
    auto vector = readBinaryDataCLI<int32_t>(file_path);
    for (auto &data : vector) {
      input_data_vector.emplace_back(std::move(data));
    }
  } else if (data_type == SUPPORTED_DATA_TYPE::INT64) {
    auto vector = readBinaryDataCLI<int64_t>(file_path);
    for (auto &data : vector) {
      input_data_vector.emplace_back(std::move(data));
    }
  } else if (data_type == SUPPORTED_DATA_TYPE::FLOAT) {
    auto vector = readBinaryDataCLI<float>(file_path);
    for (auto &data : vector) {
      input_data_vector.emplace_back(std::move(data));
    }
  } else if (data_type == SUPPORTED_DATA_TYPE::DOUBLE) {
    auto vector = readBinaryDataCLI<double>(file_path);
    for (auto &data : vector) {
      input_data_vector.emplace_back(std::move(data));
    }
  } else {
    SPDLOG_LOGGER_ERROR(
        logger(), "Unsupported data type: {}", to_string(data_type));
    return {};
  }
  return input_data_vector;
}
