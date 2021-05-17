#include "aabb.h"

/**
 * \brief Bounding box constructor
 * \param[in] Min Minimal coordinate
 * \param[in] Max Maximal coordinate
 */
aabb::aabb( const glm::vec3 &Min, const glm::vec3 &Max ) : Min(Min), Max(Max)
{
}

/**
 * \brief Intersection with ray function
 * \param[in] R Ray
 * \param[in, out] T Distance to nearest intersection
 * \return TRUE-if intersect, FALSE-if otherwise
 */
BOOL aabb::Intersect( const ray& R, FLT *T ) const
{
  FLT Tn = (Min.x - R.Org.x) * R.InvDir.x;
  FLT Tf = (Max.x - R.Org.x) * R.InvDir.x;

  FLT Near = fminf(Tn, Tf);
  FLT Far = fmaxf(Tn, Tf);

  Tn = (Min.y - R.Org.y) * R.InvDir.y;
  Tf = (Max.y - R.Org.y) * R.InvDir.y;

  Near = fmaxf(Near, fminf(Tn, Tf));
  Far = fminf(Far, fmaxf(Tn, Tf));

  Tn = (Min.z - R.Org.z) * R.InvDir.z;
  Tf = (Max.z - R.Org.z) * R.InvDir.z;

  Near = fmaxf(Near, fminf(Tn, Tf));
  Far = fminf(Far, fmaxf(Tn, Tf));
  *T = Near;
  Far *= 1.00000024f;

  return Far >= Near;
}
