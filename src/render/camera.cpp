#include "camera.h"

/**
 * \brief Camera constructor
 * \param[in] W Frame width
 * \param[in] H Frame height
 */
camera::camera( INT W, INT H )
{
  ProjMatrix =
    glm::perspective(glm::pi<float>() * 0.25f, W / (FLT)H, 0.1f, 1000.f);

  ViewProjMatrix = ProjMatrix * ViewMatrix;
}

/**
 * \brief Set width and height
 * \param[in] W Frame width
 * \param[in] H Frame height
 */
VOID camera::SetWH( INT W, INT H )
{
  ProjMatrix =
    glm::perspective(glm::pi<float>() * 0.25f, W / (FLT)H, 0.1f, 1000.f);

  ViewProjMatrix = ProjMatrix * ViewMatrix;
}

/**
 * \brief Set view matrix function
 * \param[in] Loc Camera position
 * \param[in] To Point interest
 * \param[in] Up1 Up direction
 */
VOID camera::SetLocAtUp( const glm::vec3 &Loc, const glm::vec3 &To, const glm::vec3 &Up1 )
{
  ViewMatrix = glm::lookAt(Loc, To, Up1);
  
  ViewProjMatrix = ProjMatrix * ViewMatrix;
}
