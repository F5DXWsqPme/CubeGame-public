#ifndef __fence_h_
#define __fence_h_

#include "ext/volk/volk.h"

#include "def.h"

/**
 * \brief Fence class
 */
class fence
{
public:
  /**
   * \brief Default constructor.
   */
  fence( VOID ) = default;

  /**
   * \brief Fence constructor
   * \param[in] Device Device identifier
   * \parram[in] IsSignaled Fence signaled flag.
   */
  fence( VkDevice Device, BOOL IsSignaled = FALSE );

  /**
   * \brief Fence destructor
   */
  ~fence( VOID );

  /**
   * \brief Get fence identifier function
   * \return Fence identifier
   */
  VkFence GetFenceId( VOID ) const noexcept;

  /**
   * \brief Move function
   * \param[in] Fence Vulkan fence
   * \return Reference to this
   */
  fence & operator=( fence &&Fence ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] Fence Vulkan fence
   */
  fence( fence &&Fence ) noexcept;

  /**
   * \brief Wait for fence signaled
   * \param[in] Timeout Timeout for exit from waiting.
   * return Result of waiting
   */
  VkResult Wait( UINT64 Timeout = std::numeric_limits<UINT64>::max() ) const;

  /**
   * \brief Reset fence function
   */
  VOID Reset( VOID ) const;

private:
  /**
   * \brief Removed copy function
   * \param[in] Fence Vulkan fence
   * \return Reference to this
   */
  fence & operator=( const fence &Fence ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] Fence Vulkan fence
   */
  fence( const fence &Fence ) = delete;

  /** Fence identifier */
  VkFence FenceId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /* __fence_h_ */
