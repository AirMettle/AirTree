// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <gtest/gtest.h>
#include <airtree/reader/file/Reader.hpp>
#include <filesystem>
#include <fstream>
#include <vector>
#include <variant>

using namespace airtree::reader::file;

class CSVReaderTest : public ::testing::Test {
protected:
  std::string testdata_dir;

  void SetUp() override {
    // Get path to testdata directory
    std::filesystem::path current(__FILE__);
    testdata_dir = (current.parent_path() / "testdata").string();
  }
};

// Test INT32 CSV file reading (Note: CSV typically reads integers as int64)
TEST_F(CSVReaderTest, ReadInt32Column) {
  std::string filepath = testdata_dir + "/test_int32.csv";
  std::vector<std::string> columns = {"value"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::AT_CSV,
                                    SUPPORTED_DATA_TYPE::AT_INT32, columns);

  ASSERT_EQ(data.size(), 1) << "Should return 1 column";

  // CSV typically reads integers as int64
  auto *vec_int64 = std::get_if<std::vector<int64_t>>(&data[0]);
  ASSERT_NE(vec_int64, nullptr) << "CSV integers are typically read as int64_t";

  const auto &vec = *vec_int64;
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

// Test INT64 CSV file reading
TEST_F(CSVReaderTest, ReadInt64Column) {
  std::string filepath = testdata_dir + "/test_int64.csv";
  std::vector<std::string> columns = {"value"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::AT_CSV,
                                    SUPPORTED_DATA_TYPE::AT_INT64, columns);

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

// Test FLOAT CSV file reading (Note: CSV typically reads floats as double)
TEST_F(CSVReaderTest, ReadFloatColumn) {
  std::string filepath = testdata_dir + "/test_float.csv";
  std::vector<std::string> columns = {"value"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::AT_CSV,
                                    SUPPORTED_DATA_TYPE::AT_FLOAT, columns);

  ASSERT_EQ(data.size(), 1);

  // CSV typically reads floating point as double
  auto *vec_double = std::get_if<std::vector<double>>(&data[0]);
  ASSERT_NE(vec_double, nullptr) << "CSV floats are typically read as double";

  const auto &vec = *vec_double;
  ASSERT_EQ(vec.size(), 10);

  EXPECT_DOUBLE_EQ(vec[0], 1.5);
  EXPECT_DOUBLE_EQ(vec[1], 2.5);
  EXPECT_DOUBLE_EQ(vec[2], 3.5);
  EXPECT_DOUBLE_EQ(vec[3], 4.5);
  EXPECT_DOUBLE_EQ(vec[4], 5.5);
  EXPECT_DOUBLE_EQ(vec[5], -1.5);
  EXPECT_DOUBLE_EQ(vec[6], -2.5);
  EXPECT_DOUBLE_EQ(vec[7], -3.5);
  EXPECT_DOUBLE_EQ(vec[8], 0.0);
  EXPECT_DOUBLE_EQ(vec[9], 10.5);
}

// Test DOUBLE CSV file reading
TEST_F(CSVReaderTest, ReadDoubleColumn) {
  std::string filepath = testdata_dir + "/test_double.csv";
  std::vector<std::string> columns = {"value"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::AT_CSV,
                                    SUPPORTED_DATA_TYPE::AT_DOUBLE, columns);

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

// Test multi-column CSV file reading
TEST_F(CSVReaderTest, ReadMultipleColumns) {
  std::string filepath = testdata_dir + "/test_multicolumn.csv";
  std::vector<std::string> columns = {"col_int32", "col_float", "col_double"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::AT_CSV,
                                    SUPPORTED_DATA_TYPE::AT_INT32, columns);

  ASSERT_EQ(data.size(), 3) << "Should return 3 columns";

  // Check col_int32 (will be int64 in CSV)
  auto *vec_int64 = std::get_if<std::vector<int64_t>>(&data[0]);
  ASSERT_NE(vec_int64, nullptr);
  ASSERT_EQ(vec_int64->size(), 10);
  EXPECT_EQ((*vec_int64)[0], 1);
  EXPECT_EQ((*vec_int64)[4], 5);

  // Check col_float (will be double in CSV)
  auto *vec_float = std::get_if<std::vector<double>>(&data[1]);
  ASSERT_NE(vec_float, nullptr);
  ASSERT_EQ(vec_float->size(), 10);
  EXPECT_DOUBLE_EQ((*vec_float)[0], 1.5);
  EXPECT_DOUBLE_EQ((*vec_float)[4], 5.5);

  // Check col_double
  auto *vec_double = std::get_if<std::vector<double>>(&data[2]);
  ASSERT_NE(vec_double, nullptr);
  ASSERT_EQ(vec_double->size(), 10);
  EXPECT_DOUBLE_EQ((*vec_double)[0], 1.123456);
  EXPECT_DOUBLE_EQ((*vec_double)[4], 5.567890);
}

// Test reading all columns in order
TEST_F(CSVReaderTest, ReadAllColumnsInOrder) {
  std::string filepath = testdata_dir + "/test_multicolumn.csv";
  std::vector<std::string> columns = {
      "col_int32", "col_int64", "col_float", "col_double"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::AT_CSV,
                                    SUPPORTED_DATA_TYPE::AT_INT32, columns);

  ASSERT_EQ(data.size(), 4) << "Should return 4 columns";

  // Verify we got all columns (CSV reads ints as int64, floats as double)
  EXPECT_NE(std::get_if<std::vector<int64_t>>(&data[0]), nullptr)
      << "col_int32 should be int64_t in CSV";
  EXPECT_NE(std::get_if<std::vector<int64_t>>(&data[1]), nullptr)
      << "col_int64 should be int64_t";
  EXPECT_NE(std::get_if<std::vector<double>>(&data[2]), nullptr)
      << "col_float should be double in CSV";
  EXPECT_NE(std::get_if<std::vector<double>>(&data[3]), nullptr)
      << "col_double should be double";
}

// Test reading single column from multi-column file
TEST_F(CSVReaderTest, ReadSingleColumnFromMulti) {
  std::string filepath = testdata_dir + "/test_multicolumn.csv";
  std::vector<std::string> columns = {"col_double"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::AT_CSV,
                                    SUPPORTED_DATA_TYPE::AT_DOUBLE, columns);

  ASSERT_EQ(data.size(), 1) << "Should return 1 column";

  auto *vec_ptr = std::get_if<std::vector<double>>(&data[0]);
  ASSERT_NE(vec_ptr, nullptr);
  EXPECT_EQ(vec_ptr->size(), 10);
}

// Test reading columns in different order
TEST_F(CSVReaderTest, ReadColumnsOutOfOrder) {
  std::string filepath = testdata_dir + "/test_multicolumn.csv";
  std::vector<std::string> columns = {"col_double", "col_int32"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::AT_CSV,
                                    SUPPORTED_DATA_TYPE::AT_INT32, columns);

  ASSERT_EQ(data.size(), 2) << "Should return 2 columns in requested order";

  // First requested column was col_double
  auto *vec_double = std::get_if<std::vector<double>>(&data[0]);
  ASSERT_NE(vec_double, nullptr);
  EXPECT_DOUBLE_EQ((*vec_double)[0], 1.123456);

  // Second requested column was col_int32
  auto *vec_int64 = std::get_if<std::vector<int64_t>>(&data[1]);
  ASSERT_NE(vec_int64, nullptr);
  EXPECT_EQ((*vec_int64)[0], 1);
}

// Test non-existent column
TEST_F(CSVReaderTest, ReadNonExistentColumn) {
  std::string filepath = testdata_dir + "/test_multicolumn.csv";
  std::vector<std::string> columns = {"nonexistent_column"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::AT_CSV,
                                    SUPPORTED_DATA_TYPE::AT_INT32, columns);

  EXPECT_TRUE(data.empty())
      << "Should return empty vector for non-existent column";
}

// Test non-existent file
TEST_F(CSVReaderTest, ReadNonExistentFile) {
  std::string filepath = testdata_dir + "/nonexistent.csv";
  std::vector<std::string> columns = {"value"};

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::AT_CSV,
                                    SUPPORTED_DATA_TYPE::AT_INT32, columns);

  EXPECT_TRUE(data.empty())
      << "Should return empty vector for non-existent file";
}

// Test empty CSV file (only header)
TEST_F(CSVReaderTest, EmptyCSVWithHeader) {
  std::string filepath = testdata_dir + "/empty_with_header.csv";
  std::vector<std::string> columns = {"value"};

  // Create CSV file with only header
  std::ofstream csv_file(filepath);
  csv_file << "value\n";
  csv_file.close();

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::AT_CSV,
                                    SUPPORTED_DATA_TYPE::AT_INT32, columns);

  if (!data.empty()) {
    // If implementation returns a column, it should be empty
    std::visit(
        [](auto &&vec) {
          EXPECT_TRUE(vec.empty()) << "Empty CSV should return empty vector";
        },
        data[0]);
  }

  // Cleanup
  std::filesystem::remove(filepath);
}

// Test CSV with mixed numeric types in same column
TEST_F(CSVReaderTest, MixedNumericTypes) {
  std::string filepath = testdata_dir + "/mixed_types.csv";
  std::vector<std::string> columns = {"value"};

  // Create CSV with mixed integers and floats
  std::ofstream csv_file(filepath);
  csv_file << "value\n";
  csv_file << "1\n";
  csv_file << "2.5\n";
  csv_file << "3\n";
  csv_file << "4.7\n";
  csv_file.close();

  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::AT_CSV,
                                    SUPPORTED_DATA_TYPE::AT_DOUBLE, columns);

  // Arrow CSV reader should infer as double
  if (!data.empty()) {
    auto *vec_double = std::get_if<std::vector<double>>(&data[0]);
    if (vec_double) {
      ASSERT_EQ(vec_double->size(), 4);
      EXPECT_DOUBLE_EQ((*vec_double)[0], 1.0);
      EXPECT_DOUBLE_EQ((*vec_double)[1], 2.5);
      EXPECT_DOUBLE_EQ((*vec_double)[2], 3.0);
      EXPECT_DOUBLE_EQ((*vec_double)[3], 4.7);
    }
  }

  // Cleanup
  std::filesystem::remove(filepath);
}

// Regression: a CSV larger than Arrow's read block size is returned as a
// multi-chunk column. The reader must concatenate every chunk; 
TEST_F(CSVReaderTest, MultiChunkLargeFile) {
  std::string filepath = testdata_dir + "/large_multichunk.csv";
  const int64_t kRows = 300000; // a few MB -> spans multiple read blocks

  {
    std::ofstream csv_file(filepath);
    csv_file << "value\n";
    for (int64_t i = 0; i < kRows; ++i) {
      csv_file << i << ".5\n"; // decimal point forces double inference
    }
  }

  std::vector<std::string> columns = {"value"};
  InputDataVector data = parse_file(filepath, SUPPORTED_FILE_TYPE::AT_CSV,
                                    SUPPORTED_DATA_TYPE::AT_DOUBLE, columns);

  ASSERT_EQ(data.size(), 1);
  auto *vec = std::get_if<std::vector<double>>(&data[0]);
  ASSERT_NE(vec, nullptr);
  ASSERT_EQ(vec->size(), static_cast<size_t>(kRows))
      << "All rows must be read across every Arrow block";
  EXPECT_DOUBLE_EQ(vec->front(), 0.5);
  EXPECT_DOUBLE_EQ((*vec)[kRows / 2], static_cast<double>(kRows / 2) + 0.5);
  EXPECT_DOUBLE_EQ(vec->back(), static_cast<double>(kRows - 1) + 0.5);

  std::filesystem::remove(filepath);
}
