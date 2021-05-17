#ifndef __ray_h_
#define __ray_h_

#include "def.h"

/**
 * \brief Ray
 */
class ray
{
public:
  /** Ray direction */
  glm::vec3 Dir;

  /** Ray origin */
  glm::vec3 Org;

  /** Inverted ray direction */
  glm::vec3 InvDir;

  /**
   * \brief Constructor with initial value
   * \param[in] O Ray origin
   * \param[in] D Ray direction
   */
  ray( const glm::vec3 &O, const glm::vec3 &D );

  /**
   * \brief Get intersection position
   * \param[in] T Distance along ray
   * \return Intersection point
   */
  glm::vec3 operator()( FLT T ) const;
};

#endif /* __ray_h_ */
