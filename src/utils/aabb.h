#ifndef __aabb_h_
#define __aabb_h_

#include "ray.h"

/**
 * \brief Axis aligned bounding box
 */
class aabb
{
private:
  /** Minimal coordinate */
  glm::vec3 Min;

  /** Maximal coordinate */
  glm::vec3 Max;

public:
  /**
   * \brief Bounding box constructor
   * \param[in] Min Minimal coordinate
   * \param[in] Max Maximal coordinate
   */
  aabb( const glm::vec3 &Min, const glm::vec3 &Max );

  /**
   * \brief Intersection with ray function
   * \param[in] R Ray
   * \param[in, out] T Distance to nearest intersection
   * \return TRUE-if intersect, FALSE-if otherwise
   */
  BOOL Intersect( const ray &R, FLT *T ) const;
};


#endif /* __aabb_h_ */
