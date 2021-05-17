#ifndef __pipeline_layout_h_
#define __pipeline_layout_h_

#include "ext/volk/volk.h"

#include "def.h"

/**
 * \brief Pipeline layout class
 */
class pipeline_layout
{
public:
  /**
   * \brief Default constructor.
   */
  pipeline_layout( VOID ) = default;

  /**
   * \brief Pipeline layout constructor
   * \param[in] Device Device identifier
   * \param[in] NumberOfSetLayouts Number of descriptor set layouts
   * \param[in] SetLayouts Descriptor set layouts
   * \param[in] NumberOfPushConstantRanges Number of push constant ranges
   * \param[in] PushConstantRanges Push constant ranges
   */
  pipeline_layout( const VkDevice Device, const UINT32 NumberOfSetLayouts,
    const VkDescriptorSetLayout *SetLayouts,
    const UINT32 NumberOfPushConstantRanges = 0,
    const VkPushConstantRange *PushConstantRanges = nullptr );

  /**
   * \brief Pipeline layout destructor
   */
  ~pipeline_layout( VOID );

  /**
   * \brief Get pipeline layout identifier
   * \return Pipeline layout identifier
   */
  VkPipelineLayout GetPipelineLayoutId( VOID ) const noexcept;

  /**
   * \brief Move function
   * \param[in] Layout Vulkan pipeline layout
   * \return Reference to this
   */
  pipeline_layout & operator=( pipeline_layout &&Layout ) noexcept;

  /**
   * \brief Move function
   * \param[in] Layout Vulkan pipeline layout
   */
  pipeline_layout( pipeline_layout &&Layout ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] Layout Vulkan pipeline layout
   * \return Reference to this
   */
  pipeline_layout & operator=( const pipeline_layout &Layout ) = delete;

  /**
   * \brief Removed copy function
   * \param[in] Layout Vulkan pipeline layout
   */
  pipeline_layout( const pipeline_layout &Layout ) = delete;

  /** Pipeline layout identifier */
  VkPipelineLayout PipelineLayoutId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /* __pipeline_layout_h_ */
