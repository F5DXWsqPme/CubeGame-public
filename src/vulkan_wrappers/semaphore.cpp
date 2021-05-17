#include <cassert>

#include "vulkan_validation.h"
#include "semaphore.h"

/**
 * \brief Semaphore constructor
 * \param[in] Device Device identifier
 */
semaphore::semaphore( VkDevice Device ) : DeviceId(Device)
{
  VkSemaphoreCreateInfo CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  CreateInfo.pNext = nullptr;
  CreateInfo.flags = 0;

  vulkan_validation::Check(
    vkCreateSemaphore(DeviceId, &CreateInfo, nullptr, &SemaphoreId),
    "semaphore creation failed");
}

/**
 * \brief Semaphore destructor
 */
semaphore::~semaphore( VOID )
{
  if (SemaphoreId != VK_NULL_HANDLE)
    vkDestroySemaphore(DeviceId, SemaphoreId, nullptr);
}

/**
 * \brief Get semaphore identifier function
 * \return Semaphore identifier
 */
VkSemaphore semaphore::GetSemaphoreId( VOID ) const noexcept
{
  return SemaphoreId;
}

/**
 * \brief Move function
 * \param[in] Semaphore Vulkan semaphore
 * \return Reference to this
 */
semaphore & semaphore::operator=( semaphore &&Semaphore ) noexcept
{
  std::swap(SemaphoreId, Semaphore.SemaphoreId);
  std::swap(DeviceId, Semaphore.DeviceId);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] Semaphore Vulkan semaphore
 * \return Reference to this
 */
semaphore::semaphore( semaphore &&Semaphore ) noexcept
{
  std::swap(SemaphoreId, Semaphore.SemaphoreId);
  std::swap(DeviceId, Semaphore.DeviceId);
}