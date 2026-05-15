// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <cmath>
#include <gtest/gtest.h>
#include <airtree/export/core/ArrowTableBuilder.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>
#include <arrow/api.h>

using airtree::xport::core::toArrowTable;

// --- Helpers to read Arrow columns into std::vector ---
static std::vector<double> ReadDoubleCol(const std::shared_ptr<arrow::Table> &t,
                                         int col) {
  auto chunked = t->column(col);
  EXPECT_EQ(chunked->num_chunks(), 1) << "These tests expect a single chunk";
  auto arr = std::static_pointer_cast<arrow::DoubleArray>(chunked->chunk(0));
  std::vector<double> v(arr->length());
  for (int64_t i = 0; i < arr->length(); ++i)
    v[i] = arr->Value(i);
  return v;
}

static std::vector<uint32_t>
ReadUInt32Col(const std::shared_ptr<arrow::Table> &t, int col) {
  auto chunked = t->column(col);
  EXPECT_EQ(chunked->num_chunks(), 1);
  auto arr = std::static_pointer_cast<arrow::UInt32Array>(chunked->chunk(0));
  std::vector<uint32_t> v(arr->length());
  for (int64_t i = 0; i < arr->length(); ++i)
    v[i] = arr->Value(i);
  return v;
}

static void ExpectField(const std::shared_ptr<arrow::Schema> &s, int idx,
                        const std::string &name, arrow::Type::type id) {
  ASSERT_LT(idx, s->num_fields());
  EXPECT_EQ(s->field(idx)->name(), name);
  EXPECT_EQ(s->field(idx)->type()->id(), id);
}

//------------------ Helpers for debugging ------------------

template <class T>
static void DumpVec(const char *label, const std::vector<T> &v) {
  std::cout << label << " = [";
  for (size_t i = 0; i < v.size(); ++i) {
    if (i)
      std::cout << ", ";
    std::cout << v[i];
  }
  std::cout << "]\n";
}

static void DumpSchema(const std::shared_ptr<arrow::Table> &t) {
  auto s = t->schema();
  std::cout << "schema: ";
  for (int i = 0; i < t->num_columns(); ++i) {
    std::cout << "[" << i << "] " << s->field(i)->name() << " ";
  }
  std::cout << "\n";
}

// ---- Constructors for BinBoundary*D  ----
static airtree::query::bin_boundary::BinBoundary1D B1(double lo, double hi,
                                                      uint32_t c) {
  return airtree::query::bin_boundary::BinBoundary1D(lo, hi, c);
}
static airtree::query::bin_boundary::BinBoundary2D
B2(double lx, double ux, double ly, double uy, uint32_t c) {
  return airtree::query::bin_boundary::BinBoundary2D(lx, ly, ux, uy, c);
}
static airtree::query::bin_boundary::BinBoundary3D B3(double lx, double ux,
                                                      double ly, double uy,
                                                      double lz, double uz,
                                                      uint32_t c) {
  return airtree::query::bin_boundary::BinBoundary3D(lx, ly, lz, ux, uy, uz, c);
}
static airtree::query::bin_boundary::BinBoundary4D
B4(double lx, double ux, double ly, double uy, double lz, double uz, double lw,
   double uw, uint32_t c) {
  return airtree::query::bin_boundary::BinBoundary4D(
      lx, ly, lz, lw, ux, uy, uz, uw, c);
}


// -------------------------- TESTS --------------------------

TEST(ToArrowTable, OneD_BasicValuesAndSchema) {
  airtree::query::bin_boundary::BinBoundary1DList list{
      B1(0.0, 1.0, 5),
      B1(1.0, 2.0, 7),
      B1(2.0, 3.0, 9),
  };

  auto table = toArrowTable(list);
  ASSERT_NE(table, nullptr);

  //   DumpSchema(table);
  auto xmins = ReadDoubleCol(table, 0);
  auto xmaxs = ReadDoubleCol(table, 1);
  auto counts = ReadUInt32Col(table, 2);
  //   DumpVec("x-min", xmins);
  //   DumpVec("x-max", xmaxs);
  //   DumpVec("counts", counts);
  //   std::cout << std::flush;

  EXPECT_EQ(table->num_rows(), 3);
  EXPECT_EQ(table->num_columns(), 3);

  auto schema = table->schema();
  ExpectField(schema, 0, "x-min", arrow::Type::DOUBLE);
  ExpectField(schema, 1, "x-max", arrow::Type::DOUBLE);
  ExpectField(schema, 2, "counts", arrow::Type::UINT32);

  EXPECT_EQ(xmins, (std::vector<double>{0.0, 1.0, 2.0}));
  EXPECT_EQ(xmaxs, (std::vector<double>{1.0, 2.0, 3.0}));
  EXPECT_EQ(counts, (std::vector<uint32_t>{5, 7, 9}));
}


TEST(ToArrowTable, TwoD_BasicValuesAndSchema) {
  airtree::query::bin_boundary::BinBoundary2DList list{
      B2(0.0, 1.0, 10.0, 20.0, 3),
      B2(1.0, 2.0, 20.0, 30.0, 4),
  };

  auto table = toArrowTable(list);
  ASSERT_NE(table, nullptr);
  EXPECT_EQ(table->num_rows(), 2);
  EXPECT_EQ(table->num_columns(), 5);


  //   DumpSchema(table);

  auto xmins = ReadDoubleCol(table, 0);
  auto xmaxs = ReadDoubleCol(table, 1);
  auto ymins = ReadDoubleCol(table, 2);
  auto ymaxs = ReadDoubleCol(table, 3);
  auto counts = ReadUInt32Col(table, 4);

  //   DumpVec("x-min", xmins);
  //   DumpVec("x-max", xmaxs);
  //   DumpVec("y-min", ymins);
  //   DumpVec("y-max", ymaxs);
  //   DumpVec("counts", counts);
  //   std::cout << std::flush;

  auto schema = table->schema();
  ExpectField(schema, 0, "x-min", arrow::Type::DOUBLE);
  ExpectField(schema, 1, "x-max", arrow::Type::DOUBLE);
  ExpectField(schema, 2, "y-min", arrow::Type::DOUBLE);
  ExpectField(schema, 3, "y-max", arrow::Type::DOUBLE);
  ExpectField(schema, 4, "counts", arrow::Type::UINT32);

  EXPECT_EQ(xmins, (std::vector<double>{0.0, 1.0}));
  EXPECT_EQ(xmaxs, (std::vector<double>{1.0, 2.0}));
  EXPECT_EQ(ymins, (std::vector<double>{10.0, 20.0}));
  EXPECT_EQ(ymaxs, (std::vector<double>{20.0, 30.0}));
  EXPECT_EQ(counts, (std::vector<uint32_t>{3, 4}));
}


TEST(ToArrowTable, ThreeD_BasicValuesAndSchema) {
  airtree::query::bin_boundary::BinBoundary3DList list{
      B3(0, 1, 10, 20, 100, 200, 1),
      B3(1, 2, 20, 30, 200, 300, 2),
  };

  auto table = toArrowTable(list);
  ASSERT_NE(table, nullptr);
  EXPECT_EQ(table->num_rows(), 2);
  EXPECT_EQ(table->num_columns(), 7);

  auto schema = table->schema();
  ExpectField(schema, 0, "x-min", arrow::Type::DOUBLE);
  ExpectField(schema, 1, "x-max", arrow::Type::DOUBLE);
  ExpectField(schema, 2, "y-min", arrow::Type::DOUBLE);
  ExpectField(schema, 3, "y-max", arrow::Type::DOUBLE);
  ExpectField(schema, 4, "z-min", arrow::Type::DOUBLE);
  ExpectField(schema, 5, "z-max", arrow::Type::DOUBLE);
  ExpectField(schema, 6, "counts", arrow::Type::UINT32);

  EXPECT_EQ(ReadDoubleCol(table, 0), (std::vector<double>{0, 1}));
  EXPECT_EQ(ReadDoubleCol(table, 1), (std::vector<double>{1, 2}));
  EXPECT_EQ(ReadDoubleCol(table, 2), (std::vector<double>{10, 20}));
  EXPECT_EQ(ReadDoubleCol(table, 3), (std::vector<double>{20, 30}));
  EXPECT_EQ(ReadDoubleCol(table, 4), (std::vector<double>{100, 200}));
  EXPECT_EQ(ReadDoubleCol(table, 5), (std::vector<double>{200, 300}));
  EXPECT_EQ(ReadUInt32Col(table, 6), (std::vector<uint32_t>{1, 2}));
}

TEST(ToArrowTable, FourD_BasicValuesAndSchema) {
  airtree::query::bin_boundary::BinBoundary4DList list{
      B4(0, 1, 10, 20, 100, 200, 1000, 2000, 11),
      B4(1, 2, 20, 30, 200, 300, 2000, 3000, 22),
  };

  auto table = toArrowTable(list);
  ASSERT_NE(table, nullptr);
  EXPECT_EQ(table->num_rows(), 2);
  EXPECT_EQ(table->num_columns(), 9);

  auto schema = table->schema();
  ExpectField(schema, 0, "x-min", arrow::Type::DOUBLE);
  ExpectField(schema, 1, "x-max", arrow::Type::DOUBLE);
  ExpectField(schema, 2, "y-min", arrow::Type::DOUBLE);
  ExpectField(schema, 3, "y-max", arrow::Type::DOUBLE);
  ExpectField(schema, 4, "z-min", arrow::Type::DOUBLE);
  ExpectField(schema, 5, "z-max", arrow::Type::DOUBLE);
  ExpectField(schema, 6, "w-min", arrow::Type::DOUBLE);
  ExpectField(schema, 7, "w-max", arrow::Type::DOUBLE);
  ExpectField(schema, 8, "counts", arrow::Type::UINT32);

  EXPECT_EQ(ReadDoubleCol(table, 0), (std::vector<double>{0, 1}));
  EXPECT_EQ(ReadDoubleCol(table, 1), (std::vector<double>{1, 2}));
  EXPECT_EQ(ReadDoubleCol(table, 2), (std::vector<double>{10, 20}));
  EXPECT_EQ(ReadDoubleCol(table, 3), (std::vector<double>{20, 30}));
  EXPECT_EQ(ReadDoubleCol(table, 4), (std::vector<double>{100, 200}));
  EXPECT_EQ(ReadDoubleCol(table, 5), (std::vector<double>{200, 300}));
  EXPECT_EQ(ReadDoubleCol(table, 6), (std::vector<double>{1000, 2000}));
  EXPECT_EQ(ReadDoubleCol(table, 7), (std::vector<double>{2000, 3000}));
  EXPECT_EQ(ReadUInt32Col(table, 8), (std::vector<uint32_t>{11, 22}));
}

TEST(ToArrowTable, EmptyLists_ProduceZeroRowsWithExpectedSchema) {
  {
    airtree::query::bin_boundary::BinBoundary1DList list{};
    auto table = toArrowTable(list);
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(table->num_rows(), 0);
    ExpectField(table->schema(), 0, "x-min", arrow::Type::DOUBLE);
    ExpectField(table->schema(), 1, "x-max", arrow::Type::DOUBLE);
    ExpectField(table->schema(), 2, "counts", arrow::Type::UINT32);
  }
  {
    airtree::query::bin_boundary::BinBoundary2DList list{};
    auto table = toArrowTable(list);
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(table->num_rows(), 0);
    ExpectField(table->schema(), 0, "x-min", arrow::Type::DOUBLE);
    ExpectField(table->schema(), 1, "x-max", arrow::Type::DOUBLE);
    ExpectField(table->schema(), 2, "y-min", arrow::Type::DOUBLE);
    ExpectField(table->schema(), 3, "y-max", arrow::Type::DOUBLE);
    ExpectField(table->schema(), 4, "counts", arrow::Type::UINT32);
  }
}
