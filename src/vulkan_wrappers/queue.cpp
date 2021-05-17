#include "queue.h"
#include "vulkan_validation.h"

/**
 * \brief Queue constructor
 * \param[in] Device Device identifier
 * \param[in] FamilyIndex Queue family index
 * \param[in] Index Queue index
 */
queue::queue( const VkDevice Device, const UINT32 FamilyIndex, const UINT32 Index )
{
  vkGetDeviceQueue(Device, FamilyIndex, Index, &QueueId);
}

/**
 * \brief Submit command buffers function
 * \param[in] Submits Submits structures
 * \param[in] NumberOfSubmits Number of submits structures
 * \param[in] Fence Commands fence
 */
VOID queue::Submit( const VkSubmitInfo *Submits, const UINT32 NumberOfSubmits,
                    const VkFence Fence ) const
{
  vulkan_validation::Check(
    vkQueueSubmit(QueueId, NumberOfSubmits, Submits, Fence),
    "queue submit failed");
}

/**
 * \brief Get queue identifier function
 * \return Queue identifier
 */
VkQueue queue::GetQueueId( VOID ) const noexcept
{
  return QueueId;
}
