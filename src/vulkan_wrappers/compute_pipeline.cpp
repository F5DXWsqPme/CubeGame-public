#include <cassert>

#include "compute_pipeline.h"
#include "vulkan_validation.h"

/**
 * \brief Compute pipeline constructor
 * \param[in] Device Device identifier
 * \param[in] Layout Pipeline layout
 * \param[in] Shader Shader module
 * \param[in] FuncName Name of shader function
 * \param[in] Cache Pipeline cache
 */
compute_pipeline::compute_pipeline( const VkDevice Device, const VkPipelineLayout Layout,
                                    const shader_module &Shader,
                                    const std::string_view &FuncName,
                                    const pipeline_cache &Cache ) :
  DeviceId(Device)
{
  VkComputePipelineCreateInfo CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  CreateInfo.pNext = nullptr;
  CreateInfo.flags = 0;

  CreateInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  CreateInfo.stage.pNext = nullptr;
  CreateInfo.stage.flags = 0;
  CreateInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  CreateInfo.stage.module = Shader.GetShaderModuleId();
  CreateInfo.stage.pName = FuncName.data();
  CreateInfo.stage.pSpecializationInfo = nullptr;

  CreateInfo.layout = Layout;

  vulkan_validation::Check(
    vkCreateComputePipelines(DeviceId, Cache.GetPipelineCacheId(), 1, &CreateInfo, nullptr, &PipelineId),
    "compute pipeline creating failed");
}

/**
 * \brief Get pipeline identifier
 * \return Compute pipeline identifier
 */
VkPipeline compute_pipeline::GetPipelineId( VOID ) const noexcept
{
  return PipelineId;
}

/**
 * \brief Compute pipeline destructor
 */
compute_pipeline::~compute_pipeline( VOID )
{
  if (PipelineId != VK_NULL_HANDLE)
    vkDestroyPipeline(DeviceId, PipelineId, nullptr);
}

/**
 * \brief Move function
 * \param[in] Pipeline Vulkan compute pipeline
 * \return Reference to this
 */
compute_pipeline & compute_pipeline::operator=( compute_pipeline &&Pipeline ) noexcept
{
  //assert(PipelineId == VK_NULL_HANDLE);

  std::swap(PipelineId, Pipeline.PipelineId);
  std::swap(DeviceId, Pipeline.DeviceId);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] Pipeline Vulkan compute pipeline
 * \return Reference to this
 */
compute_pipeline::compute_pipeline( compute_pipeline &&Pipeline ) noexcept
{
  //assert(PipelineId == VK_NULL_HANDLE);

  std::swap(PipelineId, Pipeline.PipelineId);
  std::swap(DeviceId, Pipeline.DeviceId);
}
