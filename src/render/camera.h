#ifndef __camera_h_
#define __camera_h_

#include "def.h"

/**
 * \brief Camera class
 */
class camera
{
public:
  /**
   * \brief Camera constructor
   * \param[in] W Frame width
   * \param[in] H Frame height
   */
  camera( INT W = 1920, INT H = 1080 );

  /**
   * \brief Set width and height
   * \param[in] W Frame width
   * \param[in] H Frame height
   */
  VOID SetWH( INT W, INT H );

  /**
   * \brief Set view matrix function
   * \param[in] Loc Camera position
   * \param[in] To Point interest
   * \param[in] Up1 Up direction
   */
  VOID SetLocAtUp( const glm::vec3 &Loc, const glm::vec3 &To, const glm::vec3 &Up1 );

  /** Projection matrix */
  glm::mat4 ProjMatrix =
    glm::perspective(glm::pi<float>() * 0.25f, 1920.f / 1080, 0.1f, 1000.f);

  /** View matrix */
  glm::mat4 ViewMatrix =
    glm::lookAt(glm::vec3(0, 80, 0), glm::vec3(20, 70, 20), glm::vec3(0, -1, 0));

  /** View matrix and projection matrix multiplication */
  glm::mat4 ViewProjMatrix =
    ProjMatrix * ViewMatrix;

private:
};

#endif /* __camera_h_ */
