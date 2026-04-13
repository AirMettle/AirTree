#ifndef AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_BIN_BOUNDARY_BIN_BOUNDARY_HPP
#define AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_BIN_BOUNDARY_BIN_BOUNDARY_HPP

#include <airtree/core/common/SpecialCounts.hpp>
#include <airtree/core/common/TrieHeader.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>

#include <memory>
#include <string>
#include <vector>
#include <variant>

namespace airtree::query::bin_boundary {

class BinBoundary1D {
public:
  BinBoundary1D(double min, double max, uint32_t count);

  double getLowerBound() const;
  double getUpperBound() const;
  uint32_t getCount() const;

  void setLowerBound(double lower);
  void setUpperBound(double upper);
  void setCount(uint32_t count);

private:
  double min_;
  double max_;
  uint32_t count_;
};

class BinBoundary2D {
public:
  BinBoundary2D(double min_x, double min_y, double max_x, double max_y,
                uint32_t count);

  double getLowerBoundX() const;
  double getUpperBoundX() const;
  double getLowerBoundY() const;
  double getUpperBoundY() const;
  uint32_t getCount() const;

  void setLowerBoundX(double lower_x);
  void setUpperBoundX(double upper_x);
  void setLowerBoundY(double lower_y);
  void setUpperBoundY(double upper_y);
  void setCount(uint32_t count);

private:
  double min_x_;
  double min_y_;
  double max_x_;
  double max_y_;
  uint32_t count_;
};

class BinBoundary3D {
public:
  BinBoundary3D(double min_x, double min_y, double min_z, double max_x,
                double max_y, double max_z, uint32_t count);

  double getLowerBoundX() const;
  double getUpperBoundX() const;
  double getLowerBoundY() const;
  double getUpperBoundY() const;
  double getLowerBoundZ() const;
  double getUpperBoundZ() const;
  uint32_t getCount() const;

  void setLowerBoundX(double lower_x);
  void setUpperBoundX(double upper_x);
  void setLowerBoundY(double lower_y);
  void setUpperBoundY(double upper_y);
  void setLowerBoundZ(double lower_z);
  void setUpperBoundZ(double upper_z);
  void setCount(uint32_t count);

private:
  double min_x_;
  double min_y_;
  double min_z_;
  double max_x_;
  double max_y_;
  double max_z_;
  uint32_t count_;
};

class BinBoundary4D {
public:
  BinBoundary4D(double min_x, double min_y, double min_z, double min_w,
                double max_x, double max_y, double max_z, double max_w,
                uint32_t count);
  double getLowerBoundX() const;
  double getUpperBoundX() const;
  double getLowerBoundY() const;
  double getUpperBoundY() const;
  double getLowerBoundZ() const;
  double getUpperBoundZ() const;
  double getLowerBoundW() const;
  double getUpperBoundW() const;
  uint32_t getCount() const;

  void setLowerBoundX(double lower_x);
  void setUpperBoundX(double upper_x);
  void setLowerBoundY(double lower_y);
  void setUpperBoundY(double upper_y);
  void setLowerBoundZ(double lower_z);
  void setUpperBoundZ(double upper_z);
  void setLowerBoundW(double lower_w);
  void setUpperBoundW(double upper_w);
  void setCount(uint32_t count);

private:
  double min_x_;
  double min_y_;
  double min_z_;
  double min_w_;
  double max_x_;
  double max_y_;
  double max_z_;
  double max_w_;
  uint32_t count_;
};

using BinBoundary1DList = std::vector<BinBoundary1D>;
using BinBoundary2DList = std::vector<BinBoundary2D>;
using BinBoundary3DList = std::vector<BinBoundary3D>;
using BinBoundary4DList = std::vector<BinBoundary4D>;

class BinBoundaryResult {
public:
  BinBoundaryResult(std::unique_ptr<SpecialCounts> specialCounts,
                    std::variant<BinBoundary1DList, BinBoundary2DList,
                                 BinBoundary3DList, BinBoundary4DList>
                        boundaries);
  std::unique_ptr<SpecialCounts> getSpecialCounts() const;
  std::unique_ptr<std::variant<BinBoundary1DList, BinBoundary2DList,
                               BinBoundary3DList, BinBoundary4DList>>
  getBoundaries() const;

private:
  std::unique_ptr<SpecialCounts> specialCounts_;
  std::variant<BinBoundary1DList, BinBoundary2DList, BinBoundary3DList,
               BinBoundary4DList>
      boundaries_;
};

/* Generates bin boundaries for a given buffer. This will be the public
** interface for all buffers so it is the responsibility of this
** class to ensure it processes only types we support.
** Accepts the serialized buffer of the trie and
** returns a BinBoundaryResult.
** @param buffer The serialized buffer of the trie.
** @return Vector of tuples representing the bin boundaries
** and the corresponding counts.
*/
class BinBoundary {
public:
  BinBoundary(std::vector<char> buffer);

  [[nodiscard]] BinBoundaryResult generateBinBoundaries();

private:
  BinBoundary1DList buildBinBoundaries1DxT();
  BinBoundary1DList buildBinBoundaries1DxF();
  BinBoundary1DList buildBinBoundaries1DxP();
  BinBoundary2DList buildBinBoundaries2DxP();
  BinBoundary3DList buildBinBoundaries3DxP();
  BinBoundary4DList buildBinBoundaries4DxP();

  std::unique_ptr<SpecialCounts> specialCounts_;
  std::unique_ptr<trie_header> header_;
  size_t offset_ = 0;
  std::vector<char> buffer_;
};

} // namespace airtree::query::bin_boundary

#endif // AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_BIN_BOUNDARY_BIN_BOUNDARY_HPP