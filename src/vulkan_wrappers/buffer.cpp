#include <cassert>

#include "buffer.h"
#include "vulkan_validation.h"

/**
 * \brief Buffer constructor
 * \param[in] Device Device identifier
 * \param[in] Size Buffer size
 * \param[in] UsageFlags Buffer usage flags
 * \param[in] Create flags Buffer create flags
 * \param[in] SharingMode Buffer sharing mode
 * \param[in] NumberOfFamilyIndices Number of queue family indices
 * \param[in] FamilyIndices Pointer to queue family indices
 */
buffer::buffer( const VkDevice Device, const VkDeviceSize Size,
                const VkBufferUsageFlags UsageFlags,
                const VkBufferCreateFlags CreateFlags,
                const VkSharingMode SharingMode,
                const UINT32 NumberOfFamilyIndices,
                const UINT32 *FamilyIndices ) :
  DeviceId(Device)
{
  VkBufferCreateInfo CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  CreateInfo.pNext = nullptr;
  CreateInfo.flags = CreateFlags;
  CreateInfo.size = Size;
  CreateInfo.usage = UsageFlags;
  CreateInfo.sharingMode = SharingMode;
  CreateInfo.queueFamilyIndexCount = NumberOfFamilyIndices;
  CreateInfo.pQueueFamilyIndices = FamilyIndices;

  vulkan_validation::Check(
    vkCreateBuffer(DeviceId, &CreateInfo, nullptr, &BufferId),
    "creating buffer failed");
}

/**
 * \brief Buffer destructor
 */
buffer::~buffer( VOID )
{
  if (BufferId != VK_NULL_HANDLE)
    vkDestroyBuffer(DeviceId, BufferId, nullptr);
}

/**
 * \brief Move function
 * \param[in] Buf Vulkan buffer
 * \return Reference to this
 */
buffer & buffer::operator=( buffer &&Buf ) noexcept
{
  // It is ok. E. g. resize buffer.
  //assert(BufferId == VK_NULL_HANDLE);

  std::swap(BufferId, Buf.BufferId);
  std::swap(DeviceId, Buf.DeviceId);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] Buf Vulkan buffer
 * \return Reference to this
 */
buffer::buffer( buffer &&Buf ) noexcept
{
  // It is ok. E. g. resize buffer.
  //assert(BufferId == VK_NULL_HANDLE);

  std::swap(BufferId, Buf.BufferId);
  std::swap(DeviceId, Buf.DeviceId);
}

/**
 * \brief Bind memory to buffer function
 * \param[in] Mem Memory for bind
 * \param[in] Offset Memory offset
 */
VOID buffer::BindMemory( const memory &Mem, const VkDeviceSize Offset ) const
{
  vulkan_validation::Check(
    vkBindBufferMemory(DeviceId, BufferId, Mem.GetMemoryId(), Offset),
    "binding memory failed");
}

/**
 * \brief Get memory requirements function
 * \return Memory requirements
 */
VkMemoryRequirements buffer::GetMemoryRequirements( VOID ) const
{
  VkMemoryRequirements MemoryRequirements;

  vkGetBufferMemoryRequirements(DeviceId, BufferId, &MemoryRequirements);

  return MemoryRequirements;
}

/**
 * \brief Get buffer identifier function
 * \return Buffer identifier
 */
VkBuffer buffer::GetBufferId( VOID ) const noexcept
{
  return BufferId;
}
