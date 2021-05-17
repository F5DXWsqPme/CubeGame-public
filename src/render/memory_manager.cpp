#include "memory_manager.h"
#include "vulkan_wrappers/command_buffer.h"

/**
 * \brief Memory manager constructor
 * \param[in] MaxNumberOfChunks Maximal number of chunks
 * \param[in] ChunkVertexSize Maximal chunk vertex buffer size in bytes
 * \param[in] ChunkIndexSize Maximal chunk index buffer size in bytes
 * \param[in] MaxTransferSize Maximal transfer operation size
 * \param[in] UniformSize Uniform buffer size
 * \param[in, out] Synchronization Synchronization object
 */
memory_manager::memory_manager( vulkan_application &VkApp, UINT32 MaxNumberOfChunks, UINT64 ChunkVertexSize,
                                UINT64 ChunkIndexSize, UINT64 MaxTransferSize, UINT64 UniformSize,
                                render_synchronization &Synchronization ) : VkApp(VkApp), Synchronization(Synchronization)
{
  UniformBuffer = buffer(VkApp.GetDeviceId(), UniformSize,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                         0, VK_SHARING_MODE_EXCLUSIVE, 0, nullptr);

  VkMemoryRequirements UniformMemoryRequirements = UniformBuffer.GetMemoryRequirements();
  std::optional<UINT32> UniformMemoryTypeIndex = FindDeviceMemoryType(UniformMemoryRequirements);

  if (!UniformMemoryTypeIndex)
    throw std::runtime_error("memory type for uniform buffer not found");

  VkMemoryType UniformMemoryType = VkApp.DeviceMemoryProperties.memoryTypes[*UniformMemoryTypeIndex];

  if ((UniformMemoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
    NeedCopyUniform = TRUE;

  UniformMemory = memory(VkApp.GetDeviceId(), UniformMemoryRequirements.size, *UniformMemoryTypeIndex);

  UniformBuffer.BindMemory(UniformMemory, 0);

  UINT64 VertexSize = ChunkVertexSize * MaxNumberOfChunks;

  VertexBuffer = buffer(VkApp.GetDeviceId(), VertexSize,
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    0, VK_SHARING_MODE_EXCLUSIVE, 0, nullptr);

  VkMemoryRequirements VertexMemoryRequirements = VertexBuffer.GetMemoryRequirements();
  std::optional<UINT32> VertexMemoryTypeIndex = FindDeviceMemoryType(VertexMemoryRequirements);

  if (!VertexMemoryTypeIndex)
    throw std::runtime_error("memory type for vertex buffer not found");

  VkMemoryType VertexMemoryType = VkApp.DeviceMemoryProperties.memoryTypes[*VertexMemoryTypeIndex];

  if ((VertexMemoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
    NeedCopyVertex = TRUE;

  VertexMemory = memory(VkApp.GetDeviceId(), VertexMemoryRequirements.size, *VertexMemoryTypeIndex);

  VertexBuffer.BindMemory(VertexMemory, 0);

  UINT64 IndexSize = ChunkVertexSize * MaxNumberOfChunks;

  IndexBuffer = buffer(VkApp.GetDeviceId(), IndexSize,
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       0, VK_SHARING_MODE_EXCLUSIVE, 0, nullptr);

  VkMemoryRequirements IndexMemoryRequirements = IndexBuffer.GetMemoryRequirements();
  std::optional<UINT32> IndexMemoryTypeIndex = FindDeviceMemoryType(IndexMemoryRequirements);

  if (!IndexMemoryTypeIndex)
    throw std::runtime_error("memory type for index buffer not found");

  VkMemoryType IndexMemoryType = VkApp.DeviceMemoryProperties.memoryTypes[*IndexMemoryTypeIndex];

  if ((IndexMemoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
    NeedCopyIndex = TRUE;

  IndexMemory = memory(VkApp.GetDeviceId(), IndexMemoryRequirements.size, *IndexMemoryTypeIndex);

  IndexBuffer.BindMemory(IndexMemory, 0);

  if (NeedCopyVertex || NeedCopyIndex || NeedCopyUniform)
  {
    TransferBuffer = buffer(VkApp.GetDeviceId(), MaxTransferSize,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            0, VK_SHARING_MODE_EXCLUSIVE, 0, nullptr);

    VkMemoryRequirements TransferMemoryRequirements = TransferBuffer.GetMemoryRequirements();
    std::optional<UINT32> TransferMemoryTypeIndex = FindDeviceMemoryType(TransferMemoryRequirements);

    if (!TransferMemoryTypeIndex)
      throw std::runtime_error("memory type for transfer buffer not found");

    TransferMemory = memory(VkApp.GetDeviceId(), TransferMemoryRequirements.size, *TransferMemoryTypeIndex);

    TransferBuffer.BindMemory(TransferMemory, 0);
  }

  if (!VkApp.TransferQueueFamilyIndex)
    throw std::runtime_error("transfer queue not found");

  PushCommandPool = command_pool(VkApp.GetDeviceId(), *VkApp.TransferQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  //SmallUpdateCommandPool = command_pool(VkApp.GetDeviceId(), *VkApp.TransferQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  CopyCommandPool = command_pool(VkApp.GetDeviceId(), *VkApp.TransferQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  PushCommandPool.AllocateCommandBuffers(&PushCommandBuffer, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  //SmallUpdateCommandPool.AllocateCommandBuffers(&SmallUpdateCommandBuffer, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  CopyCommandPool.AllocateCommandBuffers(&CopyCommandBuffer, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  TransferQueue = queue(VkApp.GetDeviceId(), *VkApp.TransferQueueFamilyIndex, 0);

  SmallUpdateFence = fence(VkApp.GetDeviceId(), FALSE);
  PushFence = fence(VkApp.GetDeviceId(), FALSE);
  CopyFence = fence(VkApp.GetDeviceId(), FALSE);

  for (UINT32 i = 0; i < MaxNumberOfChunks; i++)
    FreeChunks.insert(i);
}

/**
 * \brief Allocate chunk
 * \return Allocate memory index
 */
UINT32 memory_manager::AllocateChunk( VOID )
{
  if (FreeChunks.size() > 0)
  {
    UINT32 Res = *FreeChunks.begin();

    FreeChunks.erase(FreeChunks.begin());

    return Res;
  }
  else
    throw std::runtime_error("free chunks not found");
}

/**
 * \brief Free chunk
 * \return Free memory index
 */
VOID memory_manager::FreeChunk( UINT32 ChunkId )
{
  FreeChunks.insert(ChunkId);
}

/**
 * \brief Find memory type for device buffer
 * \param[in] MemoryRequirements Buffer memory requirements
 * \return Memory type
 */
std::optional<UINT32> memory_manager::FindDeviceMemoryType( const VkMemoryRequirements &MemoryRequirements ) const
{
  std::optional<UINT32> DeviceMemoryType =
    VkApp.FindMemoryTypeWithFlags(MemoryRequirements,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  if (!DeviceMemoryType)
  {
    DeviceMemoryType =
      VkApp.FindMemoryTypeWithFlags(MemoryRequirements,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (!DeviceMemoryType)
    {
      DeviceMemoryType =
        VkApp.FindMemoryTypeWithFlags(MemoryRequirements,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

      if (!DeviceMemoryType)
      {
        DeviceMemoryType =
          VkApp.FindMemoryTypeWithFlags(MemoryRequirements,
                                        0);
      }
    }
  }

  return DeviceMemoryType;
}

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
VOID memory_manager::SmallUpdateBuffer( const buffer &Buffer, UINT64 Offset, UINT64 Size, const BYTE *Data,
                                        VkAccessFlags SrcAccess, VkAccessFlags DstAccess,
                                        VkFlags SrcStage, VkFlags DstStage ) const
{
  command_buffer CommandBuffer(/*SmallUpdateCommandBuffer*/CopyCommandBuffer);

  CommandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  {
    VkBufferMemoryBarrier Barrier = {};

    Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    Barrier.pNext = nullptr;
    Barrier.srcAccessMask = SrcAccess;
    Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    Barrier.buffer = Buffer.GetBufferId();
    Barrier.size = Size;
    Barrier.offset = Offset;

    if (VkApp.TransferQueueFamilyIndex != VkApp.GraphicsQueueFamilyIndex)
    {
      Barrier.srcQueueFamilyIndex = *VkApp.GraphicsQueueFamilyIndex;
      Barrier.dstQueueFamilyIndex = *VkApp.TransferQueueFamilyIndex;
    }
    else
    {
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }

    vkCmdPipelineBarrier(/*SmallUpdateCommandBuffer*/CopyCommandBuffer, SrcStage,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                         1, &Barrier, 0, nullptr);
  }

  vkCmdUpdateBuffer(/*SmallUpdateCommandBuffer*/CopyCommandBuffer, Buffer.GetBufferId(), Offset, Size, Data);

  {
    VkBufferMemoryBarrier Barrier = {};

    Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    Barrier.pNext = nullptr;
    Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    Barrier.dstAccessMask = DstAccess;
    Barrier.buffer = Buffer.GetBufferId();
    Barrier.size = Size;
    Barrier.offset = Offset;

    if (VkApp.TransferQueueFamilyIndex != VkApp.GraphicsQueueFamilyIndex)
    {
      Barrier.srcQueueFamilyIndex = *VkApp.TransferQueueFamilyIndex;
      Barrier.dstQueueFamilyIndex = *VkApp.GraphicsQueueFamilyIndex;
    }
    else
    {
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }

    vkCmdPipelineBarrier(/*SmallUpdateCommandBuffer*/CopyCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         DstStage, 0, 0, nullptr,
                         1, &Barrier, 0, nullptr);
  }

  CommandBuffer.End();

  VkSubmitInfo SubmitInfo = {};

  SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  SubmitInfo.pNext = nullptr;
  SubmitInfo.waitSemaphoreCount = 0;
  SubmitInfo.pWaitSemaphores = nullptr;
  SubmitInfo.pWaitDstStageMask = nullptr;
  SubmitInfo.commandBufferCount = 1;
  SubmitInfo.pCommandBuffers = &/*SmallUpdateCommandBuffer*/CopyCommandBuffer;
  SubmitInfo.signalSemaphoreCount = 0;
  SubmitInfo.pSignalSemaphores = nullptr;

  if (VkApp.GraphicsQueueFamilyIndex == VkApp.TransferQueueFamilyIndex)
  {
    std::lock_guard<std::mutex> Lock(Synchronization.RenderMutex);

    TransferQueue.Submit(&SubmitInfo, 1, SmallUpdateFence.GetFenceId());
  }
  else
    TransferQueue.Submit(&SubmitInfo, 1, SmallUpdateFence.GetFenceId());

  SmallUpdateFence.Wait();

  CommandBuffer.Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  SmallUpdateFence.Reset();
}

/**
 * \brief Get memory for writing data function
 * \param[in] Size Size of memory
 * \param[in] Offset Offset in buffer
 * \param[in] Memory Buffer memory for writing
 * \param[in] NeedCopy Memory don't visible from CPU flag
 * \return Pointer to memory
 */
BYTE * memory_manager::GetMemoryForWriting( UINT64 Size, UINT64 Offset, const memory &Memory, BOOL NeedCopy ) const
{
  BYTE *Data;

  if (NeedCopy)
    TransferMemory.MapMemory(0, Size, reinterpret_cast<VOID **>(&Data));
  else
    Memory.MapMemory(Offset, Size, reinterpret_cast<VOID **>(&Data));

  return Data;
}

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
VOID memory_manager::CopyBufferRegion( const buffer &SrcBuffer, const buffer &DstBuffer, UINT64 OffsetSrc, UINT64 OffsetDst, UINT64 Size,
                                       VkAccessFlags SrcSrcAccess, VkAccessFlags SrcDstAccess,
                                       VkAccessFlags DstSrcAccess, VkAccessFlags DstDstAccess,
                                       VkFlags SrcSrcStage, VkFlags SrcDstStage,
                                       VkFlags DstSrcStage, VkFlags DstDstStage,
                                       UINT32 SrcSrcQueueFamilyIndex, UINT32 SrcDstQueueFamilyIndex,
                                       UINT32 DstSrcQueueFamilyIndex, UINT32 DstDstQueueFamilyIndex) const
{
  command_buffer CommandBuffer(CopyCommandBuffer);

  CommandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  {
    VkBufferMemoryBarrier Barrier = {};

    Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    Barrier.pNext = nullptr;
    Barrier.srcAccessMask = SrcSrcAccess;
    Barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    Barrier.buffer = SrcBuffer.GetBufferId();
    Barrier.size = Size;
    Barrier.offset = OffsetSrc;

    if (SrcSrcQueueFamilyIndex != *VkApp.TransferQueueFamilyIndex)
    {
      Barrier.srcQueueFamilyIndex = SrcSrcQueueFamilyIndex;
      Barrier.dstQueueFamilyIndex = *VkApp.TransferQueueFamilyIndex;
    }
    else
    {
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }

    vkCmdPipelineBarrier(CopyCommandBuffer, SrcSrcStage, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 1, &Barrier, 0, nullptr);

    if (DstSrcQueueFamilyIndex != *VkApp.TransferQueueFamilyIndex)
    {
      Barrier.srcQueueFamilyIndex = DstSrcQueueFamilyIndex;
      Barrier.dstQueueFamilyIndex = *VkApp.TransferQueueFamilyIndex;
    }
    else
    {
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }

    Barrier.buffer = DstBuffer.GetBufferId();
    Barrier.srcAccessMask = DstSrcAccess;
    Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    Barrier.offset = OffsetDst;

    vkCmdPipelineBarrier(CopyCommandBuffer, DstSrcStage, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 1, &Barrier, 0, nullptr);
  }

  VkBufferCopy Region = {};

  Region.size = Size;
  Region.srcOffset = OffsetSrc;
  Region.dstOffset = OffsetDst;

  vkCmdCopyBuffer(CopyCommandBuffer, SrcBuffer.GetBufferId(), DstBuffer.GetBufferId(), 1, &Region);

  {
    VkBufferMemoryBarrier Barrier = {};

    Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    Barrier.pNext = nullptr;
    Barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    Barrier.dstAccessMask = SrcDstAccess;
    Barrier.buffer = SrcBuffer.GetBufferId();
    Barrier.size = Size;
    Barrier.offset = OffsetSrc;

    if (SrcDstQueueFamilyIndex != *VkApp.TransferQueueFamilyIndex)
    {
      Barrier.srcQueueFamilyIndex = *VkApp.TransferQueueFamilyIndex;
      Barrier.dstQueueFamilyIndex = SrcDstQueueFamilyIndex;
    }
    else
    {
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }

    vkCmdPipelineBarrier(CopyCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, SrcDstStage,
                         0, 0, nullptr, 1, &Barrier, 0, nullptr);

    if (DstDstQueueFamilyIndex != *VkApp.TransferQueueFamilyIndex)
    {
      Barrier.srcQueueFamilyIndex = *VkApp.TransferQueueFamilyIndex;
      Barrier.dstQueueFamilyIndex = DstDstQueueFamilyIndex;
    }
    else
    {
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }

    Barrier.dstAccessMask = DstDstAccess;
    Barrier.buffer = DstBuffer.GetBufferId();
    Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    Barrier.offset = OffsetDst;

    vkCmdPipelineBarrier(CopyCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, DstDstStage,
                         0, 0, nullptr, 1, &Barrier, 0, nullptr);
  }

  CommandBuffer.End();

  VkSubmitInfo SubmitInfo = {};

  SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  SubmitInfo.pNext = nullptr;
  SubmitInfo.waitSemaphoreCount = 0;
  SubmitInfo.pWaitSemaphores = nullptr;
  SubmitInfo.pWaitDstStageMask = nullptr;
  SubmitInfo.commandBufferCount = 1;
  SubmitInfo.pCommandBuffers = &CopyCommandBuffer;
  SubmitInfo.signalSemaphoreCount = 0;
  SubmitInfo.pSignalSemaphores = nullptr;

  if (VkApp.GraphicsQueueFamilyIndex == VkApp.TransferQueueFamilyIndex)
  {
    std::lock_guard<std::mutex> Lock(Synchronization.RenderMutex);

    TransferQueue.Submit(&SubmitInfo, 1, CopyFence.GetFenceId());
  }
  else
    TransferQueue.Submit(&SubmitInfo, 1, CopyFence.GetFenceId());

  CopyFence.Wait();

  CommandBuffer.Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  CopyFence.Reset();
}

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
VOID memory_manager::PushMemory( UINT64 Size, UINT64 Offset, const memory &Memory, const buffer &Buffer, BOOL NeedCopy,
                                 VkAccessFlags SrcAccess, VkAccessFlags DstAccess ) const
{
  if (NeedCopy)
  {
    if ((VkApp.DeviceMemoryProperties.memoryTypes[TransferMemory.MemoryType].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
    {
      TransferMemory.FlushAllRange(0);
    }

    TransferMemory.UnmapMemory();
  }
  else
  {
    if ((VkApp.DeviceMemoryProperties.memoryTypes[Memory.MemoryType].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
    {
      Memory.FlushAllRange(Offset);
    }

    Memory.UnmapMemory();
  }

  command_buffer CommandBuffer(PushCommandBuffer);

  CommandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  if (NeedCopy)
  {
    {
      VkBufferMemoryBarrier Barrier = {};

      Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      Barrier.pNext = nullptr;
      Barrier.srcAccessMask = SrcAccess;
      Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      Barrier.buffer = Buffer.GetBufferId();
      Barrier.size = Size;
      Barrier.offset = Offset;

      if (VkApp.TransferQueueFamilyIndex != VkApp.GraphicsQueueFamilyIndex)
      {
        Barrier.srcQueueFamilyIndex = *VkApp.GraphicsQueueFamilyIndex;
        Barrier.dstQueueFamilyIndex = *VkApp.TransferQueueFamilyIndex;
      }
      else
      {
        Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      }

      vkCmdPipelineBarrier(PushCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           0, 0, nullptr, 1, &Barrier, 0, nullptr);
    }

    {
      VkBufferMemoryBarrier Barrier = {};

      Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      Barrier.pNext = nullptr;
      Barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
      Barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      Barrier.buffer = TransferBuffer.GetBufferId();
      Barrier.size = Size;
      Barrier.offset = 0;
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

      vkCmdPipelineBarrier(PushCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           0, 0, nullptr, 1, &Barrier, 0, nullptr);
    }

    VkBufferCopy Region = {};

    Region.size = Size;
    Region.srcOffset = 0;
    Region.dstOffset = Offset;

    vkCmdCopyBuffer(PushCommandBuffer, TransferBuffer.GetBufferId(), Buffer.GetBufferId(), 1, &Region);

    {
      VkBufferMemoryBarrier Barrier = {};

      Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      Barrier.pNext = nullptr;
      Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      Barrier.dstAccessMask = DstAccess;
      Barrier.buffer = Buffer.GetBufferId();
      Barrier.size = Size;
      Barrier.offset = Offset;

      if (VkApp.TransferQueueFamilyIndex != VkApp.GraphicsQueueFamilyIndex)
      {
        Barrier.srcQueueFamilyIndex = *VkApp.TransferQueueFamilyIndex;
        Barrier.dstQueueFamilyIndex = *VkApp.GraphicsQueueFamilyIndex;
      }
      else
      {
        Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      }

      vkCmdPipelineBarrier(PushCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                           0, nullptr, 1, &Barrier, 0, nullptr);
    }
  }
  else
  {
    {
      VkBufferMemoryBarrier Barrier = {};

      Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      Barrier.pNext = nullptr;
      Barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
      Barrier.dstAccessMask = DstAccess;
      Barrier.buffer = Buffer.GetBufferId();
      Barrier.size = Size;
      Barrier.offset = Offset;
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

      vkCmdPipelineBarrier(PushCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           0, 0, nullptr, 1, &Barrier, 0, nullptr);
    }
  }

  CommandBuffer.End();

  VkSubmitInfo SubmitInfo = {};

  SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  SubmitInfo.pNext = nullptr;
  SubmitInfo.waitSemaphoreCount = 0;
  SubmitInfo.pWaitSemaphores = nullptr;
  SubmitInfo.pWaitDstStageMask = nullptr;
  SubmitInfo.commandBufferCount = 1;
  SubmitInfo.pCommandBuffers = &PushCommandBuffer;
  SubmitInfo.signalSemaphoreCount = 0;
  SubmitInfo.pSignalSemaphores = nullptr;

  if (VkApp.GraphicsQueueFamilyIndex == VkApp.TransferQueueFamilyIndex)
  {
    std::lock_guard<std::mutex> Lock(Synchronization.RenderMutex);

    TransferQueue.Submit(&SubmitInfo, 1, PushFence.GetFenceId());
  }
  else
    TransferQueue.Submit(&SubmitInfo, 1, PushFence.GetFenceId());

  PushFence.Wait();

  CommandBuffer.Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  PushFence.Reset();
}
