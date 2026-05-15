// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SERDES_ND_SER_HPP
#define AIRTREE_CORE_SERDES_ND_SER_HPP


#include <airtree/core/schema/trie1d/1DxF.hpp>


void serializeTrieNode_16_ND(const TrieNode_16 *node, std::vector<char> &buffer,
                             bool recursively = true);


#endif // AIRTREE_CORE_SERDES_ND_SER_HPP
