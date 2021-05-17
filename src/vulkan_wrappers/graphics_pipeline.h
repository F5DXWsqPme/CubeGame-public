#ifndef __graphics_pipeline_h_
#define __graphics_pipeline_h_

#include "ext/volk/volk.h"

#include "def.h"

/**
 * \brief Graphics pipeline class
 */
class graphics_pipeline
{
public:
  /**
   * \brief Graphics pipeline constructor
   */
  graphics_pipeline( VOID ) = default;

  /**
   * \brief Graphics pipeline constructor
   * \param Device Vulkan device identifier
   * \param Cache Pipeline creation cache
   * \param Flags Pipeline creation flags
   * \param NumberOfStages Number of stages
   * \param Stages Pipeline stages
   * \param VertexInputState Vertex input stage create info
   * \param InputAssemblyState Input assembly stage create info
   * \param TessellationState Tesselation stage create info
   * \param ViewportState Viewport stage create info
   * \param RasterizationState Rasterization stage create info
   * \param MultisampleState Multisample stage create info
   * \param DepthStencilState Depth stencil stage create info
   * \param ColorBlendState Color blend stage create info
   * \param DynamicStateCreateInfo Dynamic stage create info
   * \param PipelineLayout Pipeline layout
   * \param RenderPass Render pass identifier
   * \param SubpassInRenderPass Subpass number in render pass
   */
  graphics_pipeline( VkDevice Device,
                     VkPipelineCache Cache,
                     VkPipelineCreateFlags Flags,
                     UINT32 NumberOfStages,
                     const VkPipelineShaderStageCreateInfo *Stages,
                     const VkPipelineVertexInputStateCreateInfo *VertexInputState,
                     const VkPipelineInputAssemblyStateCreateInfo *InputAssemblyState,
                     const VkPipelineTessellationStateCreateInfo *TessellationState,
                     const VkPipelineViewportStateCreateInfo *ViewportState,
                     const VkPipelineRasterizationStateCreateInfo *RasterizationState,
                     const VkPipelineMultisampleStateCreateInfo *MultisampleState,
                     const VkPipelineDepthStencilStateCreateInfo *DepthStencilState,
                     const VkPipelineColorBlendStateCreateInfo *ColorBlendState,
                     const VkPipelineDynamicStateCreateInfo *DynamicStateCreateInfo,
                     VkPipelineLayout PipelineLayout,
                     VkRenderPass RenderPass,
                     UINT32 SubpassInRenderPass );

  /**
   * \brief Graphics pipeline destructor
   */
  ~graphics_pipeline( VOID );

  /**
   * \brief Get graphics pipeline identifier
   * \return Pipeline identifier
   */
  VkPipeline GetPipelineId( VOID ) const noexcept;

  /**
   * \brief Move function
   * \param[in] Pipeline Vulkan graphics pipeline
   * \return Reference to this
   */
  graphics_pipeline & operator=( graphics_pipeline &&Pipeline ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] Pipeline Vulkan graphics pipeline
   * \return Reference to this
   */
  graphics_pipeline( graphics_pipeline &&Pipeline ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] Pipeline Vulkan graphics pipeline
   * \return Reference to this
   */
  graphics_pipeline & operator=( const graphics_pipeline &Pipeline ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] Pipeline Vulkan graphics pipeline
   * \return Reference to this
   */
  graphics_pipeline( const graphics_pipeline &Pipeline ) = delete;
  
  /** Pipeline identifier */
  VkPipeline PipelineId = VK_NULL_HANDLE;

  /** Vulkan device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /* __graphics_pipeline_h_ */
