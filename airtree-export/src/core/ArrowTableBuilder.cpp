// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/export/core/ArrowTableBuilder.hpp>
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#include <airtree/export/Logger.hpp>

using namespace airtree::xport;

namespace airtree::xport::core {


template <typename List, typename GetCount, typename... LowerFns,
          typename... UpperFns>
std::shared_ptr<arrow::Table>
arrowTableBuilder(const List &list, GetCount getCount,
                  std::tuple<LowerFns...> lowers,
                  std::tuple<UpperFns...> uppers) {

  static_assert(
      sizeof...(LowerFns) == sizeof...(UpperFns), "lower/upper arity mismatch");
  constexpr int N = sizeof...(LowerFns);
  static_assert(N >= 1 && N <= 4, "Only 1..4D supported");

  auto throw_if = [](const arrow::Status &st, const char *what) {
    if (!st.ok())
      throw std::runtime_error(std::string(what) + ": " + st.ToString());
  };

  std::array<arrow::DoubleBuilder, N> mins;
  std::array<arrow::DoubleBuilder, N> maxs;
  arrow::UInt32Builder counts;

  const int64_t nrows = static_cast<int64_t>(list.size());
  for (int d = 0; d < N; ++d) {
    throw_if(mins[d].Reserve(nrows), "Reserve mins");
    throw_if(maxs[d].Reserve(nrows), "Reserve maxs");
  }
  throw_if(counts.Reserve(nrows), "Reserve counts");

  // Per-row append with compile-time unrolling across dimensions
  for (const auto &b : list) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
      (throw_if(
           mins[I].Append(std::invoke(std::get<I>(lowers), b)), "Append min"),
       ...);
      (throw_if(
           maxs[I].Append(std::invoke(std::get<I>(uppers), b)), "Append max"),
       ...);
    }(std::make_index_sequence<N>{});
    throw_if(counts.Append(std::invoke(getCount, b)), "Append count");
  }

  // Finish and assemble
  std::vector<std::shared_ptr<arrow::Field>> fields;
  std::vector<std::shared_ptr<arrow::Array>> cols;
  fields.reserve(N * 2 + 1);
  cols.reserve(N * 2 + 1);

  const char labels[4] = {'x', 'y', 'z', 'w'};
  for (int d = 0; d < N; ++d) {
    std::shared_ptr<arrow::Array> a_min, a_max;
    auto st = mins[d].Finish(&a_min);
    if (!st.ok()) {
      SPDLOG_LOGGER_ERROR(
          logger(), "Failed to build {}-min: {}", labels[d], st.ToString());
      throw std::runtime_error(std::string("Failed to build ") + labels[d]
                               + "-min: " + st.ToString());
    }
    st = maxs[d].Finish(&a_max);
    if (!st.ok()) {
      SPDLOG_LOGGER_ERROR(
          logger(), "Failed to build {}-max: {}", labels[d], st.ToString());
      throw std::runtime_error(std::string("Failed to build ") + labels[d]
                               + "-max: " + st.ToString());
    }
    fields.push_back(
        arrow::field(std::string(1, labels[d]) + "-min", arrow::float64()));
    fields.push_back(
        arrow::field(std::string(1, labels[d]) + "-max", arrow::float64()));
    cols.push_back(std::move(a_min));
    cols.push_back(std::move(a_max));
  }

  std::shared_ptr<arrow::Array> a_counts;
  auto stc = counts.Finish(&a_counts);
  if (!stc.ok()) {
    SPDLOG_LOGGER_ERROR(logger(), "Failed to build counts: {}", stc.ToString());
    throw std::runtime_error("Failed to build counts: " + stc.ToString());
  }
  fields.push_back(arrow::field("counts", arrow::uint32()));
  cols.push_back(std::move(a_counts));

  return arrow::Table::Make(arrow::schema(fields), cols);
}

// 1-D handler
std::shared_ptr<arrow::Table> toArrowTable(const BinBoundary1DList &list) {
  using B = typename BinBoundary1DList::value_type;
  return arrowTableBuilder(list, &B::getCount,
                           std::make_tuple(&B::getLowerBound),
                           std::make_tuple(&B::getUpperBound));
}

// 2-D handler
std::shared_ptr<arrow::Table> toArrowTable(const BinBoundary2DList &list) {
  using B = typename BinBoundary2DList::value_type;
  return arrowTableBuilder(
      list, &B::getCount,
      std::make_tuple(&B::getLowerBoundX, &B::getLowerBoundY),
      std::make_tuple(&B::getUpperBoundX, &B::getUpperBoundY));
}

// 3-D handler
std::shared_ptr<arrow::Table> toArrowTable(const BinBoundary3DList &list) {
  using B = typename BinBoundary3DList::value_type;
  return arrowTableBuilder(
      list, &B::getCount,
      std::make_tuple(
          &B::getLowerBoundX, &B::getLowerBoundY, &B::getLowerBoundZ),
      std::make_tuple(
          &B::getUpperBoundX, &B::getUpperBoundY, &B::getUpperBoundZ));
}

// 4-D handler
std::shared_ptr<arrow::Table> toArrowTable(const BinBoundary4DList &list) {
  using B = typename BinBoundary4DList::value_type;
  return arrowTableBuilder(
      list, &B::getCount,
      std::make_tuple(&B::getLowerBoundX, &B::getLowerBoundY,
                      &B::getLowerBoundZ, &B::getLowerBoundW),
      std::make_tuple(&B::getUpperBoundX, &B::getUpperBoundY,
                      &B::getUpperBoundZ, &B::getUpperBoundW));
}

} // namespace airtree::xport::core