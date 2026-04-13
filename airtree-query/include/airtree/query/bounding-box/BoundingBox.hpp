#ifndef AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_BOUNDING_BOX__BOUNDING_BOX_HPP
#define AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_BOUNDING_BOX__BOUNDING_BOX_HPP
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/meta/Histogram.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <tuple>
#include <variant>
#include <vector>

namespace airtree::query::bounding_box {

class BoundingBoxBinIndices2D {
public:
  BoundingBoxBinIndices2D() = default;
  BoundingBoxBinIndices2D(uint32_t min_x, uint32_t max_x, uint32_t min_y,
                          uint32_t max_y);
  uint32_t getMinX() const;
  uint32_t getMaxX() const;
  uint32_t getMinY() const;
  uint32_t getMaxY() const;

  void setMinX(uint32_t min_x);
  void setMaxX(uint32_t max_x);
  void setMinY(uint32_t min_y);
  void setMaxY(uint32_t max_y);


  void xMinMatchFlag();
  void xMaxMatchFlag();
  void yMinMatchFlag();
  void yMaxMatchFlag();

  bool isXMinMatch() const;
  bool isXMaxMatch() const;
  bool isYMinMatch() const;
  bool isYMaxMatch() const;

  bool isPerfectMatch() const;

private:
  uint8_t match_flag_ = 0; // Used to indicate if the box edges are a match
  uint32_t min_x_;
  uint32_t max_x_;
  uint32_t min_y_;
  uint32_t max_y_;
};

class BoundingBoxBinIndices3D {
public:
  BoundingBoxBinIndices3D() = default;
  BoundingBoxBinIndices3D(uint32_t min_x, uint32_t max_x, uint32_t min_y,
                          uint32_t max_y, uint32_t min_z, uint32_t max_z);
  uint32_t getMinX() const;
  uint32_t getMaxX() const;
  uint32_t getMinY() const;
  uint32_t getMaxY() const;
  uint32_t getMinZ() const;
  uint32_t getMaxZ() const;

  void setMinX(uint32_t min_x);
  void setMaxX(uint32_t max_x);
  void setMinY(uint32_t min_y);
  void setMaxY(uint32_t max_y);
  void setMinZ(uint32_t min_z);
  void setMaxZ(uint32_t max_z);

  void xMinMatchFlag();
  void xMaxMatchFlag();
  void yMinMatchFlag();
  void yMaxMatchFlag();
  void zMinMatchFlag();
  void zMaxMatchFlag();

  bool isXMinMatch() const;
  bool isXMaxMatch() const;
  bool isYMinMatch() const;
  bool isYMaxMatch() const;
  bool isZMinMatch() const;
  bool isZMaxMatch() const;

  bool isPerfectMatch() const;

private:
  uint8_t match_flag_ =
      0; // Used to indicate if the box edges are a match (6 bits for 3D)
  uint32_t min_x_;
  uint32_t max_x_;
  uint32_t min_y_;
  uint32_t max_y_;
  uint32_t min_z_;
  uint32_t max_z_;
};

// Represents the min and max coordinates for each dimension of a bounding
// box in 2D space.
// For example, (min_x, max_x) and (min_y, max_y) can be represented as
// std::tuple<double, double> for each dimension.
class BoundingBoxCoordinate2D {
public:
  BoundingBoxCoordinate2D() = default;
  BoundingBoxCoordinate2D(double min_x, double max_x, double min_y,
                          double max_y);
  double getMinX() const;
  double getMaxX() const;
  double getMinY() const;
  double getMaxY() const;

  void setMinX(double min_x);
  void setMaxX(double max_x);
  void setMinY(double min_y);
  void setMaxY(double max_y);

private:
  double min_x_;
  double max_x_;
  double min_y_;
  double max_y_;
};

// Represents the min and max coordinates for each dimension of a bounding
// box in 3D space.
class BoundingBoxCoordinate3D {
public:
  BoundingBoxCoordinate3D() = default;
  BoundingBoxCoordinate3D(double min_x, double max_x, double min_y,
                          double max_y, double min_z, double max_z);
  double getMinX() const;
  double getMaxX() const;
  double getMinY() const;
  double getMaxY() const;
  double getMinZ() const;
  double getMaxZ() const;

  void setMinX(double min_x);
  void setMaxX(double max_x);
  void setMinY(double min_y);
  void setMaxY(double max_y);
  void setMinZ(double min_z);
  void setMaxZ(double max_z);

private:
  double min_x_;
  double max_x_;
  double min_y_;
  double max_y_;
  double min_z_;
  double max_z_;
};

using BoxCoordinate2DResult = std::tuple<BoundingBoxCoordinate2D, uint32_t>;
// BoxCoordinate2DResultPair is a pair of results for the bounding box query,
// the first element is the bounding box coordinates and the count for the
// safe box, and the second element is the bounding box coordinates and the
// count for the total box => total box is the one that includes the edge box
// and the safe box.
using BoxCoordinate2DResultPair =
    std::pair<BoxCoordinate2DResult, BoxCoordinate2DResult>;

using BatchBoundingBoxInput = std::vector<BoundingBoxCoordinate2D>;
using BatchBoundingBoxResult = std::vector<BoxCoordinate2DResultPair>;

using BoxCoordinate3DResult = std::tuple<BoundingBoxCoordinate3D, uint32_t>;
// BoxCoordinate3DResultPair is a pair of results for the bounding box query,
// the first element is the bounding box coordinates and the count for the
// safe box, and the second element is the bounding box coordinates and the
// count for the total box => total box is the one that includes the edge box
// and the safe box.
using BoxCoordinate3DResultPair =
    std::pair<BoxCoordinate3DResult, BoxCoordinate3DResult>;

using BatchBoundingBoxInput3D = std::vector<BoundingBoxCoordinate3D>;
using BatchBoundingBoxResult3D = std::vector<BoxCoordinate3DResultPair>;

/**
** Represents a bounding box query for a histogram.
** The bounding box is defined by the minimum and maximum coordinates for each
** dimension. This class is used to query the histogram for data points that
** fall within the specified bounding box.
** The bounding box is constructed from a serialized histogram buffer.
*/
class BoundingBox {
public:
  BoundingBox(std::vector<char> buffer);

  BoxCoordinate2DResultPair getCounts(BoundingBoxCoordinate2D box) const;
  BoxCoordinate3DResultPair getCounts(BoundingBoxCoordinate3D box) const;

  BatchBoundingBoxResult
  getCountsBatch(const BatchBoundingBoxInput &queries) const;
  BatchBoundingBoxResult
  getCountsBatchSimple(const BatchBoundingBoxInput &queries) const;

  // This is used to get the counts for the bounding box with just xmin and
  // ymin as inputs. The max values are set to infinity, so the "bounding" box
  // is unbounded on one end.
  [[deprecated("Use getCounts() instead.")]]
  BoxCoordinate2DResultPair
  getCountsUnbounded(BoundingBoxCoordinate2D box) const;

private:
  BoxCoordinate2DResultPair
  calculateBoundingBox2D(const BoundingBoxCoordinate2D &box) const;

  BoxCoordinate3DResultPair
  calculateBoundingBox3D(const BoundingBoxCoordinate3D &box) const;

  void initSafeBox(BoundingBoxBinIndices2D &safe, double min_x, double min_y,
                   double max_x, double max_y) const;

  void initSafeBox3D(BoundingBoxBinIndices3D &safe, double min_x, double min_y,
                     double min_z, double max_x, double max_y,
                     double max_z) const;

  void initEdgeBox(BoundingBoxBinIndices2D &edge,
                   BoundingBoxBinIndices2D &safe) const;

  void initEdgeBox3D(BoundingBoxBinIndices3D &edge,
                     BoundingBoxBinIndices3D &safe) const;

  std::size_t offset_ = 0;
  int dims_ = 0;       // Number of dimensions
  int bit_length_ = 0; // Bit length of each dimension
  int bin_count_ = 0;  // Number of bins in the histogram
  std::unique_ptr<airtree::query::meta::Histogram> histogram_;
  AirTreeType trie_node_; // Trie node for bounding box
};

} // namespace airtree::query::bounding_box

#endif // AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_BOUNDING_BOX__BOUNDING_BOX_HPP