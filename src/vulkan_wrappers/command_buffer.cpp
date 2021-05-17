#include "command_buffer.h"
#include "vulkan_validation.h"

/**
 * \brief Command buffer constructor
 * \param[in] Buffer Command buffer
 */
command_buffer::command_buffer( const VkCommandBuffer Buffer ) :
  CommandBufferId(Buffer)
{
}

/**
 * \brief Begin command buffer function
 * \param[in] Flags Buffer usage flags
 * \param[in] InheritanceInfo Buffer inheritance information
 */
VOID command_buffer::Begin( const VkCommandBufferUsageFlags Flags,
                            const VkCommandBufferInheritanceInfo *InheritanceInfo ) const
{
  VkCommandBufferBeginInfo BeginInfo = {};

  BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  BeginInfo.flags = Flags;
  BeginInfo.pInheritanceInfo = InheritanceInfo;

  vulkan_validation::Check(
    vkBeginCommandBuffer(CommandBufferId, &BeginInfo),
    "begin command buffer failed");
}

/**
 * \brief End command buffer function
 */
VOID command_buffer::End( VOID ) const
{
  vulkan_validation::Check(
    vkEndCommandBuffer(CommandBufferId),
    "end command buffer failed");
}

/**
 * \brief Reset command buffer function
 * \param[in] Flags Reset flags
 */
VOID command_buffer::Reset( const VkCommandBufferResetFlags Flags ) const
{
  vulkan_validation::Check(
    vkResetCommandBuffer(CommandBufferId, Flags),
    "reset command buffer failed");
}

/**
 * \brief Get command buffer function
 * \return Command buffer identifier
 */
VkCommandBuffer command_buffer::GetCommandBuffer( VOID ) const noexcept
{
  return CommandBufferId;
}

/**
 * \brief Bind graphics pipeline to command buffer
 * \param[in] Pipeline Graphics pipeline
 */
VOID command_buffer::CmdBindGraphicsPipeline( const graphics_pipeline &Pipeline ) const
{
  vkCmdBindPipeline(CommandBufferId, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    Pipeline.GetPipelineId());
}

/**
 * \brief Bind compute pipeline to command buffer
 * \param[in] Pipeline Compute pipeline
 */
VOID command_buffer::CmdBindComputePipeline( const compute_pipeline &Pipeline ) const
{
  vkCmdBindPipeline(CommandBufferId, VK_PIPELINE_BIND_POINT_COMPUTE,
                    Pipeline.GetPipelineId());
}
