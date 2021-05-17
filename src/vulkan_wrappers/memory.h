#ifndef __memory_h_
#define __memory_h_

#include "ext/volk/volk.h"

#include "def.h"

/**
 * \brief Vulkan memory class
 */
class memory
{
public:
  /**
   * \brief Default constructor.
   */
  memory( VOID ) = default;

  /**
   * \brief Memory constructor
   * \param[in] Device Device identifier
   * \param[in] Size Memory size
   * \param[in] MemoryTypeIndex Memory type index
   */
  memory( const VkDevice Device, const VkDeviceSize Size, const UINT32 MemoryTypeIndex );

  /**
   * \brief Memory destructor
   */
  ~memory( VOID );

  /**
   * \brief Get vulkan memory identifier
   * \return Vulkan memory identifier
   */
  VkDeviceMemory GetMemoryId( VOID ) const noexcept;

  /**
   * \brief Map memory function
   * \param[in] Offset Memory offset
   * \param[in] Size Memory size
   * \param[in, out] Data Pointer to pointer to allocation memory
   * \param[in] Flags Memory map flags
   */
  VOID MapMemory( const VkDeviceSize Offset, const VkDeviceSize Size,
                  VOID **Data, const VkMemoryMapFlags Flags = 0 ) const;

  /**
   * \brief Unmap memory function
   */
  VOID UnmapMemory( VOID ) const;

  /**
   * \brief Flush all mapped range function
   * \param[in] Offset Currently mapped memory offset.
   */
  VOID FlushAllRange( VkDeviceSize Offset ) const;

  /**
   * \brief Flush memory ranges
   * \param[in] NumberOfRanges Number of memory ranges
   * \param[in] Ranges Memory ranges
   */
  VOID FlushMemoryRanges( const UINT32 NumberOfRanges,
                          const VkMappedMemoryRange *Ranges ) const;

  /**
   * \brief Invalidate all mapped range function
   */
  VOID InvalidateAllRange( VkDeviceSize Offset ) const;

  /**
   * \brief Invalidate memory ranges
   * \param[in] NumberOfRanges Number of memory ranges
   * \param[in] Ranges Memory ranges
   */
  VOID InvalidateMemoryRanges( const UINT32 NumberOfRanges,
                               const VkMappedMemoryRange *Ranges ) const;

  /**
   * \brief Move function
   * \param[in] Mem Vulkan memory
   * \return Reference to this
   */
  memory & operator=( memory &&Mem ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] Mem Vulkan memory
   */
  memory( memory &&Mem ) noexcept;

  /** Memory type index */
  UINT32 MemoryType;

private:
  /**
   * \brief Removed copy function
   * \param[in] Mem Vulkan memory
   * \return Reference to this
   */
  memory & operator=( const memory &Mem ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] Mem Vulkan memory
   */
  memory( const memory &Mem ) = delete;

  /** Buffer identifier */
  VkDeviceMemory MemoryId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /* __memory_h_ */
