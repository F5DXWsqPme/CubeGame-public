#ifndef __chunk_h_
#define __chunk_h_

#include <memory>
#include <array>

#include "def.h"
#include "block.h"
#include "render/chunk_geometry.h"
#include "render/render.h"

/**
 * \brief Blocks chunk class
 */
class chunk
{
public:
  /** Chunk size for X-coordinate */
  static constexpr INT ChunkSizeX = 16;

  /** Chunk size for Y-coordinate */
  static constexpr INT ChunkSizeY = 256;

  /** Chunk size for Z-coordinate */
  static constexpr INT ChunkSizeZ = 16;

  /**
   * \brief Create geometry for chunk
   * \param[in, out] Render Reference to render
   * \param[in] ChunkPos Chunk position
   */
  VOID CreateGeometry( render &Render, const std::pair<INT32, INT32> &ChunkPos );

  /**
   * \brief Destroy geometry for chunk
   */
  VOID DestroyGeometry( VOID );

  /**
   * \brief Get selected block function
   * \param[in] Pos Player position
   * \param[in] Dir Player direction
   * \param[in] MaxDistance Maximal distance to box
   * \param[in, out] T Intersection distance
   * \param[in] ChunkPos Chunk position
   * \param[in, out] Normal Intersection normal
   * \return Block coordinates
   */
  glm::ivec3 GetSelectedBlock( const glm::vec3 &Pos, const glm::vec3 &Dir, INT MaxDistance, FLT *T,
                               const std::pair<INT32, INT32> &ChunkPos, glm::ivec3 &Normal ) const;

  /**
   * \brief Update block function
   * \param[in] BlockPos Block position
   */
  VOID UpdateBlock( const glm::ivec3 &BlockPos );

  /**
   * \brief Update up block border
   * \param[in] BlockPos Block position
   */
  VOID UpdateUpSide( const glm::ivec3 &BlockPos );

  /**
   * \brief Update left block border
   * \param[in] BlockPos Block position
   */
  VOID UpdateLeftSide( const glm::ivec3 &BlockPos );

  /**
   * \brief Update down block border
   * \param[in] BlockPos Block position
   */
  VOID UpdateDownSide( const glm::ivec3 &BlockPos );

  /**
   * \brief Update right block border
   * \param[in] BlockPos Block position
   */
  VOID UpdateRightSide( const glm::ivec3 &BlockPos );

  /**
   * \brief Update front block border
   * \param[in] BlockPos Block position
   */
  VOID UpdateFrontSide( const glm::ivec3 &BlockPos );

  /**
   * \brief Update back block border
   * \param[in] BlockPos Block position
   */
  VOID UpdateBackSide( const glm::ivec3 &BlockPos );

  /**
   * \brief Update command buffer function
   */
  VOID UpdateCommandBuffer( VOID ) const;

  /** Chunk blocks */
  std::array<BLOCK, ChunkSizeZ * ChunkSizeY * ChunkSizeX> Blocks;

private:
  /** Object for drawing */
  std::unique_ptr<chunk_geometry> Geometry;

  /** Threshold for normal evaluation */
  static constexpr FLT NormalThreshold = 1e-5f;
};

#endif /* __chunk_h_ */
