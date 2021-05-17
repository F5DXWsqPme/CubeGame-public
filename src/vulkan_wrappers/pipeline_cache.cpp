#include <cassert>

#include "vulkan_validation.h"
#include "pipeline_cache.h"

/**
 * \brief Empty pipeline cache constructor
 * \param[in] Device Device identifier
 */
pipeline_cache::pipeline_cache( const VkDevice Device ) : DeviceId(Device)
{
  VkPipelineCacheCreateInfo CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  CreateInfo.pNext = nullptr;
  CreateInfo.flags = 0;
  CreateInfo.initialDataSize = 0;
  CreateInfo.pInitialData = nullptr;

  vulkan_validation::Check(
    vkCreatePipelineCache(DeviceId, &CreateInfo, nullptr, &PipelineCacheId),
    "empty pipeline cache creation failed");
}

/**
 * \brief Pipeline cache destructor
 */
pipeline_cache::~pipeline_cache()
{
  if (PipelineCacheId != VK_NULL_HANDLE)
    vkDestroyPipelineCache(DeviceId, PipelineCacheId, nullptr);
}

/**
 * \brief Move function
 * \param[in] Cache Vulkan cache module
 * \return Reference to this
 */
pipeline_cache & pipeline_cache::operator=( pipeline_cache &&Cache ) noexcept
{
  //assert(PipelineCacheId == VK_NULL_HANDLE);

  std::swap(PipelineCacheId, Cache.PipelineCacheId);
  std::swap(DeviceId, Cache.DeviceId);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] Cache Vulkan cache module
 * \return Reference to this
 */
pipeline_cache::pipeline_cache( pipeline_cache &&Cache ) noexcept
{
  assert(PipelineCacheId == VK_NULL_HANDLE);

  std::swap(PipelineCacheId, Cache.PipelineCacheId);
  std::swap(DeviceId, Cache.DeviceId);
}

/**
 * \brief Get pipeline cache identifier
 * \return Pipeline cache identifier
 */
VkPipelineCache pipeline_cache::GetPipelineCacheId( VOID ) const noexcept
{
  return PipelineCacheId;
}
