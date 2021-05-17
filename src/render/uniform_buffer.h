#ifndef __uniform_buffer_h_
#define __uniform_buffer_h_

#include "def.h"

/**
 * \brief Uniform buffer structure
 */
struct uniform_buffer
{
public:
  /** World view projection matrix */
  glm::mat4 MatrWVP;

  /** Newer used padding */
  BYTE _Padding[256 - sizeof(glm::mat4)];

private:
  /**
   * \brief Compile-time test
   */
  static VOID PaddingTest( VOID )
  {
    static_assert(sizeof(uniform_buffer) == 256);
  }
};

#endif /* __uniform_buffer_h_ */
