#ifndef __compute_pipeline_h_
#define __compute_pipeline_h_

#include <string_view>

#include "ext/volk/volk.h"
#include "def.h"
#include "shader_module.h"
#include "pipeline_cache.h"

/**
 * \brief Vulkan compute pipeline class
 */
class compute_pipeline
{
public:
  /**
   * \brief Default constructor.
   */
  compute_pipeline( VOID ) = default;

  /**
   * \brief Compute pipeline constructor
   * \param[in] Device Device identifier
   * \param[in] Layout Pipeline layout
   * \param[in] Shader Shader module
   * \param[in] FuncName Name of shader function
   * \param[in] Cache Pipeline cache
   */
  compute_pipeline( const VkDevice Device, const VkPipelineLayout Layout,
                    const shader_module &Shader,
                    const std::string_view &FuncName,
                    const pipeline_cache &Cache );

  /**
   * \brief Get pipeline identifier
   * \return Compute pipeline identifier
   */
  VkPipeline GetPipelineId( VOID ) const noexcept;

  /**
   * \brief Compute pipeline destructor
   */
  ~compute_pipeline( VOID );

  /**
   * \brief Move function
   * \param[in] Pipeline Vulkan compute pipeline
   * \return Reference to this
   */
  compute_pipeline & operator=( compute_pipeline &&Pipeline ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] Pipeline Vulkan compute pipeline
   * \return Reference to this
   */
  compute_pipeline( compute_pipeline &&Pipeline ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] Pipeline Vulkan compute pipeline
   * \return Reference to this
   */
  compute_pipeline & operator=( const compute_pipeline &Pipeline ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] Pipeline Vulkan compute pipeline
   * \return Reference to this
   */
  compute_pipeline( const compute_pipeline &Pipeline ) = delete;

  /** Compute pipeline identifier */
  VkPipeline PipelineId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /* __compute_pipeline_h_ */
