#include <gtest/gtest.h>
#include <airtree/reader/file/Reader.hpp>
#include <filesystem>
#include <vector>
#include <variant>

using namespace airtree::reader::file;

class ParquetReaderTest : public ::testing::Test {
protected:
  std::string testdata_dir;

  void SetUp() override {
    // Get path to testdata directory
    std::filesystem::path current(__FILE__);
    testdata_dir = (current.parent_path() / "testdata").string();
  }
};

// Test INT32 parquet file reading
TEST_F(ParquetReaderTest, ReadInt32Column) {
  std::string filepath = testdata_dir + "/test_int32.parquet";
  std::vector<std::string> columns = {"value"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::PARQUET,
                                    SUPPORTED_DATA_TYPE::INT32, columns);

  ASSERT_EQ(data.size(), 1) << "Should return 1 column";

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

// Test INT64 parquet file reading
TEST_F(ParquetReaderTest, ReadInt64Column) {
  std::string filepath = testdata_dir + "/test_int64.parquet";
  std::vector<std::string> columns = {"value"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::PARQUET,
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

// Test FLOAT parquet file reading
TEST_F(ParquetReaderTest, ReadFloatColumn) {
  std::string filepath = testdata_dir + "/test_float.parquet";
  std::vector<std::string> columns = {"value"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::PARQUET,
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

// Test DOUBLE parquet file reading
TEST_F(ParquetReaderTest, ReadDoubleColumn) {
  std::string filepath = testdata_dir + "/test_double.parquet";
  std::vector<std::string> columns = {"value"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::PARQUET,
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

// Test multi-column parquet file reading
TEST_F(ParquetReaderTest, ReadMultipleColumns) {
  std::string filepath = testdata_dir + "/test_multicolumn.parquet";
  std::vector<std::string> columns = {"col_int32", "col_float", "col_double"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::PARQUET,
                                    SUPPORTED_DATA_TYPE::INT32, columns);

  ASSERT_EQ(data.size(), 3) << "Should return 3 columns";

  // Check col_int32
  auto *vec_int32 = std::get_if<std::vector<int32_t>>(&data[0]);
  ASSERT_NE(vec_int32, nullptr);
  ASSERT_EQ(vec_int32->size(), 10);
  EXPECT_EQ((*vec_int32)[0], 1);
  EXPECT_EQ((*vec_int32)[4], 5);

  // Check col_float
  auto *vec_float = std::get_if<std::vector<float>>(&data[1]);
  ASSERT_NE(vec_float, nullptr);
  ASSERT_EQ(vec_float->size(), 10);
  EXPECT_FLOAT_EQ((*vec_float)[0], 1.5f);
  EXPECT_FLOAT_EQ((*vec_float)[4], 5.5f);

  // Check col_double
  auto *vec_double = std::get_if<std::vector<double>>(&data[2]);
  ASSERT_NE(vec_double, nullptr);
  ASSERT_EQ(vec_double->size(), 10);
  EXPECT_DOUBLE_EQ((*vec_double)[0], 1.123456);
  EXPECT_DOUBLE_EQ((*vec_double)[4], 5.567890);
}

// Test reading all columns in order
TEST_F(ParquetReaderTest, ReadAllColumnsInOrder) {
  std::string filepath = testdata_dir + "/test_multicolumn.parquet";
  std::vector<std::string> columns = {
      "col_int32", "col_int64", "col_float", "col_double"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::PARQUET,
                                    SUPPORTED_DATA_TYPE::INT32, columns);

  ASSERT_EQ(data.size(), 4) << "Should return 4 columns";

  // Verify we got all column types
  EXPECT_NE(std::get_if<std::vector<int32_t>>(&data[0]), nullptr)
      << "col_int32 should be int32_t";
  EXPECT_NE(std::get_if<std::vector<int64_t>>(&data[1]), nullptr)
      << "col_int64 should be int64_t";
  EXPECT_NE(std::get_if<std::vector<float>>(&data[2]), nullptr)
      << "col_float should be float";
  EXPECT_NE(std::get_if<std::vector<double>>(&data[3]), nullptr)
      << "col_double should be double";
}

// Test reading single column from multi-column file
TEST_F(ParquetReaderTest, ReadSingleColumnFromMulti) {
  std::string filepath = testdata_dir + "/test_multicolumn.parquet";
  std::vector<std::string> columns = {"col_double"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::PARQUET,
                                    SUPPORTED_DATA_TYPE::DOUBLE, columns);

  ASSERT_EQ(data.size(), 1) << "Should return 1 column";

  auto *vec_ptr = std::get_if<std::vector<double>>(&data[0]);
  ASSERT_NE(vec_ptr, nullptr);
  EXPECT_EQ(vec_ptr->size(), 10);
}

// Test non-existent column
TEST_F(ParquetReaderTest, ReadNonExistentColumn) {
  std::string filepath = testdata_dir + "/test_multicolumn.parquet";
  std::vector<std::string> columns = {"nonexistent_column"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::PARQUET,
                                    SUPPORTED_DATA_TYPE::INT32, columns);

  EXPECT_TRUE(data.empty())
      << "Should return empty vector for non-existent column";
}

// Test non-existent file
TEST_F(ParquetReaderTest, ReadNonExistentFile) {
  std::string filepath = testdata_dir + "/nonexistent.parquet";
  std::vector<std::string> columns = {"value"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::PARQUET,
                                    SUPPORTED_DATA_TYPE::INT32, columns);

  EXPECT_TRUE(data.empty())
      << "Should return empty vector for non-existent file";
}

// Test empty column list
TEST_F(ParquetReaderTest, EmptyColumnList) {
  std::string filepath = testdata_dir + "/test_int32.parquet";
  std::vector<std::string> columns; // Empty

  // When columns is empty, parse_file uses data_type path which is for binary
  // files For parquet files, this should ideally handle gracefully or error
  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::PARQUET,
                                    SUPPORTED_DATA_TYPE::INT32, columns);

  // Behavior depends on implementation - typically returns empty for parquet
  // without columns This test documents the current behavior
}
