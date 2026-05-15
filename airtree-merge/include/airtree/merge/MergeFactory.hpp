// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_MERGEFACTORY_HPP
#define AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_MERGEFACTORY_HPP

#include <memory>
#include "MergeStrategy.hpp"

namespace airtree::merge {

enum class MergeConfig {
  Dx1_T,
  Dx1_F,
  Dx1_P,
  Dx2_F,
  Dx2_P,
  Dx3_F,
  Dx3_P,
  Dx4_F,
  Dx4_P,
};

class MergeFactory {
public:
  static std::unique_ptr<MergeStrategy> create(MergeConfig config);
};

} // namespace airtree::merge

#endif // AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_MERGEFACTORY_HPP
