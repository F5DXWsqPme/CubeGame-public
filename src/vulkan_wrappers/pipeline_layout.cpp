#include <cassert>

#include "pipeline_layout.h"
#include "vulkan_validation.h"

/**
 * \brief Pipeline layout constructor
 * \param[in] Device Device identifier
 * \param[in] NumberOfSetLayouts Number of descriptor set layouts
 * \param[in] SetLayouts Descriptor set layouts
 * \param[in] NumberOfPushConstantRanges Number of push constant ranges
 * \param[in] PushConstantRanges Push constant ranges
 */
pipeline_layout::pipeline_layout( const VkDevice Device,
  const UINT32 NumberOfSetLayouts,
  const VkDescriptorSetLayout *SetLayouts,
  const UINT32 NumberOfPushConstantRanges,
  const VkPushConstantRange *PushConstantRanges ) :
  DeviceId(Device)
{
  VkPipelineLayoutCreateInfo CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  CreateInfo.pNext = nullptr;
  CreateInfo.setLayoutCount = NumberOfSetLayouts;
  CreateInfo.pSetLayouts = SetLayouts;
  CreateInfo.pushConstantRangeCount = NumberOfPushConstantRanges;
  CreateInfo.pPushConstantRanges = PushConstantRanges;

  vulkan_validation::Check(
    vkCreatePipelineLayout(DeviceId, &CreateInfo, nullptr, &PipelineLayoutId),
    "pipeline layout creation failed");
}

/**
 * \brief Pipeline layout destructor
 */
pipeline_layout::~pipeline_layout( VOID )
{
  if (PipelineLayoutId != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(DeviceId, PipelineLayoutId, nullptr);
}

/**
 * \brief Move function
 * \param[in] Layout Vulkan pipeline layout
 * \return Reference to this
 */
pipeline_layout & pipeline_layout::operator=( pipeline_layout &&Layout ) noexcept
{
  //assert(PipelineLayoutId == VK_NULL_HANDLE);

  std::swap(PipelineLayoutId, Layout.PipelineLayoutId);
  std::swap(DeviceId, Layout.DeviceId);

  return *this;
}

/**
 * \brief Move function
 * \param[in] Layout Vulkan pipeline layout
 * \return Reference to this
 */
pipeline_layout::pipeline_layout( pipeline_layout &&Layout ) noexcept
{
  //assert(PipelineLayoutId == VK_NULL_HANDLE);

  std::swap(PipelineLayoutId, Layout.PipelineLayoutId);
  std::swap(DeviceId, Layout.DeviceId);
}

/**
 * \brief Get pipeline layout identifier
 * \return Pipeline layout identifier
 */
VkPipelineLayout pipeline_layout::GetPipelineLayoutId( VOID ) const noexcept
{
  return PipelineLayoutId;
}
