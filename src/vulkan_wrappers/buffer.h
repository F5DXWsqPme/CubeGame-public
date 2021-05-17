#ifndef __buffer_h_
#define __buffer_h_

#include "memory.h"

/**
 * \brief Vulkan buffer class
 */
class buffer
{
public:
  /**
   * \brief Default constructor.
   */
  buffer( VOID ) = default;

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
  buffer( const VkDevice Device, const VkDeviceSize Size,
          const VkBufferUsageFlags UsageFlags,
          const VkBufferCreateFlags CreateFlags = 0,
          const VkSharingMode SharingMode = VK_SHARING_MODE_EXCLUSIVE,
          const UINT32 NumberOfFamilyIndices = 0,
          const UINT32 *FamilyIndices = nullptr );

  /**
   * \brief Buffer destructor
   */
  ~buffer( VOID );

  /**
   * \brief Bind memory to buffer function
   * \param[in] Mem Memory for bind
   * \param[in] Offset Memory offset
   */
  VOID BindMemory( const memory &Mem, const VkDeviceSize Offset ) const;

  /**
   * \brief Get memory requirements function
   * \return Memory requirements
   */
  VkMemoryRequirements GetMemoryRequirements( VOID ) const;

  /**
   * \brief Get buffer identifier function
   * \return Buffer identifier
   */
  VkBuffer GetBufferId( VOID ) const noexcept;

  /**
   * \brief Move function
   * \param[in] Buf Vulkan buffer
   * \return Reference to this
   */
  buffer & operator=( buffer &&Buf ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] Buf Vulkan buffer
   * \return Reference to this
   */
  buffer( buffer &&Buf ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] Buf Vulkan buffer
   * \return Reference to this
   */
  buffer & operator=( const buffer &Buf ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] Buf Vulkan buffer
   * \return Reference to this
   */
  buffer( const buffer &Buf ) = delete;

  /** Buffer identifier */
  VkBuffer BufferId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /* __buffer_h_ */
