#ifndef __command_buffer_h_
#define __command_buffer_h_

#include "ext/volk/volk.h"

#include "compute_pipeline.h"
#include "graphics_pipeline.h"

/**
 * \brief Vulkan command buffer class
 */
class command_buffer
{
public:
  /**
   * \brief Command buffer constructor
   * \param[in] Buffer Command buffer
   */
  command_buffer( const VkCommandBuffer Buffer );

  /**
   * \brief Begin command buffer function
   * \param[in] Flags Buffer usage flags
   * \param[in] InheritanceInfo Buffer inheritance information
   */
  VOID Begin( const VkCommandBufferUsageFlags Flags,
              const VkCommandBufferInheritanceInfo *InheritanceInfo = nullptr ) const;

  /**
   * \brief End command buffer function
   */
  VOID End( VOID ) const;

  /**
   * \brief Reset command buffer function
   * \param[in] Flags Reset flags
   */
  VOID Reset( const VkCommandBufferResetFlags Flags = 0 ) const;

  /**
   * \brief Get command buffer function
   * \return Command buffer identifier
   */
  VkCommandBuffer GetCommandBuffer( VOID ) const noexcept;

  /**
   * \brief Bind compute pipeline to command buffer
   * \param[in] Pipeline Compute pipeline
   */
  VOID CmdBindComputePipeline( const compute_pipeline &Pipeline ) const;

  /**
   * \brief Bind graphics pipeline to command buffer
   * \param[in] Pipeline Graphics pipeline
   */
  VOID CmdBindGraphicsPipeline( const graphics_pipeline &Pipeline ) const;

private:
  /** Command buffer identifier */
  VkCommandBuffer CommandBufferId = VK_NULL_HANDLE;
};

#endif /* __command_buffer_h_ */
