#ifndef __pipeline_cache_h_
#define __pipeline_cache_h_

#include "ext/volk/volk.h"

#include "def.h"

/**
 * \brief Pipeline cache class
 */
class pipeline_cache
{
public:
  /**
   * \brief Default constructor.
   */
  pipeline_cache( VOID ) = default;

  /**
   * \brief Empty pipeline cache constructor
   * \param[in] Device Device identifier
   */
  pipeline_cache( const VkDevice Device );

  /**
   * \brief Pipeline cache destructor
   */
  ~pipeline_cache( VOID );

  /**
   * \brief Get pipeline cache identifier
   * \return Pipeline cache identifier
   */
  VkPipelineCache GetPipelineCacheId( VOID ) const noexcept;

  /**
   * \brief Move function
   * \param[in] Cache Vulkan cache module
   * \return Reference to this
   */
  pipeline_cache & operator=( pipeline_cache &&Cache ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] Cache Vulkan cache module
   */
  pipeline_cache( pipeline_cache &&Cache ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] Cache Vulkan cache module
   * \return Reference to this
   */
  pipeline_cache & operator=( const pipeline_cache &Cache ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] Cache Vulkan cache module
   */
  pipeline_cache( const pipeline_cache &Cache ) = delete;

  /** Pipeline cache identifier */
  VkPipelineCache PipelineCacheId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /* __pipeline_cache_h_ */
