#ifndef AIRTREE_CORE_AirTreeCore_INTERNAL_HPP
#define AIRTREE_CORE_AirTreeCore_INTERNAL_HPP

// ─────────────────────────────────────────────────────────────────────────────
// Common utilities / data structures
// ─────────────────────────────────────────────────────────────────────────────
#include <airtree/core/common/Bins.hpp>
#include <airtree/core/common/BitCodec.hpp>
#include <airtree/core/common/BitInterleave.hpp>
#include <airtree/core/common/BooleanArray.hpp>
#include <airtree/core/common/ChunkTypes.hpp>
#include <airtree/core/common/Conversion.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/NDims.hpp>
#include <airtree/core/common/NDNode.hpp>
#include <airtree/core/common/Reconstruct.hpp>
#include <airtree/core/common/SpecialCounts.hpp>
#include <airtree/core/common/TLE.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/AirTreeType.hpp>


#include <airtree/core/io/AirTreeWriter.hpp>
#include <airtree/core/io/AirTreeReader.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Schema: 1D
// ─────────────────────────────────────────────────────────────────────────────
#include <airtree/core/schema/trie1d/1DxT.hpp>
#include <airtree/core/schema/trie1d/1DxP.hpp>
#include <airtree/core/schema/trie1d/1DxF.hpp>

// (Add trie2d/trie3d/trie4d schema headers here when they exist)

// ─────────────────────────────────────────────────────────────────────────────
// SerDes: helpers & generic pieces
// ─────────────────────────────────────────────────────────────────────────────
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/EOF.hpp>
#include <airtree/core/serdes/ND.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// SerDes: 1D
// ─────────────────────────────────────────────────────────────────────────────
#include <airtree/core/serdes/trie1d/1DxF.hpp>
#include <airtree/core/serdes/trie1d/1DxP.hpp>
#include <airtree/core/serdes/trie1d/1DxT.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Schema & SerDes: 2D
// ─────────────────────────────────────────────────────────────────────────────
#include <airtree/core/schema/trie2d/2DxF.hpp>
#include <airtree/core/schema/trie2d/2DxP.hpp>

#include <airtree/core/serdes/trie2d/2DxF.hpp>
#include <airtree/core/serdes/trie2d/2DxP.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Schema & SerDes: 3D
// ─────────────────────────────────────────────────────────────────────────────
#include <airtree/core/schema/trie3d/3DxF.hpp>
#include <airtree/core/schema/trie3d/3DxP.hpp>

#include <airtree/core/serdes/trie3d/3DxF.hpp>
#include <airtree/core/serdes/trie3d/3DxP.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Schema & SerDes: 4D
// ─────────────────────────────────────────────────────────────────────────────
#include <airtree/core/schema/trie4d/4DxP.hpp>
#include <airtree/core/schema/trie4d/4DxF.hpp>

#include <airtree/core/serdes/trie4d/4DxP.hpp>
#include <airtree/core/serdes/trie4d/4DxF.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Core API
// ─────────────────────────────────────────────────────────────────────────────
#include <airtree/core/api/AirTreeGenerator.hpp>

#endif // AIRTREE_CORE_AirTreeCore_INTERNAL_HPP
