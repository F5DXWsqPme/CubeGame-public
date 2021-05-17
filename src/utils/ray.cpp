#include "ray.h"

/**
 * \brief Constructor with initial value
 * \param[in] O Ray origin
 * \param[in] D Ray direction
 */
ray::ray( const glm::vec3 &O, const glm::vec3 &D ) : Dir(D), Org(O), InvDir(1 / D.x, 1 / D.y, 1 / D.z)
{
}

/**
 * \brief Get intersection position
 * \param[in] T Distance along ray
 * \return Intersection point
 */
glm::vec3 ray::operator()( FLT T ) const
{
  return Org + Dir * T;
}
