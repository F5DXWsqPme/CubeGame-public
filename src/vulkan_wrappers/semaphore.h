#ifndef __semaphore_h_
#define __semaphore_h_

#include "ext/volk/volk.h"

#include "def.h"

/**
 * \brief Vulkan semaphore class
 */
class semaphore
{
public:
  /**
   * \brief Default constructor.
   */
  semaphore( VOID ) = default;

  /**
   * \brief Semaphore constructor
   * \param[in] Device Device identifier
   */
  semaphore( VkDevice Device );

  /**
   * \brief Semaphore destructor
   */
  ~semaphore( VOID );

  /**
   * \brief Get semaphore identifier function
   * \return Semaphore identifier
   */
  VkSemaphore GetSemaphoreId( VOID ) const noexcept;

  /**
   * \brief Move function
   * \param[in] Semaphore Vulkan semaphore
   * \return Reference to this
   */
  semaphore & operator=( semaphore &&Semaphore ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] Semaphore Vulkan semaphore
   * \return Reference to this
   */
  semaphore( semaphore &&Semaphore ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] Semaphore Vulkan semaphore
   * \return Reference to this
   */
  semaphore & operator=( const semaphore &Semaphore ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] Semaphore Vulkan buffer
   */
  semaphore( const semaphore &Semaphore ) = delete;

  /** Semaphore identifier */
  VkSemaphore SemaphoreId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /* __semaphore_h_ */
