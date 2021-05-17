#ifndef __block_type_h_
#define __block_type_h_

#include <vector>

#include "def.h"

/**
 * \brief Struct with block type description
 */
struct BLOCK_TYPE
{
  /** Block transparency */
  FLT Alpha = 1;

  /** Up texture coords */
  glm::vec2 TexCoordUp[4];

  /** Right texture coords */
  glm::vec2 TexCoordRight[4];

  /** Left texture coords */
  glm::vec2 TexCoordLeft[4];

  /** Down texture coords */
  glm::vec2 TexCoordDown[4];

  /** Front texture coords */
  glm::vec2 TexCoordFront[4];

  /** Back texture coords */
  glm::vec2 TexCoordBack[4];

  /**
   * \brief Block type constructor
   * \param Alpha Block transparency
   */
  BLOCK_TYPE( FLT Alpha );

  /** Block types table */
  static std::vector<BLOCK_TYPE> Table;

  /** Stone type identifier */
  static const UINT32 StoneId;

  /** Grass type identifier */
  static const UINT32 GrassId;

  /** Air type identifier */
  static const UINT32 AirId;

  /** Glass type identifier */
  static const UINT32 GlassId;
};

#endif /* __block_type_h_ */
