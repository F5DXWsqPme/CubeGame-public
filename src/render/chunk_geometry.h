#ifndef __chunk_geometry_h_
#define __chunk_geometry_h_

#include "render.h"
#include "draw_element.h"
#include "game_objects/block.h"

/**
 * \brief Chunk display class
 */
class chunk_geometry final : public draw_element
{
public:
  /** Chunk size for X-coordinate */
  static constexpr UINT ChunkSizeX = 16;

  /** Chunk size for Y-coordinate */
  static constexpr UINT ChunkSizeY = 256;

  /** Chunk size for Z-coordinate */
  static constexpr UINT ChunkSizeZ = 16;

  /** Maximal number of borders */
  const static UINT64 MaxNumberOfBorders;

  /** Maximal number of indices */
  const static UINT64 MaxNumberOfIndices;

  /** Maximal number of vertices */
  const static UINT64 MaxNumberOfVertices;

  /** Size of vertex buffer for chunk */
  const static UINT64 VertexBufferSize;

  /** Size of index buffer for chunk */
  const static UINT64 IndexBufferSize;

  /**
   * \brief Chunk display class constructor
   * \param[in, out] Render Reference to render
   * \param[in] Blocks Blocks array
   * \param[in] ChunkPos Chunk position
   */
  chunk_geometry( render &Render, const BLOCK *Blocks, const std::pair<INT32, INT32> &ChunkPos );

  /**
   * \brief Get command buffer for draw function
   * \return Secondary command buffer
   */
  VkCommandBuffer GetCommandBuffer( VOID ) override;

  /**
   * \brief Get secondary command buffer for drawing opaque objects
   * \return Command buffer
   */
  VkCommandBuffer GetTransparentCommandBuffer( VOID ) override;

  ///**
  // * \brief Update WVP function
  // */
  //VOID UpdateWVP( VOID ) override;

  /**
   * \brief Chunk geometry destructor
   */
  ~chunk_geometry( VOID );

  /**
   * \brief Update block function
   * \param[in] BlockPos Block position
   * \param[in] Blocks Blocks array
   */
  VOID UpdateBlock( const glm::ivec3 &BlockPos, const BLOCK *Blocks );

  /**
   * \brief Update up block border
   * \param[in] BlockPos Block position
   * \param[in] Blocks Blocks array
   */
  VOID UpdateUpSide( const glm::ivec3 &BlockPos, const BLOCK *Blocks );

  /**
   * \brief Update left block border
   * \param[in] BlockPos Block position
   * \param[in] Blocks Blocks array
   */
  VOID UpdateLeftSide( const glm::ivec3 &BlockPos, const BLOCK *Blocks );

  /**
   * \brief Update down block border
   * \param[in] BlockPos Block position
   * \param[in] Blocks Blocks array
   */
  VOID UpdateDownSide( const glm::ivec3 &BlockPos, const BLOCK *Blocks );

  /**
   * \brief Update right block border
   * \param[in] BlockPos Block position
   * \param[in] Blocks Blocks array
   */
  VOID UpdateRightSide( const glm::ivec3 &BlockPos, const BLOCK *Blocks );

  /**
   * \brief Update front block border
   * \param[in] BlockPos Block position
   * \param[in] Blocks Blocks array
   */
  VOID UpdateFrontSide( const glm::ivec3 &BlockPos, const BLOCK *Blocks );

  /**
   * \brief Update back block border
   * \param[in] BlockPos Block position
   * \param[in] Blocks Blocks array
   */
  VOID UpdateBackSide( const glm::ivec3 &BlockPos, const BLOCK *Blocks );

  /**
   * \brief Update command buffer function
   */
  VOID UpdateCommandBuffer( VOID ) const;

private:
  /**
   * \brief Remove up border function
   * \param[in] BlockPos Block position
   */
  VOID RemoveUpBorder( const glm::ivec3 &BlockPos );

  /**
   * \brief Remove down border function
   * \param[in] BlockPos Block position
   */
  VOID RemoveDownBorder( const glm::ivec3 &BlockPos );

  /**
   * \brief Remove right border function
   * \param[in] BlockPos Block position
   */
  VOID RemoveRightBorder( const glm::ivec3 &BlockPos );

  /**
   * \brief Remove left border function
   * \param[in] BlockPos Block position
   */
  VOID RemoveLeftBorder( const glm::ivec3 &BlockPos );

  /**
   * \brief Remove front border function
   * \param[in] BlockPos Block position
   */
  VOID RemoveFrontBorder( const glm::ivec3 &BlockPos );

  /**
   * \brief Remove back border function
   * \param[in] BlockPos Block position
   * \param[in] TexCoords Texture coordinates
   * \param[in] Alpha Alpha part
   */
  VOID RemoveBackBorder( const glm::ivec3 &BlockPos );

  /**
   * \brief Add up border function
   * \param[in] BlockPos Block position
   * \param[in] TexCoords Texture coordinates
   * \param[in] Alpha Alpha part
   */
  VOID AddUpBorder( const glm::ivec3 &BlockPos, const glm::vec2 *TexCoords, FLT Alpha );

  /**
   * \brief Add down border function
   * \param[in] BlockPos Block position
   * \param[in] TexCoords Texture coordinates
   * \param[in] Alpha Alpha part
   */
  VOID AddDownBorder( const glm::ivec3 &BlockPos, const glm::vec2 *TexCoords, FLT Alpha );

  /**
   * \brief Add right border function
   * \param[in] BlockPos Block position
   * \param[in] TexCoords Texture coordinates
   * \param[in] Alpha Alpha part
   */
  VOID AddRightBorder( const glm::ivec3 &BlockPos, const glm::vec2 *TexCoords, FLT Alpha );

  /**
   * \brief Add left border function
   * \param[in] BlockPos Block position
   * \param[in] TexCoords Texture coordinates
   * \param[in] Alpha Alpha part
   */
  VOID AddLeftBorder( const glm::ivec3 &BlockPos, const glm::vec2 *TexCoords, FLT Alpha );

  /**
   * \brief Add front border function
   * \param[in] BlockPos Block position
   * \param[in] TexCoords Texture coordinates
   * \param[in] Alpha Alpha part
   */
  VOID AddFrontBorder( const glm::ivec3 &BlockPos, const glm::vec2 *TexCoords, FLT Alpha );

  /**
   * \brief Add back border function
   * \param[in] BlockPos Block position
   * \param[in] TexCoords Texture coordinates
   * \param[in] Alpha Alpha part
   */
  VOID AddBackBorder( const glm::ivec3 &BlockPos, const glm::vec2 *TexCoords, FLT Alpha );

  /**
   * \brief Fill command buffer function
   */
  VOID CreateCommandBuffer( VOID ) const;

  /** Memory chunk index */
  UINT32 MemoryChunkId;

  /** Offset in vertex buffer */
  UINT64 VertexBufferOffset;

  /** Offset in index buffer */
  UINT64 IndexBufferOffset;

  /** Reference to vertex buffer memory */
  const memory &VertexMemory;

  /** Reference to index buffer memory */
  const memory &IndexMemory;

  /** Reference to vertex buffer */
  const buffer &VertexBuffer;

  /** Reference to index buffer */
  const buffer &IndexBuffer;

  /** Command buffer */
  VkCommandBuffer CommandBufferId;

  /** Command buffer for opaque objects */
  VkCommandBuffer TransparentCommandBufferId;

  /** Reference to render */
  render &Render;

  /** Number of vertices */
  UINT32 NumberOfVertices;

  /** Number of indices */
  UINT64 NumberOfIndices;

  /** Number of borders */
  UINT64 NumberOfBorders;

  /** Number of vertices */
  UINT32 NumberOfTransparentVertices;

  /** Number of indices */
  UINT64 NumberOfTransparentIndices;

  /** Number of borders */
  UINT64 NumberOfTransparentBorders;

  /** mutex for blocks and indices information */
  std::mutex MetaInfoMutex;

  /**
   * \brief Block geometry information
   */
  struct BLOCK_INFORMATION
  {
    /** +Y block border index */
    INT64 UpOffset = -1;

    /** -Y block border index */
    INT64 DownOffset = -1;

    /** +X block border index */
    INT64 RightOffset = -1;

    /** -X block border index */
    INT64 LeftOffset = -1;

    /** +Z block border index */
    INT64 FrontOffset = -1;

    /** -Z block border index */
    INT64 BackOffset = -1;
  };

  /**
   * \brief Border index information
   */
  struct INDEX_INFORMATION
  {
    /** Block identifier */
    INT64 BlockId = -1;

    /** Pointer to border index in block information */
    INT64 BLOCK_INFORMATION::*Offset = nullptr;
  };

  /** Blocks information */
  BLOCK_INFORMATION BlocksInfo[ChunkSizeZ * ChunkSizeY * ChunkSizeX];

  /** Indices to blocks table */
  std::vector<INDEX_INFORMATION> IndicesInfo;

  /** Indices to blocks table */
  std::vector<INDEX_INFORMATION> TransparentIndicesInfo;

  /** Chunk offset x */
  DBL ChunkOffsetX;

  /** Chunk offset z */
  DBL ChunkOffsetZ;

  /** Chunk blocks */
  const BLOCK *ChunkBlocks = nullptr;
};

#endif /* __chunk_geometry_h_ */
