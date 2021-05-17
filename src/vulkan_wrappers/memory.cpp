#include <cassert>

#include "memory.h"
#include "vulkan_validation.h"

/**
 * \brief Buffer constructor
 * \param[in] Device Device identifier
 * \param[in] Size Memory size
 * \param[in] MemoryTypeIndex Memory type index
 */
memory::memory( const VkDevice Device, const VkDeviceSize Size, const UINT32 MemoryTypeIndex ) :
  DeviceId(Device), MemoryType(MemoryTypeIndex)
{
  VkMemoryAllocateInfo AllocateInfo = {};

  AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  AllocateInfo.pNext = nullptr;
  AllocateInfo.allocationSize = Size;
  AllocateInfo.memoryTypeIndex = MemoryTypeIndex;

  vulkan_validation::Check(
    vkAllocateMemory(DeviceId, &AllocateInfo, nullptr, &MemoryId),
    "memory allocation failed");
}

/**
 * \brief Buffer destructor
 */
memory::~memory( VOID )
{
  if (MemoryId != VK_NULL_HANDLE)
    vkFreeMemory(DeviceId, MemoryId, nullptr);
}

/**
 * \brief Move function
 * \param[in] Mem Vulkan memory
 * \return Reference to this
 */
memory & memory::operator=( memory &&Mem ) noexcept
{
  // It is ok. E. g. resize memory.
  //assert(MemoryId == VK_NULL_HANDLE);

  std::swap(MemoryId, Mem.MemoryId);
  std::swap(DeviceId, Mem.DeviceId);
  std::swap(MemoryType, Mem.MemoryType);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] Mem Vulkan memory
 * \return Reference to this
 */
memory::memory( memory &&Mem ) noexcept
{
  // It is ok. E. g. resize memory.
  //assert(MemoryId == VK_NULL_HANDLE);

  std::swap(MemoryId, Mem.MemoryId);
  std::swap(DeviceId, Mem.DeviceId);
  std::swap(MemoryType, Mem.MemoryType);
}

/**
 * \brief Get vulkan memory identifier
 * \return Vulkan memory identifier
 */
VkDeviceMemory memory::GetMemoryId( VOID ) const noexcept
{
  return MemoryId;
}

/**
 * \brief Map memory function
 * \param[in] Offset Memory offset
 * \param[in] Size Memory size
 * \param[in, out] Data Pointer to pointer to allocation memory
 * \param[in] Flags Memory map flags
 */
VOID memory::MapMemory( const VkDeviceSize Offset, const VkDeviceSize Size,
                        VOID **Data, const VkMemoryMapFlags Flags ) const
{
  vulkan_validation::Check(
    vkMapMemory(DeviceId, MemoryId, Offset, Size, Flags, Data),
    "map memory failed");
}

/**
 * \brief Unmap memory function
 */
VOID memory::UnmapMemory( VOID ) const
{
  vkUnmapMemory(DeviceId, MemoryId);
}

/**
 * \brief Flush all mapped range function
 * \param[in] Offset Currently mapped memory offset.
 */
VOID memory::FlushAllRange( VkDeviceSize Offset ) const
{
  VkMappedMemoryRange Range = {};

  Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  Range.pNext = nullptr;
  Range.memory = MemoryId;
  Range.offset = Offset;
  Range.size = VK_WHOLE_SIZE;

  FlushMemoryRanges(1, &Range);
}

/**
 * \brief Flush memory ranges
 * \param[in] NumberOfRanges Number of memory ranges
 * \param[in] Ranges Memory ranges
 */
VOID memory::FlushMemoryRanges( const UINT32 NumberOfRanges,
                                const VkMappedMemoryRange *Ranges ) const
{
  vulkan_validation::Check(
    vkFlushMappedMemoryRanges(DeviceId, NumberOfRanges, Ranges),
    "flush memory ranges failed");
}

/**
 * \brief Invalidate all mapped range function
 * \param[in] Offset Currently mapped memory offset.
 */
VOID memory::InvalidateAllRange( VkDeviceSize Offset ) const
{
  VkMappedMemoryRange Range = {};

  Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  Range.pNext = nullptr;
  Range.memory = MemoryId;
  Range.offset = Offset;
  Range.size = VK_WHOLE_SIZE;

  InvalidateMemoryRanges(1, &Range);
}

/**
 * \brief Invalidate memory ranges
 * \param[in] NumberOfRanges Number of memory ranges
 * \param[in] Ranges Memory ranges
 */
VOID memory::InvalidateMemoryRanges( const UINT32 NumberOfRanges,
                                     const VkMappedMemoryRange *Ranges ) const
{
  vulkan_validation::Check(
    vkInvalidateMappedMemoryRanges(DeviceId, NumberOfRanges, Ranges),
    "memory ranges invalidation failed");
}
