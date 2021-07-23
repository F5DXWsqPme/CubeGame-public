#include <cstring>
#include <iostream>
#include <thread>

#include "chunk_geometry.h"
#include "game_objects/chunk.h"
#include "vertex.h"
#include "game_objects/chunk.h"
#include "game_objects/block_type.h"
#include "vulkan_wrappers/command_buffer.h"

/** Maximal number of indices */
const UINT64 chunk_geometry::MaxNumberOfBorders =
  chunk::ChunkSizeX * chunk::ChunkSizeY * chunk::ChunkSizeZ * 3 / 32; // TODO: check overflow

/** Maximal number of indices */
const UINT64 chunk_geometry::MaxNumberOfIndices =
  chunk_geometry::MaxNumberOfBorders * 6; // TODO: check overflow

/** Maximal number of vertices */
const UINT64 chunk_geometry::MaxNumberOfVertices =
  chunk_geometry::MaxNumberOfBorders * 4; // TODO: check overflow

/** Size of vertex buffer for chunk */
const UINT64 chunk_geometry::VertexBufferSize =
  sizeof(VERTEX) * chunk_geometry::MaxNumberOfVertices; // TODO: check overflow

/** Size of index buffer for chunk */
const UINT64 chunk_geometry::IndexBufferSize =
  sizeof(UINT32) * chunk_geometry::MaxNumberOfIndices; // TODO: check overflow

/**
 * \brief Chunk display class constructor
 * \param[in, out] Render Reference to render
 * \param[in] Blocks Blocks array
 * \param[in] ChunkPos Chunk position
 */
chunk_geometry::chunk_geometry( render &Render, const BLOCK *Blocks, const std::pair<INT32, INT32> &ChunkPos ) :
  Render(Render),
  VertexMemory(Render.MemoryManager.VertexMemory),
  IndexMemory(Render.MemoryManager.IndexMemory),
  VertexBuffer(Render.MemoryManager.VertexBuffer),
  IndexBuffer(Render.MemoryManager.IndexBuffer)
{
  static_assert(chunk::ChunkSizeX == ChunkSizeX);
  static_assert(chunk::ChunkSizeY == ChunkSizeY);
  static_assert(chunk::ChunkSizeZ == ChunkSizeZ);

  ChunkOffsetX = ChunkSizeX * (DBL)ChunkPos.first;
  ChunkOffsetZ = ChunkSizeZ * (DBL)ChunkPos.second;

  MemoryChunkId = Render.MemoryManager.AllocateChunk();

  VertexBufferOffset = MemoryChunkId * VertexBufferSize;
  IndexBufferOffset = MemoryChunkId * IndexBufferSize;

  BYTE *WriteVertexMemory = Render.MemoryManager.GetMemoryForWriting(VertexBufferSize,
    VertexBufferOffset, VertexMemory, Render.MemoryManager.NeedCopyVertex);

  VERTEX *WriteVertices = reinterpret_cast<VERTEX *>(WriteVertexMemory);

  //BYTE *WriteIndexMemory = Render.MemoryManager.GetMemoryForWriting(IndexBufferSize,
  //  IndexBufferOffset, IndexMemory, Render.MemoryManager.NeedCopyIndex);

  //UINT32 *WriteIndices = reinterpret_cast<UINT32 *>(WriteIndexMemory);

  std::vector<UINT32> WriteIndices(IndexBufferSize / sizeof(UINT32));

  UINT64 CurBorder = 0;
  UINT64 CurTransparentBorder = 0;

  {
    std::lock_guard<std::mutex> Lock(MetaInfoMutex);

    for (UINT z = 0; z < ChunkSizeZ; z++)
      for (UINT y = 0; y < ChunkSizeY; y++)
        for (UINT x = 0; x < ChunkSizeX; x++)
        {
          UINT64 BlockId = z * (UINT64)ChunkSizeY * ChunkSizeX + y * (UINT64)ChunkSizeX + x;
          const BLOCK &CurBlock = Blocks[BlockId];
          const BLOCK_TYPE &CurType = BLOCK_TYPE::Table[CurBlock.BlockTypeId];
          BLOCK_INFORMATION &CurBlockInfo = BlocksInfo[BlockId];

          if (CurType.Alpha < FLT_EPSILON)
            continue;

          if (x == 0 || BLOCK_TYPE::Table[Blocks[z * (UINT64)ChunkSizeY * ChunkSizeX + y * (UINT64)ChunkSizeX + x -
                                                 1].BlockTypeId].Alpha < 1 - FLT_EPSILON)
          {
            UINT64 CurVertexOffset = 4 * CurBorder;
            UINT64 CurIndexOffset = 6 * CurBorder;
            const glm::vec2 *TexCoords = nullptr;

            CurBlockInfo.LeftOffset = CurBorder;

            if (CurType.Alpha < 1 - FLT_EPSILON)
            {
              CurTransparentBorder++;
              CurVertexOffset = MaxNumberOfVertices - CurTransparentBorder * 4;
              CurIndexOffset = MaxNumberOfIndices - CurTransparentBorder * 6;
              CurBlockInfo.LeftOffset = MaxNumberOfBorders - CurTransparentBorder;
            }

            INDEX_INFORMATION CurIndex;

            CurIndex.BlockId = BlockId;
            CurIndex.Offset = &BLOCK_INFORMATION::LeftOffset;

            if (CurType.Alpha < 1 - FLT_EPSILON)
              TransparentIndicesInfo.push_back(CurIndex);
            else
              IndicesInfo.push_back(CurIndex);

            if (CurBlock.Direction.x != 0)
            {
              if (CurBlock.Direction.x == -1)
                TexCoords = CurType.TexCoordFront;
              else
                TexCoords = CurType.TexCoordBack;
            } else if (CurBlock.Right.x != 0)
            {
              if (CurBlock.Right.x == -1)
                TexCoords = CurType.TexCoordRight;
              else
                TexCoords = CurType.TexCoordLeft;
            } else if (CurBlock.Up.x != 0)
            {
              if (CurBlock.Up.x == -1)
                TexCoords = CurType.TexCoordUp;
              else
                TexCoords = CurType.TexCoordDown;
            }

            WriteVertices[CurVertexOffset + 0].Position = glm::vec3(ChunkOffsetX + x, y, ChunkOffsetZ + z);
            WriteVertices[CurVertexOffset + 1].Position = glm::vec3(ChunkOffsetX + x, y + 1, ChunkOffsetZ + z);
            WriteVertices[CurVertexOffset + 2].Position = glm::vec3(ChunkOffsetX + x, y + 1, ChunkOffsetZ + z + 1);
            WriteVertices[CurVertexOffset + 3].Position = glm::vec3(ChunkOffsetX + x, y, ChunkOffsetZ + z + 1);

            WriteVertices[CurVertexOffset + 0].TexCoord = TexCoords[0];
            WriteVertices[CurVertexOffset + 1].TexCoord = TexCoords[1];
            WriteVertices[CurVertexOffset + 2].TexCoord = TexCoords[2];
            WriteVertices[CurVertexOffset + 3].TexCoord = TexCoords[3];

            WriteVertices[CurVertexOffset + 0].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 1].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 2].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 3].Alpha = CurType.Alpha;

            WriteIndices[CurIndexOffset + 0] = CurVertexOffset + 0;
            WriteIndices[CurIndexOffset + 1] = CurVertexOffset + 1;
            WriteIndices[CurIndexOffset + 2] = CurVertexOffset + 2;
            WriteIndices[CurIndexOffset + 3] = CurVertexOffset + 0;
            WriteIndices[CurIndexOffset + 4] = CurVertexOffset + 2;
            WriteIndices[CurIndexOffset + 5] = CurVertexOffset + 3;

            if (CurType.Alpha >= 1 - FLT_EPSILON)
              CurBorder++;
          }

          if (x == ChunkSizeX - 1 ||
              BLOCK_TYPE::Table[Blocks[z * (UINT64)ChunkSizeY * ChunkSizeX + y * (UINT64)ChunkSizeX + x +
                                       1].BlockTypeId].Alpha < 1 - FLT_EPSILON)
          {
            UINT64 CurVertexOffset = 4 * CurBorder;
            UINT64 CurIndexOffset = 6 * CurBorder;
            const glm::vec2 *TexCoords = nullptr;

            CurBlockInfo.RightOffset = CurBorder;

            if (CurType.Alpha < 1 - FLT_EPSILON)
            {
              CurTransparentBorder++;
              CurVertexOffset = MaxNumberOfVertices - CurTransparentBorder * 4;
              CurIndexOffset = MaxNumberOfIndices - CurTransparentBorder * 6;
              CurBlockInfo.RightOffset = MaxNumberOfBorders - CurTransparentBorder;
            }

            INDEX_INFORMATION CurIndex;

            CurIndex.BlockId = BlockId;
            CurIndex.Offset = &BLOCK_INFORMATION::RightOffset;

            if (CurType.Alpha < 1 - FLT_EPSILON)
              TransparentIndicesInfo.push_back(CurIndex);
            else
              IndicesInfo.push_back(CurIndex);

            if (CurBlock.Direction.x != 0)
            {
              if (CurBlock.Direction.x == 1)
                TexCoords = CurType.TexCoordFront;
              else
                TexCoords = CurType.TexCoordBack;
            } else if (CurBlock.Right.x != 0)
            {
              if (CurBlock.Right.x == 1)
                TexCoords = CurType.TexCoordRight;
              else
                TexCoords = CurType.TexCoordLeft;
            } else if (CurBlock.Up.x != 0)
            {
              if (CurBlock.Up.x == 1)
                TexCoords = CurType.TexCoordUp;
              else
                TexCoords = CurType.TexCoordDown;
            }

            WriteVertices[CurVertexOffset + 0].Position = glm::vec3(ChunkOffsetX + x + 1, y, ChunkOffsetZ + z);
            WriteVertices[CurVertexOffset + 1].Position = glm::vec3(ChunkOffsetX + x + 1, y + 1, ChunkOffsetZ + z);
            WriteVertices[CurVertexOffset + 2].Position = glm::vec3(ChunkOffsetX + x + 1, y + 1, ChunkOffsetZ + z + 1);
            WriteVertices[CurVertexOffset + 3].Position = glm::vec3(ChunkOffsetX + x + 1, y, ChunkOffsetZ + z + 1);

            WriteVertices[CurVertexOffset + 0].TexCoord = TexCoords[0];
            WriteVertices[CurVertexOffset + 1].TexCoord = TexCoords[1];
            WriteVertices[CurVertexOffset + 2].TexCoord = TexCoords[2];
            WriteVertices[CurVertexOffset + 3].TexCoord = TexCoords[3];

            WriteVertices[CurVertexOffset + 0].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 1].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 2].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 3].Alpha = CurType.Alpha;

            WriteIndices[CurIndexOffset + 0] = CurVertexOffset + 0;
            WriteIndices[CurIndexOffset + 1] = CurVertexOffset + 1;
            WriteIndices[CurIndexOffset + 2] = CurVertexOffset + 2;
            WriteIndices[CurIndexOffset + 3] = CurVertexOffset + 0;
            WriteIndices[CurIndexOffset + 4] = CurVertexOffset + 2;
            WriteIndices[CurIndexOffset + 5] = CurVertexOffset + 3;

            if (CurType.Alpha >= 1 - FLT_EPSILON)
              CurBorder++;
          }

          if (y == 0 || BLOCK_TYPE::Table[Blocks[z * (UINT64)ChunkSizeY * ChunkSizeX + (y - 1) * (UINT64)ChunkSizeX +
                                                 x].BlockTypeId].Alpha < 1 - FLT_EPSILON)
          {
            UINT64 CurVertexOffset = 4 * CurBorder;
            UINT64 CurIndexOffset = 6 * CurBorder;
            const glm::vec2 *TexCoords = nullptr;

            CurBlockInfo.DownOffset = CurBorder;

            if (CurType.Alpha < 1 - FLT_EPSILON)
            {
              CurTransparentBorder++;
              CurVertexOffset = MaxNumberOfVertices - CurTransparentBorder * 4;
              CurIndexOffset = MaxNumberOfIndices - CurTransparentBorder * 6;
              CurBlockInfo.DownOffset = MaxNumberOfBorders - CurTransparentBorder;
            }

            INDEX_INFORMATION CurIndex;

            CurIndex.BlockId = BlockId;
            CurIndex.Offset = &BLOCK_INFORMATION::DownOffset;

            if (CurType.Alpha < 1 - FLT_EPSILON)
              TransparentIndicesInfo.push_back(CurIndex);
            else
              IndicesInfo.push_back(CurIndex);
            
            if (CurBlock.Direction.y != 0)
            {
              if (CurBlock.Direction.y == -1)
                TexCoords = CurType.TexCoordFront;
              else
                TexCoords = CurType.TexCoordBack;
            } else if (CurBlock.Right.y != 0)
            {
              if (CurBlock.Right.y == -1)
                TexCoords = CurType.TexCoordRight;
              else
                TexCoords = CurType.TexCoordLeft;
            } else if (CurBlock.Up.y != 0)
            {
              if (CurBlock.Up.y == -1)
                TexCoords = CurType.TexCoordUp;
              else
                TexCoords = CurType.TexCoordDown;
            }

            WriteVertices[CurVertexOffset + 0].Position = glm::vec3(ChunkOffsetX + x, y, ChunkOffsetZ + z);
            WriteVertices[CurVertexOffset + 1].Position = glm::vec3(ChunkOffsetX + x + 1, y, ChunkOffsetZ + z);
            WriteVertices[CurVertexOffset + 2].Position = glm::vec3(ChunkOffsetX + x + 1, y, ChunkOffsetZ + z + 1);
            WriteVertices[CurVertexOffset + 3].Position = glm::vec3(ChunkOffsetX + x, y, ChunkOffsetZ + z + 1);

            WriteVertices[CurVertexOffset + 0].TexCoord = TexCoords[0];
            WriteVertices[CurVertexOffset + 1].TexCoord = TexCoords[1];
            WriteVertices[CurVertexOffset + 2].TexCoord = TexCoords[2];
            WriteVertices[CurVertexOffset + 3].TexCoord = TexCoords[3];

            WriteVertices[CurVertexOffset + 0].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 1].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 2].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 3].Alpha = CurType.Alpha;

            WriteIndices[CurIndexOffset + 0] = CurVertexOffset + 0;
            WriteIndices[CurIndexOffset + 1] = CurVertexOffset + 1;
            WriteIndices[CurIndexOffset + 2] = CurVertexOffset + 2;
            WriteIndices[CurIndexOffset + 3] = CurVertexOffset + 0;
            WriteIndices[CurIndexOffset + 4] = CurVertexOffset + 2;
            WriteIndices[CurIndexOffset + 5] = CurVertexOffset + 3;

            if (CurType.Alpha >= 1 - FLT_EPSILON)
              CurBorder++;
          }

          if (y == ChunkSizeY - 1 ||
              BLOCK_TYPE::Table[Blocks[z * (UINT64)ChunkSizeY * ChunkSizeX + (y + 1) * (UINT64)ChunkSizeX +
                                       x].BlockTypeId].Alpha < 1 - FLT_EPSILON)
          {
            UINT64 CurVertexOffset = 4 * CurBorder;
            UINT64 CurIndexOffset = 6 * CurBorder;
            const glm::vec2 *TexCoords = nullptr;

            CurBlockInfo.UpOffset = CurBorder;

            if (CurType.Alpha < 1 - FLT_EPSILON)
            {
              CurTransparentBorder++;
              CurVertexOffset = MaxNumberOfVertices - CurTransparentBorder * 4;
              CurIndexOffset = MaxNumberOfIndices - CurTransparentBorder * 6;
              CurBlockInfo.UpOffset = MaxNumberOfBorders - CurTransparentBorder;
            }

            INDEX_INFORMATION CurIndex;

            CurIndex.BlockId = BlockId;
            CurIndex.Offset = &BLOCK_INFORMATION::UpOffset;

            if (CurType.Alpha < 1 - FLT_EPSILON)
              TransparentIndicesInfo.push_back(CurIndex);
            else
              IndicesInfo.push_back(CurIndex);
            
            if (CurBlock.Direction.y != 0)
            {
              if (CurBlock.Direction.y == 1)
                TexCoords = CurType.TexCoordFront;
              else
                TexCoords = CurType.TexCoordBack;
            } else if (CurBlock.Right.y != 0)
            {
              if (CurBlock.Right.y == 1)
                TexCoords = CurType.TexCoordRight;
              else
                TexCoords = CurType.TexCoordLeft;
            } else if (CurBlock.Up.y != 0)
            {
              if (CurBlock.Up.y == 1)
                TexCoords = CurType.TexCoordUp;
              else
                TexCoords = CurType.TexCoordDown;
            }

            WriteVertices[CurVertexOffset + 0].Position = glm::vec3(ChunkOffsetX + x, y + 1, ChunkOffsetZ + z);
            WriteVertices[CurVertexOffset + 1].Position = glm::vec3(ChunkOffsetX + x + 1, y + 1, ChunkOffsetZ + z);
            WriteVertices[CurVertexOffset + 2].Position = glm::vec3(ChunkOffsetX + x + 1, y + 1, ChunkOffsetZ + z + 1);
            WriteVertices[CurVertexOffset + 3].Position = glm::vec3(ChunkOffsetX + x, y + 1, ChunkOffsetZ + z + 1);

            WriteVertices[CurVertexOffset + 0].TexCoord = TexCoords[0];
            WriteVertices[CurVertexOffset + 1].TexCoord = TexCoords[1];
            WriteVertices[CurVertexOffset + 2].TexCoord = TexCoords[2];
            WriteVertices[CurVertexOffset + 3].TexCoord = TexCoords[3];

            WriteVertices[CurVertexOffset + 0].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 1].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 2].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 3].Alpha = CurType.Alpha;

            WriteIndices[CurIndexOffset + 0] = CurVertexOffset + 0;
            WriteIndices[CurIndexOffset + 1] = CurVertexOffset + 1;
            WriteIndices[CurIndexOffset + 2] = CurVertexOffset + 2;
            WriteIndices[CurIndexOffset + 3] = CurVertexOffset + 0;
            WriteIndices[CurIndexOffset + 4] = CurVertexOffset + 2;
            WriteIndices[CurIndexOffset + 5] = CurVertexOffset + 3;

            if (CurType.Alpha >= 1 - FLT_EPSILON)
              CurBorder++;
          }

          if (z == 0 || BLOCK_TYPE::Table[Blocks[(z - 1) * (UINT64)ChunkSizeY * ChunkSizeX + y * (UINT64)ChunkSizeX +
                                                 x].BlockTypeId].Alpha < 1 - FLT_EPSILON)
          {
            UINT64 CurVertexOffset = 4 * CurBorder;
            UINT64 CurIndexOffset = 6 * CurBorder;
            const glm::vec2 *TexCoords = nullptr;

            CurBlockInfo.BackOffset = CurBorder;

            if (CurType.Alpha < 1 - FLT_EPSILON)
            {
              CurTransparentBorder++;
              CurVertexOffset = MaxNumberOfVertices - CurTransparentBorder * 4;
              CurIndexOffset = MaxNumberOfIndices - CurTransparentBorder * 6;
              CurBlockInfo.BackOffset = MaxNumberOfBorders - CurTransparentBorder;
            }

            INDEX_INFORMATION CurIndex;

            CurIndex.BlockId = BlockId;
            CurIndex.Offset = &BLOCK_INFORMATION::BackOffset;

            if (CurType.Alpha < 1 - FLT_EPSILON)
              TransparentIndicesInfo.push_back(CurIndex);
            else
              IndicesInfo.push_back(CurIndex);
            
            if (CurBlock.Direction.z != 0)
            {
              if (CurBlock.Direction.z == -1)
                TexCoords = CurType.TexCoordFront;
              else
                TexCoords = CurType.TexCoordBack;
            } else if (CurBlock.Right.z != 0)
            {
              if (CurBlock.Right.z == -1)
                TexCoords = CurType.TexCoordRight;
              else
                TexCoords = CurType.TexCoordLeft;
            } else if (CurBlock.Up.z != 0)
            {
              if (CurBlock.Up.z == -1)
                TexCoords = CurType.TexCoordUp;
              else
                TexCoords = CurType.TexCoordDown;
            }

            WriteVertices[CurVertexOffset + 0].Position = glm::vec3(ChunkOffsetX + x, y, ChunkOffsetZ + z);
            WriteVertices[CurVertexOffset + 1].Position = glm::vec3(ChunkOffsetX + x, y + 1, ChunkOffsetZ + z);
            WriteVertices[CurVertexOffset + 2].Position = glm::vec3(ChunkOffsetX + x + 1, y + 1, ChunkOffsetZ + z);
            WriteVertices[CurVertexOffset + 3].Position = glm::vec3(ChunkOffsetX + x + 1, y, ChunkOffsetZ + z);

            //WriteVertices[CurVertexOffset + 0].Position =
            //  glm::vec3(ChunkOffsetX + x, y, ChunkOffsetZ + z);
            //WriteVertices[CurVertexOffset + 1].Position =
            //  glm::vec3(ChunkOffsetX + x + 1, y, ChunkOffsetZ + z);
            //WriteVertices[CurVertexOffset + 2].Position =
            //  glm::vec3(ChunkOffsetX + x + 1, y + 1, ChunkOffsetZ + z);
            //WriteVertices[CurVertexOffset + 3].Position =
            //  glm::vec3(ChunkOffsetX + x, y + 1, ChunkOffsetZ + z);

            WriteVertices[CurVertexOffset + 0].TexCoord = TexCoords[0];
            WriteVertices[CurVertexOffset + 1].TexCoord = TexCoords[1];
            WriteVertices[CurVertexOffset + 2].TexCoord = TexCoords[2];
            WriteVertices[CurVertexOffset + 3].TexCoord = TexCoords[3];

            WriteVertices[CurVertexOffset + 0].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 1].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 2].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 3].Alpha = CurType.Alpha;

            WriteIndices[CurIndexOffset + 0] = CurVertexOffset + 0;
            WriteIndices[CurIndexOffset + 1] = CurVertexOffset + 1;
            WriteIndices[CurIndexOffset + 2] = CurVertexOffset + 2;
            WriteIndices[CurIndexOffset + 3] = CurVertexOffset + 0;
            WriteIndices[CurIndexOffset + 4] = CurVertexOffset + 2;
            WriteIndices[CurIndexOffset + 5] = CurVertexOffset + 3;

            if (CurType.Alpha >= 1 - FLT_EPSILON)
              CurBorder++;
          }

          if (z == ChunkSizeZ - 1 ||
              BLOCK_TYPE::Table[Blocks[(z + 1) * (UINT64)ChunkSizeY * ChunkSizeX + y * (UINT64)ChunkSizeX +
                                       x].BlockTypeId].Alpha < 1 - FLT_EPSILON)
          {
            UINT64 CurVertexOffset = 4 * CurBorder;
            UINT64 CurIndexOffset = 6 * CurBorder;
            const glm::vec2 *TexCoords = nullptr;

            CurBlockInfo.FrontOffset = CurBorder;

            if (CurType.Alpha < 1 - FLT_EPSILON)
            {
              CurTransparentBorder++;
              CurVertexOffset = MaxNumberOfVertices - CurTransparentBorder * 4;
              CurIndexOffset = MaxNumberOfIndices - CurTransparentBorder * 6;
              CurBlockInfo.FrontOffset = MaxNumberOfBorders - CurTransparentBorder;
            }

            INDEX_INFORMATION CurIndex;

            CurIndex.BlockId = BlockId;
            CurIndex.Offset = &BLOCK_INFORMATION::FrontOffset;

            if (CurType.Alpha < 1 - FLT_EPSILON)
              TransparentIndicesInfo.push_back(CurIndex);
            else
              IndicesInfo.push_back(CurIndex);
            
            if (CurBlock.Direction.z != 0)
            {
              if (CurBlock.Direction.z == 1)
                TexCoords = CurType.TexCoordFront;
              else
                TexCoords = CurType.TexCoordBack;
            } else if (CurBlock.Right.z != 0)
            {
              if (CurBlock.Right.z == 1)
                TexCoords = CurType.TexCoordRight;
              else
                TexCoords = CurType.TexCoordLeft;
            } else if (CurBlock.Up.z != 0)
            {
              if (CurBlock.Up.z == 1)
                TexCoords = CurType.TexCoordUp;
              else
                TexCoords = CurType.TexCoordDown;
            }

            WriteVertices[CurVertexOffset + 0].Position = glm::vec3(ChunkOffsetX + x, y, ChunkOffsetZ + z + 1);
            WriteVertices[CurVertexOffset + 1].Position = glm::vec3(ChunkOffsetX + x, y + 1, ChunkOffsetZ + z + 1);
            WriteVertices[CurVertexOffset + 2].Position = glm::vec3(ChunkOffsetX + x + 1, y + 1, ChunkOffsetZ + z + 1);
            WriteVertices[CurVertexOffset + 3].Position = glm::vec3(ChunkOffsetX + x + 1, y, ChunkOffsetZ + z + 1);

            //WriteVertices[CurVertexOffset + 0].Position =
            //  glm::vec3(ChunkOffsetX + x, y, ChunkOffsetZ + z + 1);
            //WriteVertices[CurVertexOffset + 1].Position =
            //  glm::vec3(ChunkOffsetX + x + 1, y, ChunkOffsetZ + z + 1);
            //WriteVertices[CurVertexOffset + 2].Position =
            //  glm::vec3(ChunkOffsetX + x + 1, y + 1, ChunkOffsetZ + z + 1);
            //WriteVertices[CurVertexOffset + 3].Position =
            //  glm::vec3(ChunkOffsetX + x, y + 1, ChunkOffsetZ + z + 1);

            WriteVertices[CurVertexOffset + 0].TexCoord = TexCoords[0];
            WriteVertices[CurVertexOffset + 1].TexCoord = TexCoords[1];
            WriteVertices[CurVertexOffset + 2].TexCoord = TexCoords[2];
            WriteVertices[CurVertexOffset + 3].TexCoord = TexCoords[3];

            WriteVertices[CurVertexOffset + 0].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 1].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 2].Alpha = CurType.Alpha;
            WriteVertices[CurVertexOffset + 3].Alpha = CurType.Alpha;

            WriteIndices[CurIndexOffset + 0] = CurVertexOffset + 0;
            WriteIndices[CurIndexOffset + 1] = CurVertexOffset + 1;
            WriteIndices[CurIndexOffset + 2] = CurVertexOffset + 2;
            WriteIndices[CurIndexOffset + 3] = CurVertexOffset + 0;
            WriteIndices[CurIndexOffset + 4] = CurVertexOffset + 2;
            WriteIndices[CurIndexOffset + 5] = CurVertexOffset + 3;

            if (CurType.Alpha >= 1 - FLT_EPSILON)
              CurBorder++;
          }
        }
  }

  Render.MemoryManager.PushMemory(VertexBufferSize, VertexBufferOffset,
    VertexMemory, VertexBuffer, Render.MemoryManager.NeedCopyVertex,
    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);

  NumberOfVertices = 4 * CurBorder;
  NumberOfIndices = 6 * CurBorder;
  NumberOfBorders = CurBorder;

  NumberOfTransparentVertices = 4 * CurTransparentBorder;
  NumberOfTransparentIndices = 6 * CurTransparentBorder;
  NumberOfTransparentBorders = CurTransparentBorder;

  if (NumberOfIndices == 0 && NumberOfTransparentIndices == 0)
  {
    std::cout << "Empty chunk-mb its error\n" << std::endl;
  }
  else
  {
    BYTE *WriteIndexMemory = Render.MemoryManager.GetMemoryForWriting(IndexBufferSize,
                                                                      IndexBufferOffset, IndexMemory,
                                                                      Render.MemoryManager.NeedCopyIndex);

    memcpy(WriteIndexMemory, WriteIndices.data(), IndexBufferSize);

    Render.MemoryManager.PushMemory(IndexBufferSize, IndexBufferOffset, IndexMemory, IndexBuffer,
                                    Render.MemoryManager.NeedCopyIndex, VK_ACCESS_INDEX_READ_BIT,
                                    VK_ACCESS_INDEX_READ_BIT);
  }

  CommandBufferId = Render.GetSecondaryCommandBuffer();
  TransparentCommandBufferId = Render.GetSecondaryCommandBuffer();

  {
    std::lock_guard<std::mutex> Lock(Render.Synchronization.RenderMutex);

    CreateCommandBuffer();
  }

  Render.AddDrawElement(this);
}

/**
 * \brief Remove up border function
 * \param[in] BlockPos Block position
 */
VOID chunk_geometry::RemoveUpBorder( const glm::ivec3 &BlockPos )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  if (BlocksInfo[BlockInd].UpOffset == -1)
    return;

  if (BlocksInfo[BlockInd].UpOffset < NumberOfBorders)
  {
    if (BlocksInfo[BlockInd].UpOffset == NumberOfBorders - 1)
    {
      NumberOfBorders--;
      NumberOfIndices -= 6;
      NumberOfVertices -= 4;
      IndicesInfo.pop_back();
    }
    else
    {
      //Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.IndexBuffer,
      //                                      Render.MemoryManager.IndexBuffer,
      //                                      IndexBufferOffset + (NumberOfBorders - 1) * 6 * sizeof(UINT32),
      //                                      IndexBufferOffset + BlocksInfo[BlockInd].UpOffset * 6 * sizeof(UINT32),
      //                                      6 * sizeof(UINT32),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex);

      Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.VertexBuffer, Render.MemoryManager.VertexBuffer,
                                            VertexBufferOffset + (NumberOfBorders - 1) * 4 * sizeof(VERTEX),
                                            VertexBufferOffset + BlocksInfo[BlockInd].UpOffset * 4 * sizeof(VERTEX),
                                            4 * sizeof(VERTEX),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex);

      //assert(NumberOfBorders * 4 == NumberOfVertices);
      //assert(NumberOfBorders * 6 == NumberOfIndices);
      //std::cout << "RMUp (x6): " << (NumberOfBorders - 1) * 6 << " -> " << BlocksInfo[BlockInd].UpOffset * 6 << "\n" << std::endl;

      BlocksInfo[IndicesInfo[NumberOfBorders - 1].BlockId].*
      IndicesInfo[NumberOfBorders - 1].Offset = BlocksInfo[BlockInd].UpOffset;
      std::swap(IndicesInfo[BlocksInfo[BlockInd].UpOffset], IndicesInfo[NumberOfBorders - 1]);
      IndicesInfo.pop_back();

      NumberOfBorders--;
      NumberOfIndices -= 6;
      NumberOfVertices -= 4;
    }
  }
  else
  {
    if (BlocksInfo[BlockInd].UpOffset == MaxNumberOfBorders - NumberOfTransparentBorders)
    {
      NumberOfTransparentBorders--;
      NumberOfTransparentIndices -= 6;
      NumberOfTransparentVertices -= 4;
      TransparentIndicesInfo.pop_back();
    }
    else
    {
      //Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.IndexBuffer,
      //                                      Render.MemoryManager.IndexBuffer,
      //                                      IndexBufferOffset + (NumberOfBorders - 1) * 6 * sizeof(UINT32),
      //                                      IndexBufferOffset + BlocksInfo[BlockInd].UpOffset * 6 * sizeof(UINT32),
      //                                      6 * sizeof(UINT32),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex);

      Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.VertexBuffer, Render.MemoryManager.VertexBuffer,
                                            VertexBufferOffset + (MaxNumberOfVertices - NumberOfTransparentVertices) * sizeof(VERTEX),
                                            VertexBufferOffset + BlocksInfo[BlockInd].UpOffset * 4 * sizeof(VERTEX),
                                            4 * sizeof(VERTEX),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex);

      BlocksInfo[TransparentIndicesInfo[NumberOfTransparentBorders - 1].BlockId].*
        TransparentIndicesInfo[NumberOfTransparentBorders - 1].Offset = BlocksInfo[BlockInd].UpOffset;
      std::swap(TransparentIndicesInfo[MaxNumberOfBorders - BlocksInfo[BlockInd].UpOffset - 1],
                TransparentIndicesInfo[NumberOfTransparentBorders - 1]);
      TransparentIndicesInfo.pop_back();

      NumberOfTransparentBorders--;
      NumberOfTransparentIndices -= 6;
      NumberOfTransparentVertices -= 4;
    }
  }

  BlocksInfo[BlockInd].UpOffset = -1;
}

/**
 * \brief Remove down border function
 * \param[in] BlockPos Block position
 */
VOID chunk_geometry::RemoveDownBorder( const glm::ivec3 &BlockPos )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  if (BlocksInfo[BlockInd].DownOffset == -1)
    return;

  if (BlocksInfo[BlockInd].DownOffset < NumberOfBorders)
  {
    if (BlocksInfo[BlockInd].DownOffset == NumberOfBorders - 1)
    {
      NumberOfBorders--;
      NumberOfIndices -= 6;
      NumberOfVertices -= 4;
      IndicesInfo.pop_back();
    }
    else
    {
      //Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.IndexBuffer,
      //                                      Render.MemoryManager.IndexBuffer,
      //                                      IndexBufferOffset + (NumberOfBorders - 1) * 6 * sizeof(UINT32),
      //                                      IndexBufferOffset + BlocksInfo[BlockInd].DownOffset * 6 * sizeof(UINT32),
      //                                      6 * sizeof(UINT32),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex);

      Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.VertexBuffer,
                                            Render.MemoryManager.VertexBuffer,
                                            VertexBufferOffset + (NumberOfBorders - 1) * 4 * sizeof(VERTEX),
                                            VertexBufferOffset + BlocksInfo[BlockInd].DownOffset * 4 * sizeof(VERTEX),
                                            4 * sizeof(VERTEX),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex);

      //assert(NumberOfBorders * 4 == NumberOfVertices);
      //assert(NumberOfBorders * 6 == NumberOfIndices);
      //std::cout << "RMDown (x6): " << (NumberOfBorders - 1) * 6 << " -> " << BlocksInfo[BlockInd].DownOffset * 6 << "\n" << std::endl;

      BlocksInfo[IndicesInfo[NumberOfBorders - 1].BlockId].*IndicesInfo[NumberOfBorders - 1].Offset =
        BlocksInfo[BlockInd].DownOffset;
      std::swap(IndicesInfo[BlocksInfo[BlockInd].DownOffset], IndicesInfo[NumberOfBorders - 1]);
      IndicesInfo.pop_back();

      NumberOfBorders--;
      NumberOfIndices -= 6;
      NumberOfVertices -= 4;
    }
  }
  else
  {
    if (BlocksInfo[BlockInd].DownOffset == MaxNumberOfBorders - NumberOfTransparentBorders)
    {
      NumberOfTransparentBorders--;
      NumberOfTransparentIndices -= 6;
      NumberOfTransparentVertices -= 4;
      TransparentIndicesInfo.pop_back();
    }
    else
    {
      //Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.IndexBuffer,
      //                                      Render.MemoryManager.IndexBuffer,
      //                                      IndexBufferOffset + (NumberOfBorders - 1) * 6 * sizeof(UINT32),
      //                                      IndexBufferOffset + BlocksInfo[BlockInd].UpOffset * 6 * sizeof(UINT32),
      //                                      6 * sizeof(UINT32),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex);

      Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.VertexBuffer, Render.MemoryManager.VertexBuffer,
                                            VertexBufferOffset + (MaxNumberOfVertices - NumberOfTransparentVertices) * sizeof(VERTEX),
                                            VertexBufferOffset + BlocksInfo[BlockInd].DownOffset * 4 * sizeof(VERTEX),
                                            4 * sizeof(VERTEX),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex);

      BlocksInfo[TransparentIndicesInfo[NumberOfTransparentBorders - 1].BlockId].*
      TransparentIndicesInfo[NumberOfTransparentBorders - 1].Offset = BlocksInfo[BlockInd].DownOffset;
      std::swap(TransparentIndicesInfo[MaxNumberOfBorders - BlocksInfo[BlockInd].DownOffset - 1],
                TransparentIndicesInfo[NumberOfTransparentBorders - 1]);
      TransparentIndicesInfo.pop_back();

      NumberOfTransparentBorders--;
      NumberOfTransparentIndices -= 6;
      NumberOfTransparentVertices -= 4;
    }
  }

  BlocksInfo[BlockInd].DownOffset = -1;
}

/**
 * \brief Remove right border function
 * \param[in] BlockPos Block position
 */
VOID chunk_geometry::RemoveRightBorder( const glm::ivec3 &BlockPos )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  if (BlocksInfo[BlockInd].RightOffset == -1)
    return;

  if (BlocksInfo[BlockInd].RightOffset < NumberOfBorders)
  {
    if (BlocksInfo[BlockInd].RightOffset == NumberOfBorders - 1)
    {
      NumberOfBorders--;
      NumberOfIndices -= 6;
      NumberOfVertices -= 4;
      IndicesInfo.pop_back();
    }
    else
    {
      //Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.IndexBuffer,
      //                                      Render.MemoryManager.IndexBuffer,
      //                                      IndexBufferOffset + (NumberOfBorders - 1) * 6 * sizeof(UINT32),
      //                                      IndexBufferOffset + BlocksInfo[BlockInd].RightOffset * 6 * sizeof(UINT32),
      //                                      6 * sizeof(UINT32),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex);

      Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.VertexBuffer,
                                            Render.MemoryManager.VertexBuffer,
                                            VertexBufferOffset + (NumberOfBorders - 1) * 4 * sizeof(VERTEX),
                                            VertexBufferOffset + BlocksInfo[BlockInd].RightOffset * 4 * sizeof(VERTEX),
                                            4 * sizeof(VERTEX),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex);

      //assert(NumberOfBorders * 4 == NumberOfVertices);
      //assert(NumberOfBorders * 6 == NumberOfIndices);
      //std::cout << "RMRight (x6): " << (NumberOfBorders - 1) * 6 << " -> " << BlocksInfo[BlockInd].RightOffset * 6 << "\n" << std::endl;

      BlocksInfo[IndicesInfo[NumberOfBorders - 1].BlockId].*IndicesInfo[NumberOfBorders - 1].Offset =
        BlocksInfo[BlockInd].RightOffset;
      std::swap(IndicesInfo[BlocksInfo[BlockInd].RightOffset], IndicesInfo[NumberOfBorders - 1]);
      IndicesInfo.pop_back();

      NumberOfBorders--;
      NumberOfIndices -= 6;
      NumberOfVertices -= 4;
    }
  }
  else
  {
    if (BlocksInfo[BlockInd].RightOffset == MaxNumberOfBorders - NumberOfTransparentBorders)
    {
      NumberOfTransparentBorders--;
      NumberOfTransparentIndices -= 6;
      NumberOfTransparentVertices -= 4;
      TransparentIndicesInfo.pop_back();
    }
    else
    {
      //Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.IndexBuffer,
      //                                      Render.MemoryManager.IndexBuffer,
      //                                      IndexBufferOffset + (NumberOfBorders - 1) * 6 * sizeof(UINT32),
      //                                      IndexBufferOffset + BlocksInfo[BlockInd].UpOffset * 6 * sizeof(UINT32),
      //                                      6 * sizeof(UINT32),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex);

      Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.VertexBuffer, Render.MemoryManager.VertexBuffer,
                                            VertexBufferOffset + (MaxNumberOfVertices - NumberOfTransparentVertices) * sizeof(VERTEX),
                                            VertexBufferOffset + BlocksInfo[BlockInd].RightOffset * 4 * sizeof(VERTEX),
                                            4 * sizeof(VERTEX),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex);

      BlocksInfo[TransparentIndicesInfo[NumberOfTransparentBorders - 1].BlockId].*
      TransparentIndicesInfo[NumberOfTransparentBorders - 1].Offset = BlocksInfo[BlockInd].RightOffset;
      std::swap(TransparentIndicesInfo[MaxNumberOfBorders - BlocksInfo[BlockInd].RightOffset - 1],
                TransparentIndicesInfo[NumberOfTransparentBorders - 1]);
      TransparentIndicesInfo.pop_back();

      NumberOfTransparentBorders--;
      NumberOfTransparentIndices -= 6;
      NumberOfTransparentVertices -= 4;
    }
  }

  BlocksInfo[BlockInd].RightOffset = -1;
}

/**
 * \brief Remove left border function
 * \param[in] BlockPos Block position
 */
VOID chunk_geometry::RemoveLeftBorder( const glm::ivec3 &BlockPos )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  if (BlocksInfo[BlockInd].LeftOffset == -1)
    return;

  if (BlocksInfo[BlockInd].LeftOffset < NumberOfBorders)
  {
    if (BlocksInfo[BlockInd].LeftOffset == NumberOfBorders - 1)
    {
      NumberOfBorders--;
      NumberOfIndices -= 6;
      NumberOfVertices -= 4;
      IndicesInfo.pop_back();
    }
    else
    {
      //Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.IndexBuffer,
      //                                      Render.MemoryManager.IndexBuffer,
      //                                      IndexBufferOffset + (NumberOfBorders - 1) * 6 * sizeof(UINT32),
      //                                      IndexBufferOffset + BlocksInfo[BlockInd].LeftOffset * 6 * sizeof(UINT32),
      //                                      6 * sizeof(UINT32),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex);

      Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.VertexBuffer,
                                            Render.MemoryManager.VertexBuffer,
                                            VertexBufferOffset + (NumberOfBorders - 1) * 4 * sizeof(VERTEX),
                                            VertexBufferOffset + BlocksInfo[BlockInd].LeftOffset * 4 * sizeof(VERTEX),
                                            4 * sizeof(VERTEX),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex);

      //assert(NumberOfBorders * 4 == NumberOfVertices);
      //assert(NumberOfBorders * 6 == NumberOfIndices);
      //std::cout << "RMLeft (x6): " << (NumberOfBorders - 1) * 6 << " -> " << BlocksInfo[BlockInd].LeftOffset * 6 << "\n" << std::endl;

      BlocksInfo[IndicesInfo[NumberOfBorders - 1].BlockId].*IndicesInfo[NumberOfBorders - 1].Offset =
        BlocksInfo[BlockInd].LeftOffset;
      std::swap(IndicesInfo[BlocksInfo[BlockInd].LeftOffset], IndicesInfo[NumberOfBorders - 1]);
      IndicesInfo.pop_back();

      NumberOfBorders--;
      NumberOfIndices -= 6;
      NumberOfVertices -= 4;
    }
  }
  else
  {
    if (BlocksInfo[BlockInd].LeftOffset == MaxNumberOfBorders - NumberOfTransparentBorders)
    {
      NumberOfTransparentBorders--;
      NumberOfTransparentIndices -= 6;
      NumberOfTransparentVertices -= 4;
      TransparentIndicesInfo.pop_back();
    }
    else
    {
      //Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.IndexBuffer,
      //                                      Render.MemoryManager.IndexBuffer,
      //                                      IndexBufferOffset + (NumberOfBorders - 1) * 6 * sizeof(UINT32),
      //                                      IndexBufferOffset + BlocksInfo[BlockInd].UpOffset * 6 * sizeof(UINT32),
      //                                      6 * sizeof(UINT32),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex);

      Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.VertexBuffer, Render.MemoryManager.VertexBuffer,
                                            VertexBufferOffset + (MaxNumberOfVertices - NumberOfTransparentVertices) * sizeof(VERTEX),
                                            VertexBufferOffset + BlocksInfo[BlockInd].LeftOffset * 4 * sizeof(VERTEX),
                                            4 * sizeof(VERTEX),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex);

      BlocksInfo[TransparentIndicesInfo[NumberOfTransparentBorders - 1].BlockId].*
      TransparentIndicesInfo[NumberOfTransparentBorders - 1].Offset = BlocksInfo[BlockInd].LeftOffset;
      std::swap(TransparentIndicesInfo[MaxNumberOfBorders - BlocksInfo[BlockInd].LeftOffset - 1],
                TransparentIndicesInfo[NumberOfTransparentBorders - 1]);
      TransparentIndicesInfo.pop_back();

      NumberOfTransparentBorders--;
      NumberOfTransparentIndices -= 6;
      NumberOfTransparentVertices -= 4;
    }
  }

  BlocksInfo[BlockInd].LeftOffset = -1;
}

/**
 * \brief Remove front border function
 * \param[in] BlockPos Block position
 */
VOID chunk_geometry::RemoveFrontBorder( const glm::ivec3 &BlockPos )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  if (BlocksInfo[BlockInd].FrontOffset == -1)
    return;

  if (BlocksInfo[BlockInd].FrontOffset < NumberOfBorders)
  {
    if (BlocksInfo[BlockInd].FrontOffset == NumberOfBorders - 1)
    {
      NumberOfBorders--;
      NumberOfIndices -= 6;
      NumberOfVertices -= 4;
      IndicesInfo.pop_back();
    }
    else
    {
      //Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.IndexBuffer,
      //                                      Render.MemoryManager.IndexBuffer,
      //                                      IndexBufferOffset + (NumberOfBorders - 1) * 6 * sizeof(UINT32),
      //                                      IndexBufferOffset + BlocksInfo[BlockInd].FrontOffset * 6 * sizeof(UINT32),
      //                                      6 * sizeof(UINT32),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex);

      Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.VertexBuffer,
                                            Render.MemoryManager.VertexBuffer,
                                            VertexBufferOffset + (NumberOfBorders - 1) * 4 * sizeof(VERTEX),
                                            VertexBufferOffset + BlocksInfo[BlockInd].FrontOffset * 4 * sizeof(VERTEX),
                                            4 * sizeof(VERTEX),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex);

      //assert(NumberOfBorders * 4 == NumberOfVertices);
      //assert(NumberOfBorders * 6 == NumberOfIndices);
      //std::cout << "RMFront (x6): " << (NumberOfBorders - 1) * 6 << " -> " << BlocksInfo[BlockInd].FrontOffset * 6 << "\n" << std::endl;

      BlocksInfo[IndicesInfo[NumberOfBorders - 1].BlockId].*IndicesInfo[NumberOfBorders - 1].Offset =
        BlocksInfo[BlockInd].FrontOffset;
      std::swap(IndicesInfo[BlocksInfo[BlockInd].FrontOffset], IndicesInfo[NumberOfBorders - 1]);
      IndicesInfo.pop_back();

      NumberOfBorders--;
      NumberOfIndices -= 6;
      NumberOfVertices -= 4;
    }
  }
  else
  {
    if (BlocksInfo[BlockInd].FrontOffset == MaxNumberOfBorders - NumberOfTransparentBorders)
    {
      NumberOfTransparentBorders--;
      NumberOfTransparentIndices -= 6;
      NumberOfTransparentVertices -= 4;
      TransparentIndicesInfo.pop_back();
    }
    else
    {
      //Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.IndexBuffer,
      //                                      Render.MemoryManager.IndexBuffer,
      //                                      IndexBufferOffset + (NumberOfBorders - 1) * 6 * sizeof(UINT32),
      //                                      IndexBufferOffset + BlocksInfo[BlockInd].UpOffset * 6 * sizeof(UINT32),
      //                                      6 * sizeof(UINT32),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex);

      Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.VertexBuffer, Render.MemoryManager.VertexBuffer,
                                            VertexBufferOffset + (MaxNumberOfVertices - NumberOfTransparentVertices) * sizeof(VERTEX),
                                            VertexBufferOffset + BlocksInfo[BlockInd].FrontOffset * 4 * sizeof(VERTEX),
                                            4 * sizeof(VERTEX),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex);

      BlocksInfo[TransparentIndicesInfo[NumberOfTransparentBorders - 1].BlockId].*
      TransparentIndicesInfo[NumberOfTransparentBorders - 1].Offset = BlocksInfo[BlockInd].FrontOffset;
      std::swap(TransparentIndicesInfo[MaxNumberOfBorders - BlocksInfo[BlockInd].FrontOffset - 1],
                TransparentIndicesInfo[NumberOfTransparentBorders - 1]);
      TransparentIndicesInfo.pop_back();

      NumberOfTransparentBorders--;
      NumberOfTransparentIndices -= 6;
      NumberOfTransparentVertices -= 4;
    }
  }

  BlocksInfo[BlockInd].FrontOffset = -1;
}

/**
 * \brief Remove back border function
 * \param[in] BlockPos Block position
 */
VOID chunk_geometry::RemoveBackBorder( const glm::ivec3 &BlockPos )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  if (BlocksInfo[BlockInd].BackOffset == -1)
    return;

  if (BlocksInfo[BlockInd].BackOffset < NumberOfBorders)
  {
    if (BlocksInfo[BlockInd].BackOffset == NumberOfBorders - 1)
    {
      NumberOfBorders--;
      NumberOfIndices -= 6;
      NumberOfVertices -= 4;
      IndicesInfo.pop_back();
    }
    else
    {
      //Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.IndexBuffer,
      //                                      Render.MemoryManager.IndexBuffer,
      //                                      IndexBufferOffset + (NumberOfBorders - 1) * 6 * sizeof(UINT32),
      //                                      IndexBufferOffset + BlocksInfo[BlockInd].BackOffset * 6 * sizeof(UINT32),
      //                                      6 * sizeof(UINT32),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex);

      Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.VertexBuffer,
                                            Render.MemoryManager.VertexBuffer,
                                            VertexBufferOffset + (NumberOfBorders - 1) * 4 * sizeof(VERTEX),
                                            VertexBufferOffset + BlocksInfo[BlockInd].BackOffset * 4 * sizeof(VERTEX),
                                            4 * sizeof(VERTEX),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex);

      //assert(NumberOfBorders * 4 == NumberOfVertices);
      //assert(NumberOfBorders * 6 == NumberOfIndices);
      //std::cout << "RMBack (x6): " << (NumberOfBorders - 1) * 6 << " -> " << BlocksInfo[BlockInd].BackOffset * 6 << "\n" << std::endl;

      BlocksInfo[IndicesInfo[NumberOfBorders - 1].BlockId].*IndicesInfo[NumberOfBorders - 1].Offset =
        BlocksInfo[BlockInd].BackOffset;
      std::swap(IndicesInfo[BlocksInfo[BlockInd].BackOffset], IndicesInfo[NumberOfBorders - 1]);
      IndicesInfo.pop_back();

      NumberOfBorders--;
      NumberOfIndices -= 6;
      NumberOfVertices -= 4;
    }
  }
  else
  {
    if (BlocksInfo[BlockInd].BackOffset == MaxNumberOfBorders - NumberOfTransparentBorders)
    {
      NumberOfTransparentBorders--;
      NumberOfTransparentIndices -= 6;
      NumberOfTransparentVertices -= 4;
      TransparentIndicesInfo.pop_back();
    }
    else
    {
      //Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.IndexBuffer,
      //                                      Render.MemoryManager.IndexBuffer,
      //                                      IndexBufferOffset + (NumberOfBorders - 1) * 6 * sizeof(UINT32),
      //                                      IndexBufferOffset + BlocksInfo[BlockInd].UpOffset * 6 * sizeof(UINT32),
      //                                      6 * sizeof(UINT32),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex,
      //                                      *Render.VkApp.GraphicsQueueFamilyIndex);

      Render.MemoryManager.CopyBufferRegion(Render.MemoryManager.VertexBuffer, Render.MemoryManager.VertexBuffer,
                                            VertexBufferOffset + (MaxNumberOfVertices - NumberOfTransparentVertices) * sizeof(VERTEX),
                                            VertexBufferOffset + BlocksInfo[BlockInd].BackOffset * 4 * sizeof(VERTEX),
                                            4 * sizeof(VERTEX),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                             VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex,
                                            *Render.VkApp.GraphicsQueueFamilyIndex);

      BlocksInfo[TransparentIndicesInfo[NumberOfTransparentBorders - 1].BlockId].*
      TransparentIndicesInfo[NumberOfTransparentBorders - 1].Offset = BlocksInfo[BlockInd].BackOffset;
      std::swap(TransparentIndicesInfo[MaxNumberOfBorders - BlocksInfo[BlockInd].BackOffset - 1],
                TransparentIndicesInfo[NumberOfTransparentBorders - 1]);
      TransparentIndicesInfo.pop_back();

      NumberOfTransparentBorders--;
      NumberOfTransparentIndices -= 6;
      NumberOfTransparentVertices -= 4;
    }
  }

  BlocksInfo[BlockInd].BackOffset = -1;
}

/**
 * \brief Update command buffer function
 */
VOID chunk_geometry::UpdateCommandBuffer( VOID ) const
{
  std::lock_guard<std::mutex> Lock(Render.Synchronization.RenderMutex);

  {
    command_buffer CommandBuffer(CommandBufferId);

    CommandBuffer.Reset();
  }

  {
    command_buffer CommandBuffer(TransparentCommandBufferId);

    CommandBuffer.Reset();
  }

  CreateCommandBuffer();
}

/**
 * \brief Fill command buffer function
 */
VOID chunk_geometry::CreateCommandBuffer( VOID ) const
{
  VkCommandBufferInheritanceInfo InheritanceInfo = {};

  InheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
  InheritanceInfo.pNext = nullptr;
  InheritanceInfo.renderPass = Render.RenderPass.GetRenderPassId();
  InheritanceInfo.subpass = 0;
  InheritanceInfo.framebuffer = VK_NULL_HANDLE;
  InheritanceInfo.occlusionQueryEnable = VK_FALSE;
  InheritanceInfo.queryFlags = 0;
  InheritanceInfo.pipelineStatistics = 0;

  {
    command_buffer CommandBuffer(CommandBufferId);

    CommandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
                        &InheritanceInfo);

    CommandBuffer.CmdBindGraphicsPipeline(Render.DefaultGraphicsPipeline);

    //vkCmdPushConstants(CommandBufferId, Render.DefaultPipelineLayout.GetPipelineLayoutId(),
    //                   VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
    //                   reinterpret_cast<const VOID *>(&Render.AppliedMatrWVP));

    vkCmdBindDescriptorSets(CommandBufferId, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            Render.DefaultPipelineLayout.GetPipelineLayoutId(), 0, 1, &Render.DefaultDescriptorSet, 0,
                            nullptr);

    VkBuffer VertexBufferId = VertexBuffer.GetBufferId();
    
    vkCmdBindVertexBuffers(CommandBufferId, 0, 1, &VertexBufferId, &VertexBufferOffset);

    VkBuffer IndexBufferId = IndexBuffer.GetBufferId();

    vkCmdBindIndexBuffer(CommandBufferId, IndexBufferId, IndexBufferOffset, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(CommandBufferId, NumberOfIndices, 1, 0, 0, 0);

    CommandBuffer.End();
  }

  {
    command_buffer CommandBuffer(TransparentCommandBufferId);

    CommandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
                        &InheritanceInfo);

    CommandBuffer.CmdBindGraphicsPipeline(Render.DefaultGraphicsPipeline);

    //vkCmdPushConstants(CommandBufferId, Render.DefaultPipelineLayout.GetPipelineLayoutId(),
    //                   VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
    //                   reinterpret_cast<const VOID *>(&Render.AppliedMatrWVP));

    vkCmdBindDescriptorSets(TransparentCommandBufferId, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            Render.DefaultPipelineLayout.GetPipelineLayoutId(), 0, 1, &Render.DefaultDescriptorSet, 0,
                            nullptr);
    
    if (NumberOfTransparentBorders > 0)
    {
      //VkBuffer VertexBufferId = VertexBuffer.GetBufferId();
      //
      //UINT64 TransparentVertexBufferOffset = VertexBufferOffset + sizeof(VERTEX) * (MaxNumberOfVertices - NumberOfTransparentVertices);
      //
      //vkCmdBindVertexBuffers(TransparentCommandBufferId, 0, 1, &VertexBufferId, &TransparentVertexBufferOffset);
      //
      //VkBuffer IndexBufferId = IndexBuffer.GetBufferId();
      //
      //UINT64 TransparentIndexBufferOffset = IndexBufferOffset + sizeof(UINT32) * (MaxNumberOfIndices - NumberOfTransparentIndices);
      //
      //vkCmdBindIndexBuffer(TransparentCommandBufferId, IndexBufferId, TransparentIndexBufferOffset, VK_INDEX_TYPE_UINT32);
      //
      //vkCmdDrawIndexed(TransparentCommandBufferId, NumberOfTransparentIndices, 1, 0, 0, 0);

      VkBuffer VertexBufferId = VertexBuffer.GetBufferId();

      vkCmdBindVertexBuffers(TransparentCommandBufferId, 0, 1, &VertexBufferId, &VertexBufferOffset);

      VkBuffer IndexBufferId = IndexBuffer.GetBufferId();

      vkCmdBindIndexBuffer(TransparentCommandBufferId, IndexBufferId, IndexBufferOffset, VK_INDEX_TYPE_UINT32);

      vkCmdDrawIndexed(TransparentCommandBufferId, NumberOfTransparentIndices, 1, MaxNumberOfIndices - NumberOfTransparentIndices, 0, 0);
    }

    CommandBuffer.End();
  }
}

///**
// * \brief Update WVP function
// */
//VOID chunk_geometry::UpdateWVP( VOID )
//{
//  command_buffer CommandBuffer(CommandBufferId);
//
//  CommandBuffer.Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
//
//  CreateCommandBuffer();
//}

/**
 * \brief Add up border function
 * \param[in] BlockPos Block position
 * \param[in] TexCoords Texture coordinates
 * \param[in] Alpha Alpha part
 */
VOID chunk_geometry::AddUpBorder( const glm::ivec3 &BlockPos, const glm::vec2 *TexCoords, FLT Alpha )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  if (BlocksInfo[BlockInd].UpOffset != -1)
    throw std::runtime_error("border already exists");

  VERTEX WriteVertices[4] = {};
  UINT32 WriteIndices[6] = {};

  WriteVertices[0].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x, BlockPos.y + 1, ChunkOffsetZ + BlockPos.z);
  WriteVertices[1].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x + 1, BlockPos.y + 1, ChunkOffsetZ + BlockPos.z);
  WriteVertices[2].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x + 1, BlockPos.y + 1, ChunkOffsetZ + BlockPos.z + 1);
  WriteVertices[3].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x, BlockPos.y + 1, ChunkOffsetZ + BlockPos.z + 1);

  WriteVertices[0].TexCoord = TexCoords[0];
  WriteVertices[1].TexCoord = TexCoords[1];
  WriteVertices[2].TexCoord = TexCoords[2];
  WriteVertices[3].TexCoord = TexCoords[3];

  WriteVertices[0].Alpha = Alpha;
  WriteVertices[1].Alpha = Alpha;
  WriteVertices[2].Alpha = Alpha;
  WriteVertices[3].Alpha = Alpha;

  WriteIndices[0] = NumberOfVertices + 0;
  WriteIndices[1] = NumberOfVertices + 1;
  WriteIndices[2] = NumberOfVertices + 2;
  WriteIndices[3] = NumberOfVertices + 0;
  WriteIndices[4] = NumberOfVertices + 2;
  WriteIndices[5] = NumberOfVertices + 3;

  if (Alpha < 1 - FLT_EPSILON)
  {
    NumberOfTransparentVertices += 4;
    NumberOfTransparentIndices += 6;
    NumberOfTransparentBorders++;

    WriteIndices[0] = MaxNumberOfVertices - NumberOfTransparentVertices + 0;
    WriteIndices[1] = MaxNumberOfVertices - NumberOfTransparentVertices + 1;
    WriteIndices[2] = MaxNumberOfVertices - NumberOfTransparentVertices + 2;
    WriteIndices[3] = MaxNumberOfVertices - NumberOfTransparentVertices + 0;
    WriteIndices[4] = MaxNumberOfVertices - NumberOfTransparentVertices + 2;
    WriteIndices[5] = MaxNumberOfVertices - NumberOfTransparentVertices + 3;

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.VertexBuffer,
                                           VertexBufferOffset + sizeof(VERTEX) * (MaxNumberOfVertices - NumberOfTransparentVertices),
                                           sizeof(VERTEX) * 4,
                                           reinterpret_cast<BYTE *>(WriteVertices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.IndexBuffer,
                                           IndexBufferOffset + sizeof(UINT32) * (MaxNumberOfIndices - NumberOfTransparentIndices),
                                           sizeof(UINT32) * 6,
                                           reinterpret_cast<BYTE *>(WriteIndices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    INDEX_INFORMATION IndexInfo = {};

    IndexInfo.Offset = &BLOCK_INFORMATION::UpOffset;
    IndexInfo.BlockId = BlockInd;

    TransparentIndicesInfo.push_back(IndexInfo);

    BlocksInfo[BlockInd].UpOffset = MaxNumberOfBorders - NumberOfTransparentBorders;
  }
  else
  {
    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.VertexBuffer,
                                           VertexBufferOffset + sizeof(VERTEX) * NumberOfVertices, sizeof(VERTEX) * 4,
                                           reinterpret_cast<BYTE *>(WriteVertices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.IndexBuffer,
                                           IndexBufferOffset + sizeof(UINT32) * NumberOfIndices, sizeof(UINT32) * 6,
                                           reinterpret_cast<BYTE *>(WriteIndices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    //assert(NumberOfBorders * 4 == NumberOfVertices);
    //assert(NumberOfBorders * 6 == NumberOfIndices);
    //std::cout << "ADDUp (x6): " << NumberOfBorders * 6 << ":  (" << ChunkOffsetX + BlockPos.x << ", " << BlockPos.y << ", " << ChunkOffsetZ + BlockPos.z << ")\n" << std::endl;

    INDEX_INFORMATION IndexInfo = {};

    IndexInfo.Offset = &BLOCK_INFORMATION::UpOffset;
    IndexInfo.BlockId = BlockInd;

    IndicesInfo.push_back(IndexInfo);

    BlocksInfo[BlockInd].UpOffset = NumberOfBorders;

    NumberOfVertices += 4;
    NumberOfIndices += 6;
    NumberOfBorders++;
  }
}

/**
 * \brief Add down border function
 * \param[in] BlockPos Block position
 * \param[in] TexCoords Texture coordinates
 * \param[in] Alpha Alpha part
 */
VOID chunk_geometry::AddDownBorder( const glm::ivec3 &BlockPos, const glm::vec2 *TexCoords, FLT Alpha )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  if (BlocksInfo[BlockInd].DownOffset != -1)
    throw std::runtime_error("border already exists");

  VERTEX WriteVertices[4] = {};
  UINT32 WriteIndices[6] = {};

  WriteVertices[0].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x, BlockPos.y, ChunkOffsetZ + BlockPos.z);
  WriteVertices[1].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x + 1, BlockPos.y, ChunkOffsetZ + BlockPos.z);
  WriteVertices[2].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x + 1, BlockPos.y, ChunkOffsetZ + BlockPos.z + 1);
  WriteVertices[3].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x, BlockPos.y, ChunkOffsetZ + BlockPos.z + 1);

  WriteVertices[0].TexCoord = TexCoords[0];
  WriteVertices[1].TexCoord = TexCoords[1];
  WriteVertices[2].TexCoord = TexCoords[2];
  WriteVertices[3].TexCoord = TexCoords[3];

  WriteVertices[0].Alpha = Alpha;
  WriteVertices[1].Alpha = Alpha;
  WriteVertices[2].Alpha = Alpha;
  WriteVertices[3].Alpha = Alpha;

  WriteIndices[0] = NumberOfVertices + 0;
  WriteIndices[1] = NumberOfVertices + 1;
  WriteIndices[2] = NumberOfVertices + 2;
  WriteIndices[3] = NumberOfVertices + 0;
  WriteIndices[4] = NumberOfVertices + 2;
  WriteIndices[5] = NumberOfVertices + 3;

  if (Alpha < 1 - FLT_EPSILON)
  {
    NumberOfTransparentVertices += 4;
    NumberOfTransparentIndices += 6;
    NumberOfTransparentBorders++;

    WriteIndices[0] = MaxNumberOfVertices - NumberOfTransparentVertices + 0;
    WriteIndices[1] = MaxNumberOfVertices - NumberOfTransparentVertices + 1;
    WriteIndices[2] = MaxNumberOfVertices - NumberOfTransparentVertices + 2;
    WriteIndices[3] = MaxNumberOfVertices - NumberOfTransparentVertices + 0;
    WriteIndices[4] = MaxNumberOfVertices - NumberOfTransparentVertices + 2;
    WriteIndices[5] = MaxNumberOfVertices - NumberOfTransparentVertices + 3;

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.VertexBuffer,
                                           VertexBufferOffset + sizeof(VERTEX) * (MaxNumberOfVertices - NumberOfTransparentVertices),
                                           sizeof(VERTEX) * 4,
                                           reinterpret_cast<BYTE *>(WriteVertices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.IndexBuffer,
                                           IndexBufferOffset + sizeof(UINT32) * (MaxNumberOfIndices - NumberOfTransparentIndices),
                                           sizeof(UINT32) * 6,
                                           reinterpret_cast<BYTE *>(WriteIndices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    INDEX_INFORMATION IndexInfo = {};

    IndexInfo.Offset = &BLOCK_INFORMATION::DownOffset;
    IndexInfo.BlockId = BlockInd;

    TransparentIndicesInfo.push_back(IndexInfo);

    BlocksInfo[BlockInd].DownOffset = MaxNumberOfBorders - NumberOfTransparentBorders;
  }
  else
  {
    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.VertexBuffer,
                                           VertexBufferOffset + sizeof(VERTEX) * NumberOfVertices, sizeof(VERTEX) * 4,
                                           reinterpret_cast<BYTE *>(WriteVertices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.IndexBuffer,
                                           IndexBufferOffset + sizeof(UINT32) * NumberOfIndices, sizeof(UINT32) * 6,
                                           reinterpret_cast<BYTE *>(WriteIndices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    //assert(NumberOfBorders * 4 == NumberOfVertices);
    //assert(NumberOfBorders * 6 == NumberOfIndices);
    //std::cout << "ADDDown (x6): " << NumberOfBorders * 6 << ":  (" << ChunkOffsetX + BlockPos.x << ", " << BlockPos.y << ", " << ChunkOffsetZ + BlockPos.z << ")\n" << std::endl;

    INDEX_INFORMATION IndexInfo = {};

    IndexInfo.Offset = &BLOCK_INFORMATION::DownOffset;
    IndexInfo.BlockId = BlockInd;

    IndicesInfo.push_back(IndexInfo);

    BlocksInfo[BlockInd].DownOffset = NumberOfBorders;

    NumberOfVertices += 4;
    NumberOfIndices += 6;
    NumberOfBorders++;
  }
}

/**
 * \brief Add right border function
 * \param[in] BlockPos Block position
 * \param[in] TexCoords Texture coordinates
 * \param[in] Alpha Alpha part
 */
VOID chunk_geometry::AddRightBorder( const glm::ivec3 &BlockPos, const glm::vec2 *TexCoords, FLT Alpha )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  if (BlocksInfo[BlockInd].RightOffset != -1)
    throw std::runtime_error("border already exists");

  VERTEX WriteVertices[4] = {};
  UINT32 WriteIndices[6] = {};

  WriteVertices[0].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x + 1, BlockPos.y, ChunkOffsetZ + BlockPos.z);
  WriteVertices[1].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x + 1, BlockPos.y + 1, ChunkOffsetZ + BlockPos.z);
  WriteVertices[2].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x + 1, BlockPos.y + 1, ChunkOffsetZ + BlockPos.z + 1);
  WriteVertices[3].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x + 1, BlockPos.y, ChunkOffsetZ + BlockPos.z + 1);

  WriteVertices[0].TexCoord = TexCoords[0];
  WriteVertices[1].TexCoord = TexCoords[1];
  WriteVertices[2].TexCoord = TexCoords[2];
  WriteVertices[3].TexCoord = TexCoords[3];

  WriteVertices[0].Alpha = Alpha;
  WriteVertices[1].Alpha = Alpha;
  WriteVertices[2].Alpha = Alpha;
  WriteVertices[3].Alpha = Alpha;

  WriteIndices[0] = NumberOfVertices + 0;
  WriteIndices[1] = NumberOfVertices + 1;
  WriteIndices[2] = NumberOfVertices + 2;
  WriteIndices[3] = NumberOfVertices + 0;
  WriteIndices[4] = NumberOfVertices + 2;
  WriteIndices[5] = NumberOfVertices + 3;

  if (Alpha < 1 - FLT_EPSILON)
  {
    NumberOfTransparentVertices += 4;
    NumberOfTransparentIndices += 6;
    NumberOfTransparentBorders++;

    WriteIndices[0] = MaxNumberOfVertices - NumberOfTransparentVertices + 0;
    WriteIndices[1] = MaxNumberOfVertices - NumberOfTransparentVertices + 1;
    WriteIndices[2] = MaxNumberOfVertices - NumberOfTransparentVertices + 2;
    WriteIndices[3] = MaxNumberOfVertices - NumberOfTransparentVertices + 0;
    WriteIndices[4] = MaxNumberOfVertices - NumberOfTransparentVertices + 2;
    WriteIndices[5] = MaxNumberOfVertices - NumberOfTransparentVertices + 3;

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.VertexBuffer,
                                           VertexBufferOffset + sizeof(VERTEX) * (MaxNumberOfVertices - NumberOfTransparentVertices),
                                           sizeof(VERTEX) * 4,
                                           reinterpret_cast<BYTE *>(WriteVertices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.IndexBuffer,
                                           IndexBufferOffset + sizeof(UINT32) * (MaxNumberOfIndices - NumberOfTransparentIndices),
                                           sizeof(UINT32) * 6,
                                           reinterpret_cast<BYTE *>(WriteIndices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    INDEX_INFORMATION IndexInfo = {};

    IndexInfo.Offset = &BLOCK_INFORMATION::RightOffset;
    IndexInfo.BlockId = BlockInd;

    TransparentIndicesInfo.push_back(IndexInfo);

    BlocksInfo[BlockInd].RightOffset = MaxNumberOfBorders - NumberOfTransparentBorders;
  }
  else
  {
    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.VertexBuffer,
                                           VertexBufferOffset + sizeof(VERTEX) * NumberOfVertices, sizeof(VERTEX) * 4,
                                           reinterpret_cast<BYTE *>(WriteVertices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.IndexBuffer,
                                           IndexBufferOffset + sizeof(UINT32) * NumberOfIndices, sizeof(UINT32) * 6,
                                           reinterpret_cast<BYTE *>(WriteIndices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    //assert(NumberOfBorders * 4 == NumberOfVertices);
    //assert(NumberOfBorders * 6 == NumberOfIndices);
    //std::cout << "ADDRight (x6): " << NumberOfBorders * 6 << ":  (" << ChunkOffsetX + BlockPos.x << ", " << BlockPos.y << ", " << ChunkOffsetZ + BlockPos.z << ")\n" << std::endl;

    INDEX_INFORMATION IndexInfo = {};

    IndexInfo.Offset = &BLOCK_INFORMATION::RightOffset;
    IndexInfo.BlockId = BlockInd;

    IndicesInfo.push_back(IndexInfo);

    BlocksInfo[BlockInd].RightOffset = NumberOfBorders;

    NumberOfVertices += 4;
    NumberOfIndices += 6;
    NumberOfBorders++;
  }
}

/**
 * \brief Add left border function
 * \param[in] BlockPos Block position
 * \param[in] TexCoords Texture coordinates
 * \param[in] Alpha Alpha part
 */
VOID chunk_geometry::AddLeftBorder( const glm::ivec3 &BlockPos, const glm::vec2 *TexCoords, FLT Alpha )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  if (BlocksInfo[BlockInd].LeftOffset != -1)
    throw std::runtime_error("border already exists");

  VERTEX WriteVertices[4] = {};
  UINT32 WriteIndices[6] = {};

  WriteVertices[0].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x, BlockPos.y, ChunkOffsetZ + BlockPos.z);
  WriteVertices[1].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x, BlockPos.y + 1, ChunkOffsetZ + BlockPos.z);
  WriteVertices[2].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x, BlockPos.y + 1, ChunkOffsetZ + BlockPos.z + 1);
  WriteVertices[3].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x, BlockPos.y, ChunkOffsetZ + BlockPos.z + 1);

  WriteVertices[0].TexCoord = TexCoords[0];
  WriteVertices[1].TexCoord = TexCoords[1];
  WriteVertices[2].TexCoord = TexCoords[2];
  WriteVertices[3].TexCoord = TexCoords[3];

  WriteVertices[0].Alpha = Alpha;
  WriteVertices[1].Alpha = Alpha;
  WriteVertices[2].Alpha = Alpha;
  WriteVertices[3].Alpha = Alpha;

  WriteIndices[0] = NumberOfVertices + 0;
  WriteIndices[1] = NumberOfVertices + 1;
  WriteIndices[2] = NumberOfVertices + 2;
  WriteIndices[3] = NumberOfVertices + 0;
  WriteIndices[4] = NumberOfVertices + 2;
  WriteIndices[5] = NumberOfVertices + 3;

  if (Alpha < 1 - FLT_EPSILON)
  {
    NumberOfTransparentVertices += 4;
    NumberOfTransparentIndices += 6;
    NumberOfTransparentBorders++;

    WriteIndices[0] = MaxNumberOfVertices - NumberOfTransparentVertices + 0;
    WriteIndices[1] = MaxNumberOfVertices - NumberOfTransparentVertices + 1;
    WriteIndices[2] = MaxNumberOfVertices - NumberOfTransparentVertices + 2;
    WriteIndices[3] = MaxNumberOfVertices - NumberOfTransparentVertices + 0;
    WriteIndices[4] = MaxNumberOfVertices - NumberOfTransparentVertices + 2;
    WriteIndices[5] = MaxNumberOfVertices - NumberOfTransparentVertices + 3;

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.VertexBuffer,
                                           VertexBufferOffset + sizeof(VERTEX) * (MaxNumberOfVertices - NumberOfTransparentVertices),
                                           sizeof(VERTEX) * 4,
                                           reinterpret_cast<BYTE *>(WriteVertices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.IndexBuffer,
                                           IndexBufferOffset + sizeof(UINT32) * (MaxNumberOfIndices - NumberOfTransparentIndices),
                                           sizeof(UINT32) * 6,
                                           reinterpret_cast<BYTE *>(WriteIndices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    INDEX_INFORMATION IndexInfo = {};

    IndexInfo.Offset = &BLOCK_INFORMATION::LeftOffset;
    IndexInfo.BlockId = BlockInd;

    TransparentIndicesInfo.push_back(IndexInfo);

    BlocksInfo[BlockInd].LeftOffset = MaxNumberOfBorders - NumberOfTransparentBorders;
  }
  else
  {
    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.VertexBuffer,
                                           VertexBufferOffset + sizeof(VERTEX) * NumberOfVertices, sizeof(VERTEX) * 4,
                                           reinterpret_cast<BYTE *>(WriteVertices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.IndexBuffer,
                                           IndexBufferOffset + sizeof(UINT32) * NumberOfIndices, sizeof(UINT32) * 6,
                                           reinterpret_cast<BYTE *>(WriteIndices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    //assert(NumberOfBorders * 4 == NumberOfVertices);
    //assert(NumberOfBorders * 6 == NumberOfIndices);
    //std::cout << "ADDLeft (x6): " << NumberOfBorders * 6 << ":  (" << ChunkOffsetX + BlockPos.x << ", " << BlockPos.y << ", " << ChunkOffsetZ + BlockPos.z << ")\n" << std::endl;

    INDEX_INFORMATION IndexInfo = {};

    IndexInfo.Offset = &BLOCK_INFORMATION::LeftOffset;
    IndexInfo.BlockId = BlockInd;

    IndicesInfo.push_back(IndexInfo);

    BlocksInfo[BlockInd].LeftOffset = NumberOfBorders;

    NumberOfVertices += 4;
    NumberOfIndices += 6;
    NumberOfBorders++;
  }
}

/**
 * \brief Add front border function
 * \param[in] BlockPos Block position
 * \param[in] TexCoords Texture coordinates
 * \param[in] Alpha Alpha part
 */
VOID chunk_geometry::AddFrontBorder( const glm::ivec3 &BlockPos, const glm::vec2 *TexCoords, FLT Alpha )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  if (BlocksInfo[BlockInd].FrontOffset != -1)
    throw std::runtime_error("border already exists");

  VERTEX WriteVertices[4] = {};
  UINT32 WriteIndices[6] = {};

  WriteVertices[0].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x, BlockPos.y, ChunkOffsetZ + BlockPos.z + 1);
  WriteVertices[1].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x, BlockPos.y + 1, ChunkOffsetZ + BlockPos.z + 1);
  WriteVertices[2].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x + 1, BlockPos.y + 1, ChunkOffsetZ + BlockPos.z + 1);
  WriteVertices[3].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x + 1, BlockPos.y, ChunkOffsetZ + BlockPos.z + 1);

  WriteVertices[0].TexCoord = TexCoords[0];
  WriteVertices[1].TexCoord = TexCoords[1];
  WriteVertices[2].TexCoord = TexCoords[2];
  WriteVertices[3].TexCoord = TexCoords[3];

  WriteVertices[0].Alpha = Alpha;
  WriteVertices[1].Alpha = Alpha;
  WriteVertices[2].Alpha = Alpha;
  WriteVertices[3].Alpha = Alpha;

  WriteIndices[0] = NumberOfVertices + 0;
  WriteIndices[1] = NumberOfVertices + 1;
  WriteIndices[2] = NumberOfVertices + 2;
  WriteIndices[3] = NumberOfVertices + 0;
  WriteIndices[4] = NumberOfVertices + 2;
  WriteIndices[5] = NumberOfVertices + 3;

  if (Alpha < 1 - FLT_EPSILON)
  {
    NumberOfTransparentVertices += 4;
    NumberOfTransparentIndices += 6;
    NumberOfTransparentBorders++;

    WriteIndices[0] = MaxNumberOfVertices - NumberOfTransparentVertices + 0;
    WriteIndices[1] = MaxNumberOfVertices - NumberOfTransparentVertices + 1;
    WriteIndices[2] = MaxNumberOfVertices - NumberOfTransparentVertices + 2;
    WriteIndices[3] = MaxNumberOfVertices - NumberOfTransparentVertices + 0;
    WriteIndices[4] = MaxNumberOfVertices - NumberOfTransparentVertices + 2;
    WriteIndices[5] = MaxNumberOfVertices - NumberOfTransparentVertices + 3;

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.VertexBuffer,
                                           VertexBufferOffset + sizeof(VERTEX) * (MaxNumberOfVertices - NumberOfTransparentVertices),
                                           sizeof(VERTEX) * 4,
                                           reinterpret_cast<BYTE *>(WriteVertices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.IndexBuffer,
                                           IndexBufferOffset + sizeof(UINT32) * (MaxNumberOfIndices - NumberOfTransparentIndices),
                                           sizeof(UINT32) * 6,
                                           reinterpret_cast<BYTE *>(WriteIndices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    INDEX_INFORMATION IndexInfo = {};

    IndexInfo.Offset = &BLOCK_INFORMATION::FrontOffset;
    IndexInfo.BlockId = BlockInd;

    TransparentIndicesInfo.push_back(IndexInfo);

    BlocksInfo[BlockInd].FrontOffset = MaxNumberOfBorders - NumberOfTransparentBorders;
  }
  else
  {
    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.VertexBuffer,
                                           VertexBufferOffset + sizeof(VERTEX) * NumberOfVertices, sizeof(VERTEX) * 4,
                                           reinterpret_cast<BYTE *>(WriteVertices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.IndexBuffer,
                                           IndexBufferOffset + sizeof(UINT32) * NumberOfIndices, sizeof(UINT32) * 6,
                                           reinterpret_cast<BYTE *>(WriteIndices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    //assert(NumberOfBorders * 4 == NumberOfVertices);
    //assert(NumberOfBorders * 6 == NumberOfIndices);
    //std::cout << "ADDFront (x6): " << NumberOfBorders * 6 << ":  (" << ChunkOffsetX + BlockPos.x << ", " << BlockPos.y << ", " << ChunkOffsetZ + BlockPos.z << ")\n" << std::endl;

    INDEX_INFORMATION IndexInfo = {};

    IndexInfo.Offset = &BLOCK_INFORMATION::FrontOffset;
    IndexInfo.BlockId = BlockInd;

    IndicesInfo.push_back(IndexInfo);

    BlocksInfo[BlockInd].FrontOffset = NumberOfBorders;

    NumberOfVertices += 4;
    NumberOfIndices += 6;
    NumberOfBorders++;
  }
}

/**
 * \brief Add back border function
 * \param[in] BlockPos Block position
 * \param[in] TexCoords Texture coordinates
 * \param[in] Alpha Alpha part
 */
VOID chunk_geometry::AddBackBorder( const glm::ivec3 &BlockPos, const glm::vec2 *TexCoords, FLT Alpha )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  if (BlocksInfo[BlockInd].BackOffset != -1)
    throw std::runtime_error("border already exists");

  VERTEX WriteVertices[4] = {};
  UINT32 WriteIndices[6] = {};

  WriteVertices[0].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x, BlockPos.y, ChunkOffsetZ + BlockPos.z);
  WriteVertices[1].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x, BlockPos.y + 1, ChunkOffsetZ + BlockPos.z);
  WriteVertices[2].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x + 1, BlockPos.y + 1, ChunkOffsetZ + BlockPos.z);
  WriteVertices[3].Position =
    glm::vec3(ChunkOffsetX + BlockPos.x + 1, BlockPos.y, ChunkOffsetZ + BlockPos.z);

  WriteVertices[0].TexCoord = TexCoords[0];
  WriteVertices[1].TexCoord = TexCoords[1];
  WriteVertices[2].TexCoord = TexCoords[2];
  WriteVertices[3].TexCoord = TexCoords[3];

  WriteVertices[0].Alpha = Alpha;
  WriteVertices[1].Alpha = Alpha;
  WriteVertices[2].Alpha = Alpha;
  WriteVertices[3].Alpha = Alpha;

  WriteIndices[0] = NumberOfVertices + 0;
  WriteIndices[1] = NumberOfVertices + 1;
  WriteIndices[2] = NumberOfVertices + 2;
  WriteIndices[3] = NumberOfVertices + 0;
  WriteIndices[4] = NumberOfVertices + 2;
  WriteIndices[5] = NumberOfVertices + 3;

  if (Alpha < 1 - FLT_EPSILON)
  {
    NumberOfTransparentVertices += 4;
    NumberOfTransparentIndices += 6;
    NumberOfTransparentBorders++;

    WriteIndices[0] = MaxNumberOfVertices - NumberOfTransparentVertices + 0;
    WriteIndices[1] = MaxNumberOfVertices - NumberOfTransparentVertices + 1;
    WriteIndices[2] = MaxNumberOfVertices - NumberOfTransparentVertices + 2;
    WriteIndices[3] = MaxNumberOfVertices - NumberOfTransparentVertices + 0;
    WriteIndices[4] = MaxNumberOfVertices - NumberOfTransparentVertices + 2;
    WriteIndices[5] = MaxNumberOfVertices - NumberOfTransparentVertices + 3;

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.VertexBuffer,
                                           VertexBufferOffset + sizeof(VERTEX) * (MaxNumberOfVertices - NumberOfTransparentVertices),
                                           sizeof(VERTEX) * 4,
                                           reinterpret_cast<BYTE *>(WriteVertices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.IndexBuffer,
                                           IndexBufferOffset + sizeof(UINT32) * (MaxNumberOfIndices - NumberOfTransparentIndices),
                                           sizeof(UINT32) * 6,
                                           reinterpret_cast<BYTE *>(WriteIndices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    INDEX_INFORMATION IndexInfo = {};

    IndexInfo.Offset = &BLOCK_INFORMATION::BackOffset;
    IndexInfo.BlockId = BlockInd;

    TransparentIndicesInfo.push_back(IndexInfo);

    BlocksInfo[BlockInd].BackOffset = MaxNumberOfBorders - NumberOfTransparentBorders;
  }
  else
  {
    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.VertexBuffer,
                                           VertexBufferOffset + sizeof(VERTEX) * NumberOfVertices, sizeof(VERTEX) * 4,
                                           reinterpret_cast<BYTE *>(WriteVertices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    Render.MemoryManager.SmallUpdateBuffer(Render.MemoryManager.IndexBuffer,
                                           IndexBufferOffset + sizeof(UINT32) * NumberOfIndices, sizeof(UINT32) * 6,
                                           reinterpret_cast<BYTE *>(WriteIndices),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           (VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                                            VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    //assert(NumberOfBorders * 4 == NumberOfVertices);
    //assert(NumberOfBorders * 6 == NumberOfIndices);
    //std::cout << "ADDBack (x6): " << NumberOfBorders * 6 << ":  (" << ChunkOffsetX + BlockPos.x << ", " << BlockPos.y << ", " << ChunkOffsetZ + BlockPos.z << ")\n" << std::endl;

    INDEX_INFORMATION IndexInfo = {};

    IndexInfo.Offset = &BLOCK_INFORMATION::BackOffset;
    IndexInfo.BlockId = BlockInd;

    IndicesInfo.push_back(IndexInfo);

    BlocksInfo[BlockInd].BackOffset = NumberOfBorders;

    NumberOfVertices += 4;
    NumberOfIndices += 6;
    NumberOfBorders++;
  }
}

/**
 * \brief Update up block border
 * \param[in] BlockPos Block position
 * \param[in] Blocks Blocks array
 */
VOID chunk_geometry::UpdateUpSide( const glm::ivec3 &BlockPos, const BLOCK *Blocks )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  std::lock_guard<std::mutex> Lock(MetaInfoMutex);

  if (BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].Alpha < FLT_EPSILON ||
      (BlockPos.y < ChunkSizeY - 1 &&
       BLOCK_TYPE::Table[Blocks[BlockPos.z * ChunkSizeY * ChunkSizeX +
         (BlockPos.y + 1) * ChunkSizeX + BlockPos.x].BlockTypeId].Alpha > 1 - FLT_EPSILON))
  {
    RemoveUpBorder(BlockPos);
  }
  else if (BlocksInfo[BlockInd].UpOffset == -1)
  {
    glm::vec2 *TexCoords = nullptr;

    if (Blocks[BlockInd].Direction.y != 0)
    {
      if (Blocks[BlockInd].Direction.y == 1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordFront;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordBack;
    }
    else if (Blocks[BlockInd].Right.y != 0)
    {
      if (Blocks[BlockInd].Right.y == 1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordRight;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordLeft;
    }
    else if (Blocks[BlockInd].Up.y != 0)
    {
      if (Blocks[BlockInd].Up.y == 1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordUp;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordDown;
    }

    if (TexCoords == nullptr)
      throw std::runtime_error("Texture coordinates not defined");

    AddUpBorder(BlockPos, TexCoords, BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].Alpha);
  }
}

/**
 * \brief Update left block border
 * \param[in] BlockPos Block position
 * \param[in] Blocks Blocks array
 */
VOID chunk_geometry::UpdateLeftSide( const glm::ivec3 &BlockPos, const BLOCK *Blocks )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  std::lock_guard<std::mutex> Lock(MetaInfoMutex);

  if (BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].Alpha < FLT_EPSILON ||
    (BlockPos.x > 0 &&
    BLOCK_TYPE::Table[Blocks[BlockPos.z * ChunkSizeY * ChunkSizeX +
    BlockPos.y * ChunkSizeX + BlockPos.x - 1].BlockTypeId].Alpha > 1 - FLT_EPSILON))
  {
    RemoveLeftBorder(BlockPos);
  }
  else if (BlocksInfo[BlockInd].LeftOffset == -1)
  {
    glm::vec2 *TexCoords = nullptr;

    if (Blocks[BlockInd].Direction.x != 0)
    {
      if (Blocks[BlockInd].Direction.x == -1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordFront;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordBack;
    }
    else if (Blocks[BlockInd].Right.x != 0)
    {
      if (Blocks[BlockInd].Right.x == -1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordRight;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordLeft;
    }
    else if (Blocks[BlockInd].Up.x != 0)
    {
      if (Blocks[BlockInd].Up.x == -1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordUp;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordDown;
    }

    if (TexCoords == nullptr)
      throw std::runtime_error("Texture coordinates not defined");

    AddLeftBorder(BlockPos, TexCoords, BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].Alpha);
  }
}

/**
 * \brief Update down block border
 * \param[in] BlockPos Block position
 * \param[in] Blocks Blocks array
 */
VOID chunk_geometry::UpdateDownSide( const glm::ivec3 &BlockPos, const BLOCK *Blocks )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  std::lock_guard<std::mutex> Lock(MetaInfoMutex);

  if (BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].Alpha < FLT_EPSILON ||
      (BlockPos.y > 0 &&
       BLOCK_TYPE::Table[Blocks[BlockPos.z * ChunkSizeY * ChunkSizeX +
         (BlockPos.y - 1) * ChunkSizeX + BlockPos.x].BlockTypeId].Alpha > 1 - FLT_EPSILON))
  {
    RemoveDownBorder(BlockPos);
  }
  else if (BlocksInfo[BlockInd].DownOffset == -1)
  {
    glm::vec2 *TexCoords = nullptr;

    if (Blocks[BlockInd].Direction.y != 0)
    {
      if (Blocks[BlockInd].Direction.y == -1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordFront;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordBack;
    }
    else if (Blocks[BlockInd].Right.y != 0)
    {
      if (Blocks[BlockInd].Right.y == -1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordRight;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordLeft;
    }
    else if (Blocks[BlockInd].Up.y != 0)
    {
      if (Blocks[BlockInd].Up.y == -1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordUp;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordDown;
    }

    if (TexCoords == nullptr)
      throw std::runtime_error("Texture coordinates not defined");

    AddDownBorder(BlockPos, TexCoords, BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].Alpha);
  }
}

/**
 * \brief Update right block border
 * \param[in] BlockPos Block position
 * \param[in] Blocks Blocks array
 */
VOID chunk_geometry::UpdateRightSide( const glm::ivec3 &BlockPos, const BLOCK *Blocks )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  std::lock_guard<std::mutex> Lock(MetaInfoMutex);

  if (BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].Alpha < FLT_EPSILON ||
      (BlockPos.x < ChunkSizeX - 1 &&
       BLOCK_TYPE::Table[Blocks[BlockPos.z * ChunkSizeY * ChunkSizeX +
                                BlockPos.y * ChunkSizeX + BlockPos.x + 1].BlockTypeId].Alpha > 1 - FLT_EPSILON))
  {
    RemoveRightBorder(BlockPos);
  }
  else if (BlocksInfo[BlockInd].RightOffset == -1)
  {
    glm::vec2 *TexCoords = nullptr;

    if (Blocks[BlockInd].Direction.x != 0)
    {
      if (Blocks[BlockInd].Direction.x == 1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordFront;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordBack;
    }
    else if (Blocks[BlockInd].Right.x != 0)
    {
      if (Blocks[BlockInd].Right.x == 1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordRight;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordLeft;
    }
    else if (Blocks[BlockInd].Up.x != 0)
    {
      if (Blocks[BlockInd].Up.x == 1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordUp;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordDown;
    }

    if (TexCoords == nullptr)
      throw std::runtime_error("Texture coordinates not defined");

    AddRightBorder(BlockPos, TexCoords, BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].Alpha);
  }
}

/**
 * \brief Update front block border
 * \param[in] BlockPos Block position
 * \param[in] Blocks Blocks array
 */
VOID chunk_geometry::UpdateFrontSide( const glm::ivec3 &BlockPos, const BLOCK *Blocks )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  std::lock_guard<std::mutex> Lock(MetaInfoMutex);

  if (BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].Alpha < FLT_EPSILON ||
      (BlockPos.z < ChunkSizeZ - 1 &&
       BLOCK_TYPE::Table[Blocks[(BlockPos.z + 1) * ChunkSizeY * ChunkSizeX +
         BlockPos.y * ChunkSizeX + BlockPos.x].BlockTypeId].Alpha > 1 - FLT_EPSILON))
  {
    RemoveFrontBorder(BlockPos);
  }
  else if (BlocksInfo[BlockInd].FrontOffset == -1)
  {
    glm::vec2 *TexCoords = nullptr;

    if (Blocks[BlockInd].Direction.z != 0)
    {
      if (Blocks[BlockInd].Direction.z == 1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordFront;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordBack;
    }
    else if (Blocks[BlockInd].Right.z != 0)
    {
      if (Blocks[BlockInd].Right.z == 1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordRight;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordLeft;
    }
    else if (Blocks[BlockInd].Up.z != 0)
    {
      if (Blocks[BlockInd].Up.z == 1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordUp;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordDown;
    }

    if (TexCoords == nullptr)
      throw std::runtime_error("Texture coordinates not defined");

    AddFrontBorder(BlockPos, TexCoords, BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].Alpha);
  }
}

/**
 * \brief Update back block border
 * \param[in] BlockPos Block position
 * \param[in] Blocks Blocks array
 */
VOID chunk_geometry::UpdateBackSide( const glm::ivec3 &BlockPos, const BLOCK *Blocks )
{
  UINT32 BlockInd = BlockPos.z * ChunkSizeY * ChunkSizeX +
                    BlockPos.y * ChunkSizeX + BlockPos.x;

  std::lock_guard<std::mutex> Lock(MetaInfoMutex);

  if (BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].Alpha < FLT_EPSILON ||
      (BlockPos.z > 0 &&
       BLOCK_TYPE::Table[Blocks[(BlockPos.z - 1) * ChunkSizeY * ChunkSizeX +
                                BlockPos.y * ChunkSizeX + BlockPos.x].BlockTypeId].Alpha > 1 - FLT_EPSILON))
  {
    RemoveBackBorder(BlockPos);
  }
  else if (BlocksInfo[BlockInd].BackOffset == -1)
  {
    glm::vec2 *TexCoords = nullptr;

    if (Blocks[BlockInd].Direction.z != 0)
    {
      if (Blocks[BlockInd].Direction.z == -1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordFront;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordBack;
    }
    else if (Blocks[BlockInd].Right.z != 0)
    {
      if (Blocks[BlockInd].Right.z == -1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordRight;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordLeft;
    }
    else if (Blocks[BlockInd].Up.z != 0)
    {
      if (Blocks[BlockInd].Up.z == -1)
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordUp;
      else
        TexCoords = BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].TexCoordDown;
    }

    if (TexCoords == nullptr)
      throw std::runtime_error("Texture coordinates not defined");

    AddBackBorder(BlockPos, TexCoords, BLOCK_TYPE::Table[Blocks[BlockInd].BlockTypeId].Alpha);
  }
}

/**
 * \brief Get command buffer for draw function
 * \return Secondary command buffer
 */
VkCommandBuffer chunk_geometry::GetCommandBuffer( VOID )
{
  return CommandBufferId;
}

/**
 * \brief Get secondary command buffer for drawing opaque objects
 * \return Command buffer
 */
VkCommandBuffer chunk_geometry::GetTransparentCommandBuffer( VOID )
{
  return TransparentCommandBufferId;
}

/**
 * \brief Chunk geometry destructor
 */
chunk_geometry::~chunk_geometry( VOID )
{
  Render.MemoryManager.FreeChunk(MemoryChunkId);
  Render.DeleteDrawElement(this);

  {
    std::lock_guard<std::mutex> Lock(Render.Synchronization.RenderMutex);

    {
      command_buffer CommandBuffer(CommandBufferId);

      CommandBuffer.Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    }

    {
      command_buffer CommandBuffer(TransparentCommandBufferId);

      CommandBuffer.Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    }
  }
  
  Render.ReturnSecondaryCommandBuffer(CommandBufferId);
  Render.ReturnSecondaryCommandBuffer(TransparentCommandBufferId);
}

/**
 * \brief Update block function
 * \param[in] BlockPos Block position
 * \param[in] Blocks Blocks array
 */
VOID chunk_geometry::UpdateBlock( const glm::ivec3 &BlockPos, const BLOCK *Blocks )
{
  UpdateLeftSide(BlockPos, Blocks);
  UpdateRightSide(BlockPos, Blocks);
  UpdateUpSide(BlockPos, Blocks);
  UpdateDownSide(BlockPos, Blocks);
  UpdateFrontSide(BlockPos, Blocks);
  UpdateBackSide(BlockPos, Blocks);
}
