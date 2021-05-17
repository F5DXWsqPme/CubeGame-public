#include "sampler.h"
#include "vulkan_validation.h"

/**
 * \brief Sampler constructor
 * \param[in] Device Device identifier
 * \param[in] CreateInfo Sampler create information
 */
sampler::sampler( VkDevice Device, const VkSamplerCreateInfo &CreateInfo ) :
  DeviceId(Device)
{
  vulkan_validation::Check(
    vkCreateSampler(DeviceId, &CreateInfo, nullptr, &SamplerId),
    "sampler creation failed");
}

/**
 * \brief Sampler destructor
 */
sampler::~sampler( VOID )
{
  if (SamplerId != VK_NULL_HANDLE)
    vkDestroySampler(DeviceId, SamplerId, nullptr);
}

/**
 * \brief Get sampler identifier function
 * \return Sampler identifier
 */
VkSampler sampler::GetSamplerId( VOID ) const noexcept
{
  return SamplerId;
}

/**
 * \brief Move function
 * \param[in] Sampler Vulkan sampler
 * \return Reference to this
 */
sampler & sampler::operator=( sampler &&Sampler ) noexcept
{
  std::swap(SamplerId, Sampler.SamplerId);
  std::swap(DeviceId, Sampler.DeviceId);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] Sampler Vulkan buffer
 */
sampler::sampler( sampler &&Sampler ) noexcept
{
  std::swap(SamplerId, Sampler.SamplerId);
  std::swap(DeviceId, Sampler.DeviceId);
}
