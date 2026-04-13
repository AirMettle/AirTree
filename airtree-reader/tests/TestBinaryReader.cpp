#include <gtest/gtest.h>
#include <airtree/reader/file/Reader.hpp>
#include <filesystem>
#include <fstream>
#include <vector>
#include <variant>

using namespace airtree::reader::file;

class BinaryReaderTest : public ::testing::Test {
protected:
  std::string testdata_dir;

  void SetUp() override {
    // Get path to testdata directory
    std::filesystem::path current(__FILE__);
    testdata_dir = (current.parent_path() / "testdata").string();
  }
};

// Test INT32 binary file reading
TEST_F(BinaryReaderTest, ReadInt32File) {
  std::string filepath = testdata_dir + "/test_int32.bin";
  std::vector<std::string> columns; // Empty for binary files

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::BINARY,
                                    SUPPORTED_DATA_TYPE::INT32, columns);

  ASSERT_EQ(data.size(), 1) << "Binary file should return 1 column";

  // Expected data: {1, 2, 3, 4, 5, -1, -2, -3, 0, 100}
  auto *vec_ptr = std::get_if<std::vector<int32_t>>(&data[0]);
  ASSERT_NE(vec_ptr, nullptr) << "Data should be int32_t vector";

  const auto &vec = *vec_ptr;
  ASSERT_EQ(vec.size(), 10) << "Should have 10 elements";

  EXPECT_EQ(vec[0], 1);
  EXPECT_EQ(vec[1], 2);
  EXPECT_EQ(vec[2], 3);
  EXPECT_EQ(vec[3], 4);
  EXPECT_EQ(vec[4], 5);
  EXPECT_EQ(vec[5], -1);
  EXPECT_EQ(vec[6], -2);
  EXPECT_EQ(vec[7], -3);
  EXPECT_EQ(vec[8], 0);
  EXPECT_EQ(vec[9], 100);
}

// Test INT64 binary file reading
TEST_F(BinaryReaderTest, ReadInt64File) {
  std::string filepath = testdata_dir + "/test_int64.bin";
  std::vector<std::string> columns;

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::BINARY,
                                    SUPPORTED_DATA_TYPE::INT64, columns);

  ASSERT_EQ(data.size(), 1);

  // Expected data: {100, 200, 300, 400, 500, -100, -200, -300, 0, 1000}
  auto *vec_ptr = std::get_if<std::vector<int64_t>>(&data[0]);
  ASSERT_NE(vec_ptr, nullptr) << "Data should be int64_t vector";

  const auto &vec = *vec_ptr;
  ASSERT_EQ(vec.size(), 10);

  EXPECT_EQ(vec[0], 100);
  EXPECT_EQ(vec[1], 200);
  EXPECT_EQ(vec[2], 300);
  EXPECT_EQ(vec[3], 400);
  EXPECT_EQ(vec[4], 500);
  EXPECT_EQ(vec[5], -100);
  EXPECT_EQ(vec[6], -200);
  EXPECT_EQ(vec[7], -300);
  EXPECT_EQ(vec[8], 0);
  EXPECT_EQ(vec[9], 1000);
}

// Test FLOAT binary file reading
TEST_F(BinaryReaderTest, ReadFloatFile) {
  std::string filepath = testdata_dir + "/test_float.bin";
  std::vector<std::string> columns;

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::BINARY,
                                    SUPPORTED_DATA_TYPE::FLOAT, columns);

  ASSERT_EQ(data.size(), 1);

  // Expected data: {1.5, 2.5, 3.5, 4.5, 5.5, -1.5, -2.5, -3.5, 0.0, 10.5}
  auto *vec_ptr = std::get_if<std::vector<float>>(&data[0]);
  ASSERT_NE(vec_ptr, nullptr) << "Data should be float vector";

  const auto &vec = *vec_ptr;
  ASSERT_EQ(vec.size(), 10);

  EXPECT_FLOAT_EQ(vec[0], 1.5f);
  EXPECT_FLOAT_EQ(vec[1], 2.5f);
  EXPECT_FLOAT_EQ(vec[2], 3.5f);
  EXPECT_FLOAT_EQ(vec[3], 4.5f);
  EXPECT_FLOAT_EQ(vec[4], 5.5f);
  EXPECT_FLOAT_EQ(vec[5], -1.5f);
  EXPECT_FLOAT_EQ(vec[6], -2.5f);
  EXPECT_FLOAT_EQ(vec[7], -3.5f);
  EXPECT_FLOAT_EQ(vec[8], 0.0f);
  EXPECT_FLOAT_EQ(vec[9], 10.5f);
}

// Test DOUBLE binary file reading
TEST_F(BinaryReaderTest, ReadDoubleFile) {
  std::string filepath = testdata_dir + "/test_double.bin";
  std::vector<std::string> columns;

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::BINARY,
                                    SUPPORTED_DATA_TYPE::DOUBLE, columns);

  ASSERT_EQ(data.size(), 1);

  // Expected data: {1.123456, 2.234567, 3.345678, 4.456789, 5.567890,
  //                 -1.123456, -2.234567, -3.345678, 0.0, 10.123456}
  auto *vec_ptr = std::get_if<std::vector<double>>(&data[0]);
  ASSERT_NE(vec_ptr, nullptr) << "Data should be double vector";

  const auto &vec = *vec_ptr;
  ASSERT_EQ(vec.size(), 10);

  EXPECT_DOUBLE_EQ(vec[0], 1.123456);
  EXPECT_DOUBLE_EQ(vec[1], 2.234567);
  EXPECT_DOUBLE_EQ(vec[2], 3.345678);
  EXPECT_DOUBLE_EQ(vec[3], 4.456789);
  EXPECT_DOUBLE_EQ(vec[4], 5.567890);
  EXPECT_DOUBLE_EQ(vec[5], -1.123456);
  EXPECT_DOUBLE_EQ(vec[6], -2.234567);
  EXPECT_DOUBLE_EQ(vec[7], -3.345678);
  EXPECT_DOUBLE_EQ(vec[8], 0.0);
  EXPECT_DOUBLE_EQ(vec[9], 10.123456);
}

// Test reading non-existent file
TEST_F(BinaryReaderTest, ReadNonExistentFile) {
  std::string filepath = testdata_dir + "/nonexistent.bin";
  std::vector<std::string> columns;

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::BINARY,
                                    SUPPORTED_DATA_TYPE::INT32, columns);

  EXPECT_TRUE(data.empty())
      << "Should return empty vector for non-existent file";
}

// Test with unsupported data type (should handle IGNORE gracefully)
TEST_F(BinaryReaderTest, UnsupportedDataType) {
  std::string filepath = testdata_dir + "/test_int32.bin";
  std::vector<std::string> columns;

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::BINARY,
                                    SUPPORTED_DATA_TYPE::IGNORE, columns);

  EXPECT_TRUE(data.empty())
      << "Should return empty vector for IGNORE data type";
}

// Test empty file handling
TEST_F(BinaryReaderTest, EmptyFile) {
  std::string filepath = testdata_dir + "/empty.bin";
  std::vector<std::string> columns;

  // Create empty file
  std::ofstream empty_file(filepath);
  empty_file.close();

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::BINARY,
                                    SUPPORTED_DATA_TYPE::INT32, columns);

  ASSERT_EQ(data.size(), 1);
  auto *vec_ptr = std::get_if<std::vector<int32_t>>(&data[0]);
  ASSERT_NE(vec_ptr, nullptr);
  EXPECT_TRUE(vec_ptr->empty()) << "Empty file should return empty vector";

  // Cleanup
  std::filesystem::remove(filepath);
}
