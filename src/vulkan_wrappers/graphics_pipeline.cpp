#include "graphics_pipeline.h"
#include "vulkan_validation.h"

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
graphics_pipeline::graphics_pipeline( VkDevice Device,
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
                                      UINT32 SubpassInRenderPass ) :
  DeviceId(Device)
{
  VkGraphicsPipelineCreateInfo CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  CreateInfo.pNext = nullptr;
  CreateInfo.flags = Flags;
  CreateInfo.stageCount = NumberOfStages;
  CreateInfo.pStages = Stages;
  CreateInfo.pVertexInputState = VertexInputState;
  CreateInfo.pInputAssemblyState = InputAssemblyState;
  CreateInfo.pTessellationState = TessellationState;
  CreateInfo.pViewportState = ViewportState;
  CreateInfo.pRasterizationState = RasterizationState;
  CreateInfo.pMultisampleState = MultisampleState;
  CreateInfo.pDepthStencilState = DepthStencilState;
  CreateInfo.pColorBlendState = ColorBlendState;
  CreateInfo.pDynamicState = DynamicStateCreateInfo;
  CreateInfo.layout = PipelineLayout;
  CreateInfo.renderPass = RenderPass;
  CreateInfo.subpass = SubpassInRenderPass;
  CreateInfo.basePipelineHandle = VK_NULL_HANDLE;
  CreateInfo.basePipelineHandle = 0;

  vulkan_validation::Check(
    vkCreateGraphicsPipelines(Device, Cache, 1, &CreateInfo, nullptr, &PipelineId),
    "graphics pipeline creation failed");
}

/**
 * \brief Graphics pipeline destructor
 */
graphics_pipeline::~graphics_pipeline( VOID )
{
  if (PipelineId != VK_NULL_HANDLE)
    vkDestroyPipeline(DeviceId, PipelineId, nullptr);
}

/**
 * \brief Get graphics pipeline identifier
 * \return Pipeline identifier
 */
VkPipeline graphics_pipeline::GetPipelineId( VOID ) const noexcept
{
  return PipelineId;
}

/**
 * \brief Move function
 * \param[in] Pipeline Vulkan graphics pipeline
 * \return Reference to this
 */
graphics_pipeline & graphics_pipeline::operator=( graphics_pipeline &&Pipeline ) noexcept
{
  std::swap(PipelineId, Pipeline.PipelineId);
  std::swap(DeviceId, Pipeline.DeviceId);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] Pipeline Vulkan graphics pipeline
 * \return Reference to this
 */
graphics_pipeline::graphics_pipeline( graphics_pipeline &&Pipeline ) noexcept
{
  std::swap(PipelineId, Pipeline.PipelineId);
  std::swap(DeviceId, Pipeline.DeviceId);
}
