#ifndef __queue_h_
#define __queue_h_

#include "ext/volk/volk.h"

#include "def.h"

/**
 * \brief Vulkan queue class
 */
class queue
{
public:
  /**
   * \brief Default constructor.
   */
  queue( VOID ) = default;

  /**
   * \brief Queue constructor
   * \param[in] Device Device identifier
   * \param[in] FamilyIndex Queue family index
   * \param[in] Index Queue index
   */
  queue( const VkDevice Device, const UINT32 FamilyIndex, const UINT32 Index );

  /**
   * \brief Submit command buffers function
   * \param[in] Submits Submits structures
   * \param[in] NumberOfSubmits Number of submits structures
   * \param[in] Fence Commands fence
   */
  VOID Submit( const VkSubmitInfo *Submits, const UINT32 NumberOfSubmits = 1,
               const VkFence Fence = VK_NULL_HANDLE ) const;

  /**
   * \brief Get queue identifier function
   * \return Queue identifier
   */
  VkQueue GetQueueId( VOID ) const noexcept;

private:
  /** Queue identifier */
  VkQueue QueueId = VK_NULL_HANDLE;
};

#endif /* __queue_h_ */
