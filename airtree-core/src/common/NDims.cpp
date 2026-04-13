#include <airtree/core/common/NDims.hpp>


int getNumDims2D(unsigned int combinedTLE) {

  // Extract individual TLEs from the combinedTLE
  unsigned int tle1 = (combinedTLE >> 3) & 0x7; // Extract bits 3-5 for tle1
  unsigned int tle2 = combinedTLE & 0x7;        // Extract bits 0-2 for tle2

  // Check special conditions
  bool isTle1Special = (tle1 == 0 || tle1 == 1 || tle1 == 4 || tle1 == 7);
  bool isTle2Special = (tle2 == 0 || tle2 == 1 || tle2 == 4 || tle2 == 7);

  // Compute ndims based on the special conditions
  int ndims = (~((isTle1Special << 1) | isTle2Special)) & 0x3;

  return ndims;
}

int getNumDims3D(unsigned int combinedTLE) {
  // Extract individual TLEs from the combinedTLE
  unsigned int tle1 = (combinedTLE >> 6) & 0x7; // Extract bits 6-8 for tle1
  unsigned int tle2 = (combinedTLE >> 3) & 0x7; // Extract bits 3-5 for tle2
  unsigned int tle3 = combinedTLE & 0x7;        // Extract bits 0-2 for tle3

  // Check special conditions
  bool isTle1Special = (tle1 == 0 || tle1 == 1 || tle1 == 4 || tle1 == 7);
  bool isTle2Special = (tle2 == 0 || tle2 == 1 || tle2 == 4 || tle2 == 7);
  bool isTle3Special = (tle3 == 0 || tle3 == 1 || tle3 == 4 || tle3 == 7);

  // Compute ndims based on the special conditions
  int ndims =
      (~((isTle1Special << 2) | (isTle2Special << 1) | isTle3Special)) & 0x7;

  return ndims;
}

int getNumDims4D(unsigned int combinedTLE) {
  // Extract individual TLEs from the combinedTLE
  unsigned int tle1 = (combinedTLE >> 9) & 0x7; // Extract bits 9-11 for tle1
  unsigned int tle2 = (combinedTLE >> 6) & 0x7; // Extract bits 6-8 for tle1
  unsigned int tle3 = (combinedTLE >> 3) & 0x7; // Extract bits 3-5 for tle2
  unsigned int tle4 = combinedTLE & 0x7;        // Extract bits 0-2 for tle3

  // Check special conditions
  bool isTle1Special = (tle1 == 0 || tle1 == 1 || tle1 == 4 || tle1 == 7);
  bool isTle2Special = (tle2 == 0 || tle2 == 1 || tle2 == 4 || tle2 == 7);
  bool isTle3Special = (tle3 == 0 || tle3 == 1 || tle3 == 4 || tle3 == 7);
  bool isTle4Special = (tle4 == 0 || tle4 == 1 || tle4 == 4 || tle4 == 7);

  // Compute ndims based on the special conditions
  int ndims = (~((isTle1Special << 3) | (isTle2Special << 2)
                 | (isTle3Special << 1) | isTle4Special << 0))
              & 0xF;

  return ndims;
}