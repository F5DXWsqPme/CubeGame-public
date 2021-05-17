#ifndef __vertex_h_
#define __vertex_h_

/**
 * \brief Vertex description structure
 */
struct VERTEX
{
  /** Vertex position */
  glm::vec3 Position;

  /** Vertex alpha channel */
  FLT Alpha;

  /** Texture coordinates */
  glm::vec2 TexCoord;
};

#endif /* __vertex_h_ */
