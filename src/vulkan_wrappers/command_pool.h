#ifndef __command_pool_h_
#define __command_pool_h_

#include "ext/volk/volk.h"

#include "def.h"

/**
 * \brief Vulkan command pool class
 */
class command_pool
{
public:
  /**
   * \brief Default constructor.
   */
  command_pool( VOID ) = default;

  /**
   * \brief Command pool constructor
   * \param[in] Device Device identifier
   */
  command_pool( const VkDevice Device, const UINT32 QueueFamilyIndex,
                const VkCommandPoolCreateFlags Flags );

  /**
   * \brief Command pool destructor
   */
  ~command_pool( VOID );

  /**
   * \brief Allocate command buffers function
   * \param[in, out] CommandBuffers Pointer to command buffers
   * \param[in] NumberOfBuffers Number of command buffers
   * \param[in] Level Command buffers level
   */
  VOID AllocateCommandBuffers( VkCommandBuffer *CommandBuffers, const UINT32 NumberOfBuffers = 1,
                               const VkCommandBufferLevel Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY ) const;

  /**
   * \brief Free command buffers function
   * \param[in] CommandBuffers Pointer to command buffers
   * \param[in] NumberOfBuffers Number of command buffers
   */
  VOID FreeCommandBuffers( const VkCommandBuffer *CommandBuffers,
                           const UINT32 NumberOfBuffers = 1 ) const;

  /**
   * \brief Reset command pool function
   * \param[in] Flags Reset flags
   */
  VOID Reset( const VkCommandPoolResetFlags Flags = 0 ) const;

  /**
   * \brief Move function
   * \param[in] Pool Vulkan command pool
   * \return Reference to this
   */
  command_pool & operator=( command_pool &&Pool ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] Pool Vulkan command pool
   * \return Reference to this
   */
  command_pool( command_pool &&Pool ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] Pool Vulkan command pool
   * \return Reference to this
   */
  command_pool & operator=( const command_pool &Pool ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] Pool Vulkan command pool
   * \return Reference to this
   */
  command_pool( const command_pool &Pool ) = delete;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;

  /** Command pool identifier */
  VkCommandPool CommandPoolId = VK_NULL_HANDLE;
};

#endif /* __command_pool_h_ */
