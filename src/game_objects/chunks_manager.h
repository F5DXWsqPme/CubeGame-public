#ifndef __chunks_manager_h_
#define __chunks_manager_h_

#include <map>
#include <boost/thread/sync_queue.hpp>
#include <atomic>
#include <mutex>
#include <future>
#include "ext/FastNoiseLite/Cpp/FastNoiseLite.h"

#include "chunk.h"
#include "render/render.h"

class player;

/**
 * \brief Chunks manager class
 */
class chunks_manager
{
public:
  /**
   * \brief Chunks manager constructor
   * \param[in, out] Render Render object
   * \param[in] RenderDistance Render distance in chunks
   * \param[in] Player Reference to player
   */
  chunks_manager( render &Render, INT RenderDistance, player &Player );

  /**
   * \brief Set current central chunk in active function
   * \param[in] CurrentChunk Current chunk
   */
  VOID SetCurrentChunk( const std::pair<INT32, INT32> &CurrentChunk );

  /**
   * \brief Chunks manager destructor
   */
  ~chunks_manager( VOID );

  /**
   * \brief Get mutex for active chunks set
   * \return Reference to mutex
   */
  std::mutex & GetActiveChunksMutex( VOID );

  /**
   * \brief Get chunk
   * \param[in] ChunkPos Chunk position
   * \param[in, out] ChunkPtr Reference to pointer to chunk
   * \return Chunk exists flag
   */
  BOOL GetChunk( const std::pair<INT32, INT32> &ChunkPos, chunk *&ChunkPtr );

  /**
   * \brief Update block geometry
   * \param[in] ChunkPos Chunk position
   * \param[in] BlockPos Block position
   */
  VOID UpdateBlock( const std::pair<INT32, INT32> &ChunkPos, const glm::ivec3 &BlockPos );

  /**
   * \brief Get selected block function
   * \param[in] Pos Player position
   * \param[in] Dir Player direction
   * \param[in] PlayerChunkPos Player chunk
   * \param[in] MaxSelectionDistance Maximal selection distance
   * \param[in, out] BlockPos Block position in chunk
   * \param[in, out] Near Distance to nearest intersection
   * \param[in, out] IntersectionChunkPos Intersection chunk position
   * \param[in, out] Normal Intersection normal
   * \return Intersection exists flag
   */
  BOOL GetSelectedBlock( const glm::vec3 &Pos, const glm::vec3 &Dir, const std::pair<INT32, INT32> &PlayerChunkPos,
                         INT MaxSelectionDistance,
                         glm::ivec3 &BlockPos, FLT &Near,
                         std::pair<INT32, INT32> &IntersectionChunkPos, glm::ivec3 &Normal );

private:
  /**
   * \brief Update block side
   * \param[in] x Block x-coordinate
   * \param[in] y Block y-coordinate
   * \param[in] z Block z-coordinate
   * \param[in] ChunkPos Loaded chunk position
   * \param[in, out] ChunkPtr Pointer to chunk
   * \param[in] UpdateFunction Update block side function
   */
  VOID UpdateBlockSide( INT64 x, INT64 y, INT64 z,
                        const std::pair<INT32, INT32> &ChunkPos,
                        chunk *ChunkPtr,
                        VOID (chunk::*UpdateFunction)( const glm::ivec3 &BlockPos ) );

  /**
   * \brief Load chunk function
   * \param[in] ChunkPos Chunk position
   */
  VOID LoadChunk( const std::pair<INT32, INT32> &ChunkPos );

  /**
   * \brief Unload chunk function
   * \param[in] ChunkPos Chunk position
   */
  VOID UnloadChunk( const std::pair<INT32, INT32> &ChunkPos );

  /** Current central chunk in active */
  std::pair<INT32, INT32> CurrentCentralChunk = std::pair<INT32, INT32>(-1, -1);

  /** Active chunks */
  std::map<std::pair<INT32, INT32>, chunk> ActiveChunks;

  /** Render distance */
  INT RenderDistance;

  /** Reference to render */
  render &Render;

  /** Object for noise generation */
  FastNoiseLite Noise;

  /**
   * \brief Load or unload chunk request
   */
  struct CHUNK_REQUEST
  {
    /** Type of operation enumeration */
    enum class OPERATION_TYPE
    {
      LOAD,
      UNLOAD
    };

    /** Type of operation */
    OPERATION_TYPE OperationType;

    /** Chunk position */
    std::pair<INT32, INT32> ChunkPos;
  };

  /** Chunk requests queue */
  boost::concurrent::sync_queue<CHUNK_REQUEST> ChunkRequests;

  /** Exit flag */
  std::atomic<BOOL> ExitFlag;

  /** Mutex for active chunks */
  std::mutex ActiveChunksMutex;

  /** Chunks in drive */
  std::set<std::pair<INT32, INT32>> ChunksInDrive;

  /** Chunks loader thread future */
  std::future<VOID> ChunksLoaderThread;

  /** Chunks loader thread future */
  std::future<VOID> ChunksControllerThread;

  /** Reference to player */
  player &Player;

  /** Controller delay in milliseconds */
  UINT32 ControllerDelay = 200;
};


#endif /* __chunks_manager_h_ */
