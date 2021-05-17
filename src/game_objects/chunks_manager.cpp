#include <iostream>
#include <fstream>
#include <filesystem>
#include "ext/zlib/zlib.h"

#include "chunks_manager.h"
#include "block_type.h"
#include "player.h"

/**
 * \brief Chunks manager constructor
 * \param Render Render object
 * \param RenderDistance Render distance in chunks
 * \param[in] Player Reference to player
 */
chunks_manager::chunks_manager( render &Render, INT RenderDistance, player &Player ) :
  Render(Render), RenderDistance(RenderDistance), Player(Player)
{
  ExitFlag.store(FALSE, std::memory_order_seq_cst);

  {
    std::string FileName = "world/saved.txt";
    std::ifstream File(FileName);

    if (File)
    {
      std::pair<INT32, INT32> Pos;

      while ((File >> Pos.first) && (File >> Pos.second))
        ChunksInDrive.insert(Pos);
    }
  }

  ChunksLoaderThread = std::async([&]( VOID )
  {
    try
    {
      while (!ChunkRequests.empty() || !ExitFlag.load(std::memory_order_seq_cst))
      {
        CHUNK_REQUEST Request;

        ChunkRequests.wait_pull(Request);

        switch (Request.OperationType)
        {
        case CHUNK_REQUEST::OPERATION_TYPE::LOAD:
          LoadChunk(Request.ChunkPos);
          break;
        case CHUNK_REQUEST::OPERATION_TYPE::UNLOAD:
          UnloadChunk(Request.ChunkPos);
          break;
        }
      }
    }
    catch ( const std::runtime_error &Err )
    {
      std::cout << "Error:\n  " << Err.what() << "\n";
      exit(1);
    }
  });

  ChunksControllerThread = std::async([&]( VOID )
  {
    try
    {
      while (!ExitFlag.load(std::memory_order_seq_cst))
      {
        SetCurrentChunk(Player.GetCurrentChunk());
        std::this_thread::sleep_for(std::chrono::milliseconds(ControllerDelay));
      }
    }
    catch ( const std::runtime_error &Err )
    {
      std::cout << "Error:\n  " << Err.what() << "\n";
      exit(1);
    }
  });

  SetCurrentChunk(std::pair<INT32, INT32>(0, 0));
}

/**
 * \brief Get mutex for active chunks set
 * \return Reference to mutex
 */
std::mutex & chunks_manager::GetActiveChunksMutex( VOID )
{
  return ActiveChunksMutex;
}

/**
 * \brief Get chunk
 * \param[in] ChunkPos Chunk position
 * \param[in, out] ChunkPtr Reference to pointer to chunk
 * \return Chunk exists flag
 */
BOOL chunks_manager::GetChunk( const std::pair<INT32, INT32> &ChunkPos, chunk *&ChunkPtr )
{
  std::map<std::pair<INT32, INT32>, chunk>::iterator It = ActiveChunks.find(ChunkPos);

  if (It == ActiveChunks.end())
    return FALSE;

  ChunkPtr = &It->second;

  return TRUE;
}

/**
 * \brief Update block geometry
 * \param ChunkPos Chunk position
 * \param BlockPos Block position
 */
VOID chunks_manager::UpdateBlock( const std::pair<INT32, INT32> &ChunkPos, const glm::ivec3 &BlockPos )
{
  chunk *ChunkPtr = nullptr;
  BOOL GetChunkResult = GetChunk(ChunkPos, ChunkPtr);

  if (GetChunkResult)
    ChunkPtr->UpdateBlock(BlockPos);

  INT64 GlobalX = chunk::ChunkSizeX * (INT64)ChunkPos.first + BlockPos.x;
  INT64 GlobalZ = chunk::ChunkSizeZ * (INT64)ChunkPos.second + BlockPos.z;

  UpdateBlockSide(GlobalX - 1, BlockPos.y, GlobalZ, ChunkPos, ChunkPtr, &chunk::UpdateRightSide);
  UpdateBlockSide(GlobalX + 1, BlockPos.y, GlobalZ, ChunkPos, ChunkPtr, &chunk::UpdateLeftSide);
  UpdateBlockSide(GlobalX, BlockPos.y, GlobalZ - 1, ChunkPos, ChunkPtr, &chunk::UpdateFrontSide);
  UpdateBlockSide(GlobalX, BlockPos.y, GlobalZ + 1, ChunkPos, ChunkPtr, &chunk::UpdateBackSide);

  if (BlockPos.y > 0)
    UpdateBlockSide(GlobalX, BlockPos.y - 1, GlobalZ, ChunkPos, ChunkPtr, &chunk::UpdateUpSide);

  if (BlockPos.y < chunk::ChunkSizeY - 1)
    UpdateBlockSide(GlobalX, BlockPos.y + 1, GlobalZ, ChunkPos, ChunkPtr, &chunk::UpdateDownSide);

  if (GetChunkResult)
    ChunkPtr->UpdateCommandBuffer();

  {
    std::lock_guard<std::mutex> Lock(Render.Synchronization.RenderMutex);

    Render.UpdateCommandBuffers();
  }
}

/**
 * \brief Update block side
 * \param[in] x Block x-coordinate
 * \param[in] y Block y-coordinate
 * \param[in] z Block z-coordinate
 * \param[in] ChunkPos Loaded chunk position
 * \param[in, out] ChunkPtr Pointer to chunk
 * \param[in] UpdateFunction Update block side function
 */
VOID chunks_manager::UpdateBlockSide( INT64 x, INT64 y, INT64 z,
                                      const std::pair<INT32, INT32> &ChunkPos,
                                      chunk *ChunkPtr,
                                      VOID (chunk::*UpdateFunction)( const glm::ivec3 &BlockPos ) )
{
  std::pair<INT32, INT32> NewChunkPos(
    floor(x / (DBL)chunk::ChunkSizeX),
    floor(z / (DBL)chunk::ChunkSizeZ));
  chunk *NewChunkPtr = ChunkPtr;
  glm::ivec3 NewBlockPos(
    x - NewChunkPos.first * (INT64)chunk::ChunkSizeX,
    y,
    z - NewChunkPos.second * (INT64)chunk::ChunkSizeZ);

  if (NewChunkPos != ChunkPos || ChunkPtr == nullptr)
  {
    if (GetChunk(NewChunkPos, NewChunkPtr))
    {
      (NewChunkPtr->*UpdateFunction)(NewBlockPos);
      NewChunkPtr->UpdateCommandBuffer();
    }
  }
  else
  {
    (ChunkPtr->*UpdateFunction)(NewBlockPos);
  }
}

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
BOOL chunks_manager::GetSelectedBlock( const glm::vec3 &Pos, const glm::vec3 &Dir, const std::pair<INT32, INT32> &PlayerChunkPos,
                                       INT MaxSelectionDistance,
                                       glm::ivec3 &BlockPos, FLT &Near,
                                       std::pair<INT32, INT32> &IntersectionChunkPos, glm::ivec3 &Normal )
{
  BOOL IsIntersected = FALSE;
  Near = INFINITY;

  {
    std::lock_guard<std::mutex> Lock(ActiveChunksMutex);

    for (INT32 ChunkX = PlayerChunkPos.first - 1; ChunkX <= PlayerChunkPos.first + 1; ChunkX++)
      for (INT32 ChunkZ = PlayerChunkPos.second - 1; ChunkZ <= PlayerChunkPos.second + 1; ChunkZ++)
      {
        std::pair<INT32, INT32> ChunkPos(ChunkX, ChunkZ);
        chunk *ChunkPtr = nullptr;
        FLT T = INFINITY;

        if (GetChunk(ChunkPos, ChunkPtr))
        {
          glm::ivec3 NewNormal;
          glm::ivec3 Res = ChunkPtr->GetSelectedBlock(Pos, Dir, MaxSelectionDistance, &T, ChunkPos, NewNormal);

          if (T < MaxSelectionDistance && T < Near)
          {
            IsIntersected = TRUE;
            Near = T;
            BlockPos = Res;
            IntersectionChunkPos = ChunkPos;
            Normal = NewNormal;
          }
        }
      }
  }

  return IsIntersected;
}

/**
 * \brief Load chunk function
 * \param[in] ChunkPos Chunk position
 */
VOID chunks_manager::LoadChunk( const std::pair<INT32, INT32> &ChunkPos )
{
  chunk *ChunkPtr = nullptr;

  {
    std::lock_guard<std::mutex> Lock(ActiveChunksMutex);

    ChunkPtr = &ActiveChunks[ChunkPos];

    if (ChunksInDrive.find(ChunkPos) == ChunksInDrive.end())
    {
      INT32 ChunkX = ChunkPos.first;
      INT32 ChunkZ = ChunkPos.second;

      for (UINT64 z = 0; z < chunk::ChunkSizeZ; z++)
      {
        DBL GlobalZ = ChunkZ * chunk::ChunkSizeZ + (DBL)z;

        for (UINT64 x = 0; x < chunk::ChunkSizeZ; x++)
        {
          DBL GlobalX = ChunkX * chunk::ChunkSizeX + (DBL)x;

          DBL GlobalY = 15 * Noise.GetNoise(GlobalX / 2, GlobalZ / 2, 0.0) + 65;

          UINT64 MaxY = GlobalY;

          for (UINT64 y = 0; y < MaxY; y++)
            ChunkPtr->Blocks[z * chunk::ChunkSizeY * chunk::ChunkSizeX + y * chunk::ChunkSizeX +
                             x].BlockTypeId = BLOCK_TYPE::StoneId;

          BOOL IsGrassCovered = Noise.GetNoise(GlobalX * 10, GlobalZ * 10, 50.0) > -0.5;

          if (IsGrassCovered)
            ChunkPtr->Blocks[z * chunk::ChunkSizeY * chunk::ChunkSizeX + MaxY * chunk::ChunkSizeX +
                             x].BlockTypeId = BLOCK_TYPE::GrassId;
          else
            ChunkPtr->Blocks[z * chunk::ChunkSizeY * chunk::ChunkSizeX + MaxY * chunk::ChunkSizeX +
                             x].BlockTypeId = BLOCK_TYPE::StoneId;
        }
      }
    }
    else
    {
      if (!std::filesystem::exists("world"))
        throw std::runtime_error("save directory doesn't exists");

      std::string FileName = "world/" + std::to_string(ChunkPos.first) + "," + std::to_string(ChunkPos.second) + ".chunk";

      std::ifstream File(FileName, std::ios::binary | std::ios::ate);

      if (!File)
        throw std::runtime_error("file not opened for read");

      INT64 Size = File.tellg();
      std::vector<CHAR> Bytes(Size);

      File.seekg(std::ios::beg);
      File.read(Bytes.data(), Size);

      UINT32 DstDataSize = ChunkPtr->Blocks.size() * sizeof(BLOCK);
      UINT32 DecompressedSize = DstDataSize;

      INT DecompressionRes = uncompress(
        reinterpret_cast<Bytef *>(ChunkPtr->Blocks.data()),
        reinterpret_cast<uLongf *>(&DecompressedSize),
        reinterpret_cast<const Bytef *>(Bytes.data()),
        Bytes.size() * sizeof(BYTE));

      if (DecompressionRes != Z_OK || DecompressedSize != DstDataSize)
        throw std::runtime_error("decompression error");
    }
  }

  ChunkPtr->CreateGeometry(Render, ChunkPos);
}

/**
 * \brief Unload chunk function
 * \param[in] ChunkPos Chunk position
 */
VOID chunks_manager::UnloadChunk( const std::pair<INT32, INT32> &ChunkPos )
{
  std::lock_guard<std::mutex> Lock(ActiveChunksMutex);
  std::map<std::pair<INT32, INT32>, chunk>::iterator It = ActiveChunks.find(ChunkPos);

  if (It == ActiveChunks.end())
  {
    //throw std::runtime_error("chunk doesn't exists");
    std::cout << "chunk doesn't exists (mb it's not good)\n" << std::endl;
    return;
  }
  
  if (!std::filesystem::exists("world"))
    std::filesystem::create_directory("world");

  std::string FileName = "world/" + std::to_string(ChunkPos.first) + "," + std::to_string(ChunkPos.second) + ".chunk";

  std::ofstream File(FileName, std::ios::binary);

  if (!File)
    throw std::runtime_error("file not opened for write");

  UINT32 SrcDataSize = It->second.Blocks.size() * sizeof(BLOCK);

  UINT32 MaxDstDataSize = compressBound(SrcDataSize);
  UINT32 DstDataSize = MaxDstDataSize;

  std::vector<BYTE> CompressedData(MaxDstDataSize);

  INT CompressionRes = compress(reinterpret_cast<Bytef *>(CompressedData.data()),
    reinterpret_cast<uLongf *>(&DstDataSize),
    reinterpret_cast<const Bytef *>(It->second.Blocks.data()),
    SrcDataSize);

  if (CompressionRes != Z_OK)
    throw std::runtime_error("compression error");

  File.write(reinterpret_cast<const CHAR *>(CompressedData.data()), DstDataSize);

  ChunksInDrive.insert(ChunkPos);

  ActiveChunks.erase(ChunkPos);
}

/**
 * \brief Set current central chunk in active function
 * \param[in] CurrentChunk Current chunk
 */
VOID chunks_manager::SetCurrentChunk( const std::pair<INT32, INT32> &CurrentChunk )
{
  if (CurrentChunk == CurrentCentralChunk)
    return;

  CurrentCentralChunk = CurrentChunk;

  INT32 FirstXChunk = CurrentChunk.first - RenderDistance;
  INT32 LastXChunk = CurrentChunk.first + RenderDistance;
  INT32 FirstYChunk = CurrentChunk.second - RenderDistance;
  INT32 LastYChunk = CurrentChunk.second + RenderDistance;

  {
    std::lock_guard<std::mutex> Lock(ActiveChunksMutex);

    for (const auto &[Pos, Chunk] : ActiveChunks)
      if (Pos.first < FirstXChunk || Pos.first > LastXChunk ||
          Pos.second < FirstYChunk || Pos.second > LastYChunk)
      {
        CHUNK_REQUEST Request = {CHUNK_REQUEST::OPERATION_TYPE::UNLOAD, Pos};

        ChunkRequests.wait_push(Request);
      }
  }

  for (INT32 ChunkZ = FirstYChunk; ChunkZ <= LastYChunk; ChunkZ++)
    for (INT32 ChunkX = FirstXChunk; ChunkX <= LastXChunk; ChunkX++)
    {
      std::pair<INT32, INT32> ChunkPos(ChunkX, ChunkZ);

      std::map<std::pair<INT32, INT32>, chunk>::iterator It;

      {
        std::lock_guard<std::mutex> Lock(ActiveChunksMutex);

        It = ActiveChunks.find(ChunkPos);

        if (It == ActiveChunks.end())
          ActiveChunks[ChunkPos];
      }

      if (It == ActiveChunks.end())
      {
        CHUNK_REQUEST Request = {CHUNK_REQUEST::OPERATION_TYPE::LOAD, ChunkPos};

        ChunkRequests.wait_push(Request);
      }
    }
}

/**
 * \brief Chunks manager destructor
 */
chunks_manager::~chunks_manager( VOID )
{
  for (const auto &[Pos, Chunk] : ActiveChunks)
  {
    CHUNK_REQUEST Request = {CHUNK_REQUEST::OPERATION_TYPE::UNLOAD, Pos};

    ChunkRequests.wait_push(Request);
  }

  ExitFlag.store(TRUE, std::memory_order_seq_cst);

  ChunksControllerThread.wait();
  ChunksLoaderThread.wait();

  if (!std::filesystem::exists("world"))
    std::filesystem::create_directory("world");

  if (std::filesystem::exists("world/saved.txt"))
    std::filesystem::remove("world/saved.txt");

  std::string FileName = "world/saved.txt";
  std::ofstream File(FileName);

  if (!File)
  {
    std::cout << "Error: file with chunks in drive not opened for write\n" << std::endl;
    return; //throw std::runtime_error("file not opened");
  }

  for (const std::pair<INT32, INT32> &Pos : ChunksInDrive)
    File << Pos.first << " " << Pos.second << "\n";
}

