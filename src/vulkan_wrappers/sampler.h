#ifndef __sampler_h_
#define __sampler_h_

#include "memory.h"

/**
 * \brief Sampler class
 */
class sampler
{
public:
  /**
   * \brief Default constructor.
   */
  sampler( VOID ) = default;

  /**
   * \brief Sampler constructor
   * \param[in] Device Device identifier
   * \param[in] CreateInfo Sampler create information
   */
  sampler( VkDevice Device, const VkSamplerCreateInfo &CreateInfo );

  /**
   * \brief Sampler destructor
   */
  ~sampler( VOID );

  /**
   * \brief Get sampler identifier function
   * \return Sampler identifier
   */
  VkSampler GetSamplerId( VOID ) const noexcept;

  /**
   * \brief Move function
   * \param[in] Sampler Vulkan sampler
   * \return Reference to this
   */
  sampler & operator=( sampler &&Sampler ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] Sampler Vulkan buffer
   */
  sampler( sampler &&Sampler ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] Sampler Vulkan sampler
   * \return Reference to this
   */
  sampler & operator=( const sampler &Sampler ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] Sampler Vulkan sampler
   */
  sampler( const sampler &Sampler ) = delete;

  /** Sampler identifier */
  VkSampler SamplerId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /** __sampler_h_ */
