// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_EXPORT_ARROWTABLEBUILDER_HPP
#define AIRTREE_EXPORT_ARROWTABLEBUILDER_HPP


#include <arrow/table.h>
#include <airtree/query/AirTreeQuery_internal.hpp>

using namespace airtree::query::bin_boundary;

// Cannot use the word export as a namespace name - its a C++ keyword
namespace airtree::xport::core {

template <typename List, typename GetCount, typename... LowerFns,
          typename... UpperFns>
std::shared_ptr<arrow::Table> arrowTableBuilder(const List &list,
                                                GetCount getCount,
                                                std::tuple<LowerFns...> lowers,
                                                std::tuple<UpperFns...> uppers);

std::shared_ptr<arrow::Table> toArrowTable(const BinBoundary1DList &list);
std::shared_ptr<arrow::Table> toArrowTable(const BinBoundary2DList &list);
std::shared_ptr<arrow::Table> toArrowTable(const BinBoundary3DList &list);
std::shared_ptr<arrow::Table> toArrowTable(const BinBoundary4DList &list);


} // namespace airtree::xport::core


#endif // AIRTREE_EXPORT_ARROWTABLEBUILDER_HPP