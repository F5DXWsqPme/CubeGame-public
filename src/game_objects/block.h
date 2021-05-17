#ifndef __block_h_
#define __block_h_

#include "def.h"

/**
 * \brief Block description
 */
struct BLOCK
{
  /** Block type identifier */
  UINT32 BlockTypeId = 0;

  /** Block direction */
  glm::ivec3 Direction = glm::ivec3(1, 0, 0);

  /** Block right direction */
  glm::ivec3 Right = glm::ivec3(0, 0, 1);

  /** Block up direction */
  glm::ivec3 Up = glm::ivec3(0, 1, 0);
};

#endif /* __block_h_ */
