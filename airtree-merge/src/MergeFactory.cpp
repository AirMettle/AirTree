#include <stdexcept>

#include <airtree/merge/MergeFactory.hpp>
#include <airtree/merge/MergeStrategy.hpp>
#include <airtree/merge/trie1d/Merge1DxT.hpp>
#include <airtree/merge/trie1d/Merge1DxF.hpp>
#include <airtree/merge/trie1d/Merge1DxP.hpp>
#include <airtree/merge/trie2d/Merge2DxF.hpp>
#include <airtree/merge/trie2d/Merge2DxP.hpp>
#include <airtree/merge/trie3d/Merge3DxF.hpp>
#include <airtree/merge/trie3d/Merge3DxP.hpp>
#include <airtree/merge/trie4d/Merge4DxF.hpp>
#include <airtree/merge/trie4d/Merge4DxP.hpp>

using namespace airtree::merge;
using namespace airtree::merge::dim1;
using namespace airtree::merge::dim2;
using namespace airtree::merge::dim3;
using namespace airtree::merge::dim4;

std::unique_ptr<MergeStrategy> MergeFactory::create(MergeConfig config) {
  switch (config) {
  case MergeConfig::Dx1_T:
    return std::make_unique<Merge1DxT>();
  case MergeConfig::Dx1_F:
    return std::make_unique<Merge1DxF>();
  case MergeConfig::Dx1_P:
    return std::make_unique<Merge1DxP>();
  case MergeConfig::Dx2_F:
    return std::make_unique<Merge2DxF>();
  case MergeConfig::Dx2_P:
    return std::make_unique<Merge2DxP>();
  case MergeConfig::Dx3_F:
    return std::make_unique<Merge3DxF>();
  case MergeConfig::Dx3_P:
    return std::make_unique<Merge3DxP>();
  case MergeConfig::Dx4_F:
    return std::make_unique<Merge4DxF>();
  case MergeConfig::Dx4_P:
    return std::make_unique<Merge4DxP>();
  default:
    throw std::runtime_error("Unsupported config");
  }
}
