#ifndef __memory_manager_h_
#define __memory_manager_h_

#include <set>

#include "def.h"
#include "vulkan_wrappers/vulkan_application.h"
#include "vulkan_wrappers/memory.h"
#include "vulkan_wrappers/buffer.h"
#include "vulkan_wrappers/command_pool.h"
#include "vulkan_wrappers/queue.h"
#include "vulkan_wrappers/fence.h"
#include "render_synchronization.h"

/**
 * \brief Memory manger class
 */
class memory_manager
{
public:
  /**
   * \brief Memory manager constructor
   * \param[in] MaxNumberOfChunks Maximal number of chunks
   * \param[in] ChunkVertexSize Maximal chunk vertex buffer size in bytes
   * \param[in] ChunkIndexSize Maximal chunk index buffer size in bytes
   * \param[in] MaxTransferSize Maximal transfer operation size
   * \param[in] UniformSize Uniform buffer size
   * \param[in, out] Synchronization Synchronization object
   */
  memory_manager( vulkan_application &VkApp, UINT32 MaxNumberOfChunks,
                  UINT64 ChunkVertexSize, UINT64 ChunkIndexSize,
                  UINT64 MaxTransferSize, UINT64 UniformSize,
                  render_synchronization &Synchronization );

  /**
   * \brief Find memory type for device buffer
   * \param[in] MemoryRequirements Buffer memory requirements
   * \return Memory type
   */
  std::optional<UINT32> FindDeviceMemoryType( const VkMemoryRequirements &MemoryRequirements ) const;

  /**
   * \brief Small update buffer function (Size must be less than or equal 65536 bytes)
   * \param[in] Buffer Buffer for update
   * \param[in] Offset Offset in buffer
   * \param[in] Size Update size
   * \param[in] Data New data
   * \param[in] SrcAccess Source access flags
   * \param[in] DstAccess Destination access flags
   * \param[in] SrcStage Source stage
   * \param[in] DstStage Destination stage
   */
  VOID SmallUpdateBuffer( const buffer &Buffer, UINT64 Offset, UINT64 Size, const BYTE *Data,
                          VkAccessFlags SrcAccess, VkAccessFlags DstAccess,
                          VkFlags SrcStage, VkFlags DstStage ) const;

  /**
   * \brief Copy buffer region
   * \param[in] SrcBuffer Buffer for copy (source)
   * \param[in] DstBuffer Buffer for copy (destination)
   * \param[in] OffsetSrc Source offset in buffer
   * \param[in] OffsetDst Destination offset in buffer
   * \param[in] Size Region size
   * \param[in] SrcSrcAccess Source access flags (source buffer)
   * \param[in] SrcDstAccess Destination access flags (source buffer)
   * \param[in] DstSrcAccess Source access flags (destination buffer)
   * \param[in] DstDstAccess Destination access flags (destination buffer)
   * \param[in] SrcSrcStage Source stage (source buffer)
   * \param[in] SrcDstStage Destination stage (source buffer)
   * \param[in] DstSrcStage Source stage (destination buffer)
   * \param[in] DstDstStage Destination stage (destination buffer)
   * \param[in] SrcSrcQueueFamilyIndex Source family index (source buffer)
   * \param[in] SrcDstQueueFamilyIndex Destination family index (source buffer)
   * \param[in] DstSrcQueueFamilyIndex Source family index (destination buffer)
   * \param[in] DstDstQueueFamilyIndex Destination family index (destination buffer)
   */
  VOID CopyBufferRegion( const buffer &SrcBuffer, const buffer &DstBuffer, UINT64 OffsetSrc, UINT64 OffsetDst, UINT64 Size,
                         VkAccessFlags SrcSrcAccess, VkAccessFlags SrcDstAccess,
                         VkAccessFlags DstSrcAccess, VkAccessFlags DstDstAccess,
                         VkFlags SrcSrcStage, VkFlags SrcDstStage,
                         VkFlags DstSrcStage, VkFlags DstDstStage,
                         UINT32 SrcSrcQueueFamilyIndex, UINT32 SrcDstQueueFamilyIndex,
                         UINT32 DstSrcQueueFamilyIndex, UINT32 DstDstQueueFamilyIndex) const;

  /**
   * \brief Get memory for writing data function
   * \param[in] Size Size of memory
   * \param[in] Offset Offset in buffer
   * \param[in] Memory Buffer memory for writing
   * \param[in] NeedCopy Memory don't visible from CPU flag
   * \return Pointer to memory
   */
  BYTE * GetMemoryForWriting( UINT64 Size, UINT64 Offset, const memory &Memory, BOOL NeedCopy ) const;

  /**
   * \brief Get memory for writing data function
   * \param[in] Size Size of memory
   * \param[in] Offset Offset in buffer
   * \param[in] Memory Buffer memory for writing
   * \param[in] Buffer Buffer for writing
   * \param[in] NeedCopy Memory don't visible from CPU flag
   * \param[in] SrcAccess Source access flags
   * \param[in] DstAccess Destination access flags
   */
  VOID PushMemory( UINT64 Size, UINT64 Offset, const memory &Memory, const buffer &Buffer, BOOL NeedCopy,
                   VkFlags SrcAccess, VkFlags DstAccess ) const;

  /**
   * \brief Allocate chunk
   * \return Allocate memory index
   */
  UINT32 AllocateChunk( VOID );

  /**
   * \brief Free chunk
   * \return Free memory index
   */
  VOID FreeChunk( UINT32 ChunkId );

  /** Vertex memory don't visible from CPU flag */
  BOOL NeedCopyVertex = FALSE;

  /** Index memory don't visible from CPU flag */
  BOOL NeedCopyIndex = FALSE;

  /** Uniform memory don't visible from CPU flag */
  BOOL NeedCopyUniform = FALSE;

  /** Memory for vertex buffer */
  memory VertexMemory;

  /** Memory for index buffer */
  memory IndexMemory;

  /** Memory for transfer data from CPU to GPU */
  memory TransferMemory;

  /** Memory for uniform buffer */
  memory UniformMemory;

  /** Vertex buffer */
  buffer VertexBuffer;

  /** Index buffer */
  buffer IndexBuffer;

  /** Buffer for transfer data from CPU to GPU */
  buffer TransferBuffer;

  /** Uniform buffer */
  buffer UniformBuffer;

private:
  /** Free chunks set */
  std::set<UINT32> FreeChunks;

  /** Reference to vulkan application */
  vulkan_application &VkApp;

  /** Command pool for push */
  command_pool PushCommandPool;

  /** Command pool for small update */
  //command_pool SmallUpdateCommandPool;

  /** Command pool for copy */
  command_pool CopyCommandPool;

  /** Transfer command buffer for push */
  VkCommandBuffer PushCommandBuffer;

  /** Transfer command buffer for small update */
  //VkCommandBuffer SmallUpdateCommandBuffer;

  /** Transfer command buffer for copy */
  VkCommandBuffer CopyCommandBuffer;

  /** Transfer queue */
  queue TransferQueue;

  /** Fence for waiting in push */
  fence PushFence;

  /** Fence for waiting in copy */
  fence CopyFence;

  /** Fence for waiting in small update */
  fence SmallUpdateFence;

  /** Synchronization object */
  render_synchronization &Synchronization;
};

#endif /* __memory_manager_h_ */