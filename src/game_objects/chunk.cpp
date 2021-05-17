#include "chunk.h"
#include "utils/aabb.h"
#include "block_type.h"

/**
 * \brief Create geometry for chunk
 * \param[in, out] Render Reference to render
 * \param[in] ChunkPos Chunk position
 */
VOID chunk::CreateGeometry( render &Render, const std::pair<INT32, INT32> &ChunkPos )
{
  Geometry = std::make_unique<chunk_geometry>(Render, Blocks.data(), ChunkPos);
}

/**
 * \brief Destroy geometry for chunk
 */
VOID chunk::DestroyGeometry( VOID )
{
  Geometry = nullptr;
}

/**
 * \brief Update block function
 * \param BlockPos Block position
 */
VOID chunk::UpdateBlock( const glm::ivec3 &BlockPos )
{
  Geometry->UpdateBlock(BlockPos, Blocks.data());
}

/**
 * \brief Update command buffer function
 */
VOID chunk::UpdateCommandBuffer( VOID ) const
{
  Geometry->UpdateCommandBuffer();
}

/**
 * \brief Update up block border
 * \param[in] BlockPos Block position
 */
VOID chunk::UpdateUpSide( const glm::ivec3 &BlockPos )
{
  Geometry->UpdateUpSide(BlockPos, Blocks.data());
}

/**
 * \brief Update left block border
 * \param[in] BlockPos Block position
 */
VOID chunk::UpdateLeftSide( const glm::ivec3 &BlockPos )
{
  Geometry->UpdateLeftSide(BlockPos, Blocks.data());
}

/**
 * \brief Update down block border
 * \param[in] BlockPos Block position
 */
VOID chunk::UpdateDownSide( const glm::ivec3 &BlockPos )
{
  Geometry->UpdateDownSide(BlockPos, Blocks.data());
}

/**
 * \brief Update right block border
 * \param[in] BlockPos Block position
 */
VOID chunk::UpdateRightSide( const glm::ivec3 &BlockPos )
{
  Geometry->UpdateRightSide(BlockPos, Blocks.data());
}

/**
 * \brief Update front block border
 * \param[in] BlockPos Block position
 */
VOID chunk::UpdateFrontSide( const glm::ivec3 &BlockPos )
{
  Geometry->UpdateFrontSide(BlockPos, Blocks.data());
}

/**
 * \brief Update back block border
 * \param[in] BlockPos Block position
 */
VOID chunk::UpdateBackSide( const glm::ivec3 &BlockPos )
{
  Geometry->UpdateBackSide(BlockPos, Blocks.data());
}

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
glm::ivec3 chunk::GetSelectedBlock( const glm::vec3 &Pos, const glm::vec3 &Dir, INT MaxDistance, FLT *T,
                                    const std::pair<INT32, INT32> &ChunkPos, glm::ivec3 &Normal ) const
{
  *T = std::numeric_limits<FLT>::max();
  glm::ivec3 Result(0, 0, 0);

  glm::vec3 RelativePos = glm::vec3(Pos.x - ChunkPos.first * ChunkSizeX, Pos.y,
                                    Pos.z - ChunkPos.second * ChunkSizeX);

  ray Ray(RelativePos, Dir);

  INT64 CenterX = floor(RelativePos.x);
  INT64 CenterY = floor(RelativePos.y);
  INT64 CenterZ = floor(RelativePos.z);

  INT64 MinX = std::max(CenterX - MaxDistance, (INT64)0);
  INT64 MinY = std::max(CenterY - MaxDistance, (INT64)0);
  INT64 MinZ = std::max(CenterZ - MaxDistance, (INT64)0);

  INT64 MaxX = std::min(CenterX + MaxDistance, (INT64)ChunkSizeX - 1);
  INT64 MaxY = std::min(CenterY + MaxDistance, (INT64)ChunkSizeY - 1);
  INT64 MaxZ = std::min(CenterZ + MaxDistance, (INT64)ChunkSizeZ - 1);

  if (Dir.x > 0)
    MinX = std::max(CenterX, (INT64)0);
  else
    MaxX = std::min(CenterX, (INT64)ChunkSizeX - 1);

  if (Dir.y > 0)
    MinY = std::max(CenterY, (INT64)0);
  else
    MaxY = std::min(CenterY, (INT64)ChunkSizeY - 1);

  if (Dir.z > 0)
    MinZ = std::max(CenterZ, (INT64)0);
  else
    MaxZ = std::min(CenterZ, (INT64)ChunkSizeZ - 1);

  for (INT64 z = MinZ; z <= MaxZ; z++)
    for (INT64 y = MinY; y <= MaxY; y++)
      for (INT64 x = MinX; x <= MaxX; x++)
      {
        if (Blocks[z * (UINT64)ChunkSizeY * ChunkSizeX +
            y * (UINT64)ChunkSizeX + x].BlockTypeId != BLOCK_TYPE::AirId)
        {
          aabb Box(glm::vec3(x, y, z), glm::vec3(x + 1, y + 1, z + 1));
          FLT NewT = std::numeric_limits<FLT>::max();

          if (Box.Intersect(Ray, &NewT))
          {
            if (NewT > 0 && NewT < *T)
            {
              *T = NewT;
              Result = glm::ivec3(x, y, z);

              glm::vec3 IntrPos = Ray(NewT);

              if (fabsf(IntrPos.x - x) < NormalThreshold)
                Normal = glm::ivec3(-1, 0, 0);
              else if (fabsf(IntrPos.y - y) < NormalThreshold)
                Normal = glm::ivec3(0, -1, 0);
              else if (fabsf(IntrPos.z - z) < NormalThreshold)
                Normal = glm::ivec3(0, 0, -1);
              else if (fabsf(IntrPos.x - x - 1) < NormalThreshold)
                Normal = glm::ivec3(1, 0, 0);
              else if (fabsf(IntrPos.y - y - 1) < NormalThreshold)
                Normal = glm::ivec3(0, 1, 0);
              else
                Normal = glm::ivec3(0, 0, 1);
            }
          }
        }
      }

  return Result;
}
