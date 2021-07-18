#include <stdexcept>
#include <iostream>
#include <cstddef>
#include <optional>
#include <numeric>

#include "vulkan_wrappers/vulkan_application.h"
#include "vulkan_wrappers/graphics_pipeline.h"
#include "vulkan_wrappers/pipeline_cache.h"
#include "vulkan_wrappers/render_pass.h"
#include "vulkan_wrappers/shader_module.h"
#include "vulkan_wrappers/glfw_surface.h"
#include "vulkan_wrappers/pipeline_layout.h"
#include "vulkan_wrappers/descriptor_set_layout.h"
#include "vulkan_wrappers/swapchain.h"
#include "vulkan_wrappers/vulkan_validation.h"
#include "vulkan_wrappers/fence.h"
#include "vulkan_wrappers/command_pool.h"
#include "vulkan_wrappers/command_buffer.h"
#include "vulkan_wrappers/queue.h"
#include "vulkan_wrappers/memory.h"
#include "vulkan_wrappers/buffer.h"
#include "vulkan_wrappers/descriptor_pool.h"

#include "glfw_window.h"
#include "render.h"
#include "vertex.h"

/**
 * \brief Create render function
 * \param[in] VkApp Vulkan application
 * \param[in] Surface Surface for presentation
 * \param[in] MaxNumberOfSecondaryBuffers Number of secondary buffers (number of draw elements)
 * \param[in, out] MemoryManager Memory manager
 * \param[in, out] Synchronization Synchronization object
 */
render::render( vulkan_application &VkApp, const glfw_surface &Surface, UINT32 MaxNumberOfSecondaryBuffers,
                memory_manager &MemoryManager, render_synchronization &Synchronization ) :
  SurfaceSize(Surface.GetSurfaceSize()), VkApp(VkApp), Surface(Surface),
  MemoryManager(MemoryManager), Synchronization(Synchronization),
  TextureAtlas(VkApp, "textures/atlas/atlas.xml", "textures/atlas/atlas.png")
{
  Camera.SetWH(SurfaceSize.width, SurfaceSize.height);

  GraphicsQueue = queue(VkApp.GetDeviceId(), *VkApp.GraphicsQueueFamilyIndex, 0);
  PresentationQueue = queue(VkApp.GetDeviceId(), *VkApp.PresentationQueueFamilyIndex, 0);

  GraphicsCommandPool = command_pool(VkApp.GetDeviceId(), *VkApp.GraphicsQueueFamilyIndex,
                                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  SecondaryGraphicsCommandPool = command_pool(VkApp.GetDeviceId(), *VkApp.GraphicsQueueFamilyIndex,
                                              VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  NextImageFence = fence(VkApp.GetDeviceId());
  RenderFence = fence(VkApp.GetDeviceId());
  RenderToPresentationSemaphore = semaphore(VkApp.GetDeviceId());

  CreateDepthBuffer();
  CreateRenderPass();
  CreateDefaultGraphicsPipeline();

  Swapchain = swapchain(VkApp.GetDeviceId(),
                        Surface.GetSurfaceFormat(),
                        Surface.GetSurfaceId(), RenderPass.GetRenderPassId(),
                        SurfaceSize, DepthBufferView.GetImageViewId());

  CreateDescriptorPoolAndAllocateSets();
  WriteDescriptorSets();

  ImageIndices.resize(Swapchain.GetNumberOfFramebuffers());

  std::iota(ImageIndices.begin(), ImageIndices.end(), 0);

  SubmitInfos.resize(Swapchain.GetNumberOfFramebuffers());
  PresentInfos.resize(Swapchain.GetNumberOfFramebuffers());
  DrawCommandBuffers.resize(Swapchain.GetNumberOfFramebuffers());
  
  GraphicsCommandPool.AllocateCommandBuffers(DrawCommandBuffers.data(),
    Swapchain.GetNumberOfFramebuffers(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  if (MaxNumberOfSecondaryBuffers > 0)
  {
    UnusedSecondaryCommandBuffers.resize(MaxNumberOfSecondaryBuffers);
    SecondaryGraphicsCommandPool.AllocateCommandBuffers(UnusedSecondaryCommandBuffers.data(), MaxNumberOfSecondaryBuffers,
                                                        VK_COMMAND_BUFFER_LEVEL_SECONDARY);
  }

  //AppliedMatrWVP = Camera.ViewProjMatrix;
  UpdateWVP();

  CreateCommandBuffers();
}

/**
 * \brief Create render pass function
 */
VOID render::CreateRenderPass( VOID )
{
  VkAttachmentDescription RenderPassAttachments[2] = {};

  RenderPassAttachments[0].flags = 0;
  RenderPassAttachments[0].format = Surface.GetSurfaceFormat().format;
  RenderPassAttachments[0].samples = NumberOfSamples;
  RenderPassAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  RenderPassAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  RenderPassAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  RenderPassAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  RenderPassAttachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  RenderPassAttachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  RenderPassAttachments[1].flags = 0;
  RenderPassAttachments[1].format = DepthFormat;
  RenderPassAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
  RenderPassAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  RenderPassAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  RenderPassAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  RenderPassAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  RenderPassAttachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  RenderPassAttachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference ColorAttachmentReference = {};

  ColorAttachmentReference.attachment = 0;
  ColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference DepthAttachmentReference = {};

  DepthAttachmentReference.attachment = 1;
  DepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription SubpassDescription = {};

  SubpassDescription.flags = 0;
  SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  SubpassDescription.inputAttachmentCount = 0;
  SubpassDescription.pInputAttachments = nullptr;
  SubpassDescription.colorAttachmentCount = 1;
  SubpassDescription.pColorAttachments = &ColorAttachmentReference;
  SubpassDescription.pResolveAttachments = nullptr;
  SubpassDescription.pDepthStencilAttachment = &DepthAttachmentReference;
  SubpassDescription.preserveAttachmentCount = 0;
  SubpassDescription.pPreserveAttachments = nullptr;

  VkSubpassDependency SubpassDependency = {};

  SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  SubpassDependency.dstSubpass = 0;
  SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  SubpassDependency.srcAccessMask = 0;
  SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  RenderPass = render_pass(VkApp.GetDeviceId(), 2, RenderPassAttachments, 1, &SubpassDescription, 1, &SubpassDependency);
}

/**
 * \brief Create default graphics pipeline function
 */
VOID render::CreateDefaultGraphicsPipeline( VOID )
{
  DefaultVertexShader = shader_module(VkApp.GetDeviceId(), "shaders-build/default/shader.vert.spv");
  DefaultFragmentShader = shader_module(VkApp.GetDeviceId(), "shaders-build/default/shader.frag.spv");

  VkPipelineShaderStageCreateInfo ShaderStageCreateInfos[2] = {};

  ShaderStageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  ShaderStageCreateInfos[0].pNext = nullptr;
  ShaderStageCreateInfos[0].flags = 0;
  ShaderStageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  ShaderStageCreateInfos[0].module = DefaultVertexShader.GetShaderModuleId();
  ShaderStageCreateInfos[0].pName = "main";
  ShaderStageCreateInfos[0].pSpecializationInfo = nullptr;

  ShaderStageCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  ShaderStageCreateInfos[1].pNext = nullptr;
  ShaderStageCreateInfos[1].flags = 0;
  ShaderStageCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  ShaderStageCreateInfos[1].module = DefaultFragmentShader.GetShaderModuleId();
  ShaderStageCreateInfos[1].pName = "main";
  ShaderStageCreateInfos[1].pSpecializationInfo = nullptr;
  
  VkVertexInputBindingDescription BindingDescription = {};

  BindingDescription.binding = 0;
  BindingDescription.stride = sizeof(VERTEX);
  BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription AttributeDescriptions[3] = {};

  AttributeDescriptions[0].location = 0;
  AttributeDescriptions[0].binding = 0;
  AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  AttributeDescriptions[0].offset = offsetof(VERTEX, Position);

  AttributeDescriptions[1].location = 1;
  AttributeDescriptions[1].binding = 0;
  AttributeDescriptions[1].format = VK_FORMAT_R32_SFLOAT;
  AttributeDescriptions[1].offset = offsetof(VERTEX, Alpha);

  AttributeDescriptions[2].location = 2;
  AttributeDescriptions[2].binding = 0;
  AttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
  AttributeDescriptions[2].offset = offsetof(VERTEX, TexCoord);

  VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo = {};

  VertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  VertexInputStateCreateInfo.pNext = nullptr;
  VertexInputStateCreateInfo.flags = 0;
  VertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
  VertexInputStateCreateInfo.pVertexBindingDescriptions = &BindingDescription;
  VertexInputStateCreateInfo.vertexAttributeDescriptionCount = 3;
  VertexInputStateCreateInfo.pVertexAttributeDescriptions = AttributeDescriptions;

  VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = {};

  InputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  InputAssemblyStateCreateInfo.pNext = nullptr;
  InputAssemblyStateCreateInfo.flags = 0;
  InputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

  VkViewport Viewport = {};

  Viewport.x = 0;
  Viewport.y = 0;
  Viewport.width = SurfaceSize.width;
  Viewport.height = SurfaceSize.height;
  Viewport.minDepth = 0;
  Viewport.maxDepth = 1;

  VkRect2D Scissor = {};

  Scissor.offset.x = 0;
  Scissor.offset.y = 0;
  Scissor.extent.width = SurfaceSize.width;
  Scissor.extent.height = SurfaceSize.height;

  VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = {};

  ViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  ViewportStateCreateInfo.pNext = nullptr;
  ViewportStateCreateInfo.flags = 0;
  ViewportStateCreateInfo.viewportCount = 1;
  ViewportStateCreateInfo.pViewports = &Viewport;
  ViewportStateCreateInfo.scissorCount = 1;
  ViewportStateCreateInfo.pScissors = &Scissor;

  VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo = {};

  RasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  RasterizationStateCreateInfo.pNext = nullptr;
  RasterizationStateCreateInfo.flags = 0;
  RasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
  RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
  RasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
  RasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
  RasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  RasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
  RasterizationStateCreateInfo.depthBiasConstantFactor = 0;
  RasterizationStateCreateInfo.depthBiasClamp = 0;
  RasterizationStateCreateInfo.depthBiasSlopeFactor = 0;
  RasterizationStateCreateInfo.lineWidth = 1;

  VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo = {};

  MultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  MultisampleStateCreateInfo.pNext = nullptr;
  MultisampleStateCreateInfo.flags = 0;
  MultisampleStateCreateInfo.rasterizationSamples = NumberOfSamples;
  MultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
  MultisampleStateCreateInfo.minSampleShading = 1;
  MultisampleStateCreateInfo.pSampleMask = nullptr;
  MultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
  MultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

  VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo = {};

  DepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  DepthStencilStateCreateInfo.pNext = nullptr;
  DepthStencilStateCreateInfo.flags = 0;
  DepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
  DepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
  DepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
  DepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
  DepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
  DepthStencilStateCreateInfo.front = {};
  DepthStencilStateCreateInfo.back = {};
  DepthStencilStateCreateInfo.minDepthBounds = 0;
  DepthStencilStateCreateInfo.maxDepthBounds = 0;

  VkPipelineColorBlendAttachmentState ColorBlendAttachmentState = {};

  ColorBlendAttachmentState.blendEnable = VK_TRUE;
  ColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  ColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  ColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
  ColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  ColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  ColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
  ColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                             VK_COLOR_COMPONENT_G_BIT |
                                             VK_COLOR_COMPONENT_B_BIT |
                                             VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo = {};

  ColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  ColorBlendStateCreateInfo.pNext = nullptr;
  ColorBlendStateCreateInfo.flags = 0;
  ColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
  ColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_NO_OP;
  ColorBlendStateCreateInfo.attachmentCount = 1;
  ColorBlendStateCreateInfo.pAttachments = &ColorBlendAttachmentState;
  ColorBlendStateCreateInfo.blendConstants[0] = 0;
  ColorBlendStateCreateInfo.blendConstants[1] = 0;
  ColorBlendStateCreateInfo.blendConstants[2] = 0;
  ColorBlendStateCreateInfo.blendConstants[3] = 0;

  VkDescriptorSetLayoutBinding LayoutBindings[2] = {};

  LayoutBindings[0].binding = 0;
  LayoutBindings[0].pImmutableSamplers = nullptr;
  LayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  LayoutBindings[0].descriptorCount = 1;
  LayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  LayoutBindings[1].binding = 1;
  LayoutBindings[1].pImmutableSamplers = nullptr;
  LayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  LayoutBindings[1].descriptorCount = 1;
  LayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  DefaultDescriptorSetLayout = descriptor_set_layout(VkApp.GetDeviceId(), 2, LayoutBindings);
  VkDescriptorSetLayout DescriptorSetLayoutId = DefaultDescriptorSetLayout.GetSetLayoutId();

 //VkPushConstantRange MatrWVPPushConstantRange = {};

 //MatrWVPPushConstantRange.size = sizeof(glm::mat4);
 //MatrWVPPushConstantRange.offset = 0;
 //MatrWVPPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  DefaultPipelineLayout =
    pipeline_layout(VkApp.GetDeviceId(), 1, &DescriptorSetLayoutId, 0, nullptr/*1, &MatrWVPPushConstantRange*/);

  pipeline_cache EmptyCache;

  DefaultGraphicsPipeline = graphics_pipeline(VkApp.GetDeviceId(),
    EmptyCache.GetPipelineCacheId(), 0, 2, ShaderStageCreateInfos, &VertexInputStateCreateInfo,
    &InputAssemblyStateCreateInfo, nullptr, &ViewportStateCreateInfo,
    &RasterizationStateCreateInfo, &MultisampleStateCreateInfo, &DepthStencilStateCreateInfo,
    &ColorBlendStateCreateInfo, nullptr,
    DefaultPipelineLayout.GetPipelineLayoutId(), RenderPass.GetRenderPassId(), 0);
}

/**
 * \brief Create descriptor pool and allocate descriptor sets function.
 */
VOID render::CreateDescriptorPoolAndAllocateSets( VOID )
{
  VkDescriptorPoolSize DescriptorPoolSizes[2];

  DescriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  DescriptorPoolSizes[0].descriptorCount = 1;

  DescriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  DescriptorPoolSizes[1].descriptorCount = 1;

  DefaultDescriptorPool =
    descriptor_pool(VkApp.GetDeviceId(), 0, 1, 2, DescriptorPoolSizes);

  VkDescriptorSetLayout DescriptorSetsLayout[1] =
  {
    DefaultDescriptorSetLayout.GetSetLayoutId()
  };

  DefaultDescriptorPool.AllocateSets(&DefaultDescriptorSet, 1, DescriptorSetsLayout);
}

/**
 * \brief Write descriptor sets function.
 */
VOID render::WriteDescriptorSets( VOID )
{
  VkWriteDescriptorSet WriteDescriptorSetStructures[2] = {};

  VkDescriptorImageInfo ImageInfos[1] = {};

  ImageInfos[0].sampler = TextureAtlas.Sampler.GetSamplerId();
  ImageInfos[0].imageView = TextureAtlas.ImageView.GetImageViewId();
  ImageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  WriteDescriptorSetStructures[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  WriteDescriptorSetStructures[0].pNext = nullptr;
  WriteDescriptorSetStructures[0].dstSet = DefaultDescriptorSet;
  WriteDescriptorSetStructures[0].dstBinding = 0;
  WriteDescriptorSetStructures[0].dstArrayElement = 0;
  WriteDescriptorSetStructures[0].descriptorCount = 1;
  WriteDescriptorSetStructures[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  WriteDescriptorSetStructures[0].pImageInfo = &ImageInfos[0];
  WriteDescriptorSetStructures[0].pBufferInfo = nullptr;
  WriteDescriptorSetStructures[0].pTexelBufferView = nullptr;

  VkDescriptorBufferInfo BufferInfos[1] = {};

  BufferInfos[0].buffer = MemoryManager.UniformBuffer.GetBufferId();
  BufferInfos[0].offset = 0;
  BufferInfos[0].range = sizeof(uniform_buffer);

  WriteDescriptorSetStructures[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  WriteDescriptorSetStructures[1].pNext = nullptr;
  WriteDescriptorSetStructures[1].dstSet = DefaultDescriptorSet;
  WriteDescriptorSetStructures[1].dstBinding = 1;
  WriteDescriptorSetStructures[1].dstArrayElement = 0;
  WriteDescriptorSetStructures[1].descriptorCount = 1;
  WriteDescriptorSetStructures[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  WriteDescriptorSetStructures[1].pImageInfo = nullptr;
  WriteDescriptorSetStructures[1].pBufferInfo = &BufferInfos[0];
  WriteDescriptorSetStructures[1].pTexelBufferView = nullptr;

  vkUpdateDescriptorSets(VkApp.GetDeviceId(), 2, WriteDescriptorSetStructures, 0, nullptr);
}

/**
 * \brief Create depth buffer function
 */
VOID render::CreateDepthBuffer( VOID )
{
  VkImageCreateInfo ImageCreateInfo = {};

  ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ImageCreateInfo.pNext = nullptr;
  ImageCreateInfo.flags = 0;
  ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  ImageCreateInfo.format = DepthFormat;
  ImageCreateInfo.extent = {SurfaceSize.width, SurfaceSize.height, 1};
  ImageCreateInfo.mipLevels = 1;
  ImageCreateInfo.arrayLayers = 1;
  ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  ImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  ImageCreateInfo.queueFamilyIndexCount = 0;
  ImageCreateInfo.pQueueFamilyIndices = nullptr;
  ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  DepthBuffer = image(VkApp.GetDeviceId(), ImageCreateInfo);

  VkMemoryRequirements ImageMemoryRequirements = {};

  ImageMemoryRequirements = DepthBuffer.GetMemoryRequirements();

  std::optional<UINT32> ImageMemoryIndex =
    VkApp.FindMemoryTypeWithFlags(ImageMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (!ImageMemoryIndex)
    ImageMemoryIndex =
      VkApp.FindMemoryTypeWithFlags(ImageMemoryRequirements, 0);

  if (!ImageMemoryIndex)
    throw std::runtime_error("memory for texture atlas image not found");

  DepthBufferMemory = memory(VkApp.GetDeviceId(), ImageMemoryRequirements.size, *ImageMemoryIndex);

  DepthBuffer.BindMemory(DepthBufferMemory, 0);

  VkCommandBuffer CommandBufferId = VK_NULL_HANDLE;

  GraphicsCommandPool.AllocateCommandBuffers(&CommandBufferId);

  command_buffer CommandBuffer(CommandBufferId);

  CommandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  {
    VkImageMemoryBarrier Barrier = {};

    Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.pNext = nullptr;
    Barrier.srcAccessMask = 0;
    Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    Barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.image = DepthBuffer.GetImageId();
    Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    Barrier.subresourceRange.baseMipLevel = 0;
    Barrier.subresourceRange.levelCount = 1;
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(CommandBufferId, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &Barrier);
  }

  CommandBuffer.End();

  VkSubmitInfo SubmitInfo = {};

  SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  SubmitInfo.pNext = nullptr;
  SubmitInfo.waitSemaphoreCount = 0;
  SubmitInfo.pWaitSemaphores = nullptr;
  SubmitInfo.pWaitDstStageMask = nullptr;
  SubmitInfo.commandBufferCount = 1;
  SubmitInfo.pCommandBuffers = &CommandBufferId;
  SubmitInfo.signalSemaphoreCount = 0;
  SubmitInfo.pSignalSemaphores = nullptr;

  GraphicsQueue.Submit(&SubmitInfo, 1, RenderFence.GetFenceId());

  RenderFence.Wait();
  RenderFence.Reset();

  GraphicsCommandPool.FreeCommandBuffers(&CommandBufferId);

  VkImageViewCreateInfo ImageViewCreateInfo = {};

  ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ImageViewCreateInfo.pNext = nullptr;
  ImageViewCreateInfo.flags = 0;
  ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ImageViewCreateInfo.format = DepthFormat;
  ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
  ImageViewCreateInfo.subresourceRange.levelCount = 1;
  ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
  ImageViewCreateInfo.subresourceRange.layerCount = 1;
  ImageViewCreateInfo.image = DepthBuffer.GetImageId();

  DepthBufferView = image_view(VkApp.GetDeviceId(), ImageViewCreateInfo);
}

/**
 * \brief Get secondary command buffer function
 * \return New command buffer
 */
VkCommandBuffer render::GetSecondaryCommandBuffer( VOID )
{
  VkCommandBuffer Res = UnusedSecondaryCommandBuffers.back();

  UnusedSecondaryCommandBuffers.pop_back();

  return Res;
}

/**
 * \brief Revert secondary command buffer function
 * \param[in] CommandBuffer Buffer for return in pool
 */
VOID render::ReturnSecondaryCommandBuffer( VkCommandBuffer CommandBuffer )
{
  UnusedSecondaryCommandBuffers.push_back(CommandBuffer);
}

/**
 * \brief Create command buffers for every framebuffer
 */
VOID render::CreateCommandBuffers( VOID )
{
  SwapchainId = Swapchain.GetSwapchainId();

  for (UINT32 ImageIndex = 0;
       ImageIndex < Swapchain.GetNumberOfFramebuffers();
       ImageIndex++)
  {
    VkCommandBuffer CommandBufferId = DrawCommandBuffers[ImageIndex];
    command_buffer CommandBuffer(CommandBufferId);

    CommandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

    VkClearValue ClearValues[2] = {};

    ClearValues[0] = ClearColor;
    ClearValues[1].depthStencil.depth = 1;
    ClearValues[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo RenderPassBeginInfo = {};

    RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.pNext = nullptr;
    RenderPassBeginInfo.renderPass = RenderPass.GetRenderPassId();
    RenderPassBeginInfo.framebuffer = Swapchain.GetFramebufferId(ImageIndex);
    RenderPassBeginInfo.renderArea.offset.x = 0;
    RenderPassBeginInfo.renderArea.offset.y = 0;
    RenderPassBeginInfo.renderArea.extent = Surface.GetSurfaceSize();
    RenderPassBeginInfo.clearValueCount = 2;
    RenderPassBeginInfo.pClearValues = ClearValues;

    vkCmdBeginRenderPass(CommandBufferId, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    //vkCmdPushConstants(CommandBufferId, DefaultPipelineLayout.GetPipelineLayoutId(),
    //                   VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
    //                   reinterpret_cast<const VOID *>(&MatrWVP));

    if (SecondaryCommandBuffersVector.size() > 0)
    {
      vkCmdExecuteCommands(CommandBufferId, SecondaryCommandBuffersVector.size(),
                           SecondaryCommandBuffersVector.data());
    }

    vkCmdEndRenderPass(CommandBufferId);

    {
      VkImageMemoryBarrier Barrier = {};

      Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      Barrier.pNext = nullptr;
      Barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      Barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      Barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      Barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

      if (*VkApp.PresentationQueueFamilyIndex != *VkApp.GraphicsQueueFamilyIndex)
      {
        Barrier.srcQueueFamilyIndex = *VkApp.PresentationQueueFamilyIndex;
        Barrier.dstQueueFamilyIndex = *VkApp.GraphicsQueueFamilyIndex;
      }
      else
      {
        Barrier.srcQueueFamilyIndex = 0;
        Barrier.dstQueueFamilyIndex = 0;
      }

      Barrier.image = Swapchain.GetImageId(ImageIndex);
      Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      Barrier.subresourceRange.baseMipLevel = 0;
      Barrier.subresourceRange.levelCount = 1;
      Barrier.subresourceRange.baseArrayLayer = 0;
      Barrier.subresourceRange.layerCount = 1;

      vkCmdPipelineBarrier(CommandBufferId, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                           VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &Barrier);
    }
    
    CommandBuffer.End();

    RenderToPresentationSemaphoreId = RenderToPresentationSemaphore.GetSemaphoreId();

    SubmitInfos[ImageIndex].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfos[ImageIndex].pNext = nullptr;
    SubmitInfos[ImageIndex].waitSemaphoreCount = 0;
    SubmitInfos[ImageIndex].pWaitSemaphores = nullptr;
    SubmitInfos[ImageIndex].pWaitDstStageMask = nullptr;
    SubmitInfos[ImageIndex].commandBufferCount = 1;
    SubmitInfos[ImageIndex].pCommandBuffers = &DrawCommandBuffers[ImageIndex];
    SubmitInfos[ImageIndex].signalSemaphoreCount = 1;
    SubmitInfos[ImageIndex].pSignalSemaphores = &RenderToPresentationSemaphoreId;

    PresentInfos[ImageIndex].sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfos[ImageIndex].pNext = nullptr;
    PresentInfos[ImageIndex].waitSemaphoreCount = 1;
    PresentInfos[ImageIndex].pWaitSemaphores = &RenderToPresentationSemaphoreId;
    PresentInfos[ImageIndex].swapchainCount = 1;
    PresentInfos[ImageIndex].pSwapchains = &SwapchainId;
    PresentInfos[ImageIndex].pImageIndices = &ImageIndices[ImageIndex];
    PresentInfos[ImageIndex].pResults = &PresentResult;
  }
}

/**
  * \brief Evaluate and print FPS function
  * \param Time Current time
  */
VOID render::DetectFPS( FLT Time )
{
  if (OldFPSEvaluationTime < 0)
    OldFPSEvaluationTime = Time;

  FLT DeltaTime = Time - OldFPSEvaluationTime;

  if (DeltaTime > 1)
  {
    std::cout << "FPS: " << (NumberOfFrames / DeltaTime) << "\n" << std::endl;
    NumberOfFrames = 0;
    OldFPSEvaluationTime = Time;
  }

  NumberOfFrames++;
}

/**
 * \brief Render frame function
 * \param Time Current time
 */
VOID render::RenderFrame( FLT Time )
{
  UINT32 ImageIndex = 0;

  BOOL IsAcquireNextImageReturnSuccess =
    vkAcquireNextImageKHR(VkApp.GetDeviceId(), Swapchain.GetSwapchainId(), std::numeric_limits<UINT64>::max(),
                          VK_NULL_HANDLE, NextImageFence.GetFenceId(), &ImageIndex) == VK_SUCCESS;

  if (!IsAcquireNextImageReturnSuccess || NextImageFence.Wait(NextImageTimeout) != VK_SUCCESS)
  {
    IsAcquireNextImageReturnSuccess = FALSE;

    VkExtent2D NewSize = Surface.GetSurfaceSize();

    if (NewSize.width != 0 && NewSize.height != 0)
    {
      std::cout << "Recreating swapchain\n" << std::endl;

      if (NewSize.width != SurfaceSize.width || NewSize.height != SurfaceSize.height)
        throw std::runtime_error("size of surface was changed");

      std::lock_guard<std::mutex> Lock(Synchronization.RenderMutex);

      Swapchain.Resize(NewSize);

      UpdateCommandBuffers();

      std::cout << "Swapchain recreated\n" << std::endl;
    }

    NextImageFence.Reset();

    return;
  }

  NextImageFence.Reset();

  {
    std::lock_guard<std::mutex> RenderLock(Synchronization.RenderMutex);
    
    GraphicsQueue.Submit(&SubmitInfos[ImageIndex], 1, RenderFence.GetFenceId());

    VkResult PresentFuncResult = vkQueuePresentKHR(PresentationQueue.GetQueueId(), &PresentInfos[ImageIndex]);

    if (PresentResult != VK_SUCCESS || PresentFuncResult != VK_SUCCESS)
    {
      std::cout << "Presentation failed\n" << std::endl;
    }

    DetectFPS(Time);

    RenderFence.Wait();
  }
  RenderFence.Reset();
}

/**
 * \brief Render destructor
 */
render::~render( VOID )
{
  GraphicsCommandPool.Reset();
}

/**
 * \brief Update WVP matrix
 * \param[in] World World matrix
 */
VOID render::UpdateWVP( const glm::mat4 &World )
{
  UniformBufferData.MatrWVP = Camera.ViewProjMatrix * World;

  MemoryManager.SmallUpdateBuffer(MemoryManager.UniformBuffer, 0, sizeof(uniform_buffer),
    reinterpret_cast<BYTE *>(&UniformBufferData),
    VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

  //std::lock_guard<std::mutex> Lock(Synchronization.RenderMutex);
  //
  //AppliedMatrWVP = Camera.ViewProjMatrix * World;
  //
  //for (draw_element *Element : DrawElements)
  //  Element->UpdateWVP();
  //
  //CreateCommandBuffers();

  //command_buffer CommandBuffer(UpdateWVPCommandBufferId);
  //
  //CommandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
  //
  //vkCmdPushConstants(UpdateWVPCommandBufferId, DefaultPipelineLayout.GetPipelineLayoutId(),
  //                   VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
  //                   reinterpret_cast<const VOID *>(&WVP));
  //
  //CommandBuffer.End();
  //
  //VkSubmitInfo SubmitInfo = {};
  //
  //SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  //SubmitInfo.pNext = nullptr;
  //SubmitInfo.waitSemaphoreCount = 0;
  //SubmitInfo.pWaitSemaphores = nullptr;
  //SubmitInfo.pWaitDstStageMask = nullptr;
  //SubmitInfo.commandBufferCount = 1;
  //SubmitInfo.pCommandBuffers = &UpdateWVPCommandBufferId;
  //SubmitInfo.signalSemaphoreCount = 0;
  //SubmitInfo.pSignalSemaphores = nullptr;
  //
  //if (VkApp.GraphicsQueueFamilyIndex == VkApp.TransferQueueFamilyIndex)
  //{
  //  std::lock_guard<std::mutex> Lock(Synchronization.TransferAndGraphicsSubmitMutex);
  //
  //  GraphicsQueue.Submit(&SubmitInfo, 1, Fence.GetFenceId());
  //}
  //else
  //  GraphicsQueue.Submit(&SubmitInfo, 1, Fence.GetFenceId());
  //
  //Fence.Wait();
  //
  //Fence.Reset();
  //
  //CommandBuffer.Reset();
}

/**
 * \brief Update WVP matrix without world matrix
 */
VOID render::UpdateWVP( VOID )
{
  UniformBufferData.MatrWVP = Camera.ViewProjMatrix;

  MemoryManager.SmallUpdateBuffer(MemoryManager.UniformBuffer, 0, sizeof(uniform_buffer),
                                  reinterpret_cast<BYTE *>(&UniformBufferData),
                                  VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                                  VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

  //std::lock_guard<std::mutex> Lock(Synchronization.RenderMutex);
  //
  //AppliedMatrWVP = Camera.ViewProjMatrix;
  //
  //for (draw_element *Element : DrawElements)
  //  Element->UpdateWVP();
  //
  //CreateCommandBuffers();

  //command_buffer CommandBuffer(UpdateWVPCommandBufferId);
  //
  //CommandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
  //
  //vkCmdPushConstants(UpdateWVPCommandBufferId, DefaultPipelineLayout.GetPipelineLayoutId(),
  //                   VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
  //                   reinterpret_cast<const VOID *>(&Camera.ViewProjMatrix));
  //
  //CommandBuffer.End();
  //
  //VkSubmitInfo SubmitInfo = {};
  //
  //SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  //SubmitInfo.pNext = nullptr;
  //SubmitInfo.waitSemaphoreCount = 0;
  //SubmitInfo.pWaitSemaphores = nullptr;
  //SubmitInfo.pWaitDstStageMask = nullptr;
  //SubmitInfo.commandBufferCount = 1;
  //SubmitInfo.pCommandBuffers = &UpdateWVPCommandBufferId;
  //SubmitInfo.signalSemaphoreCount = 0;
  //SubmitInfo.pSignalSemaphores = nullptr;
  //
  //if (VkApp.GraphicsQueueFamilyIndex == VkApp.TransferQueueFamilyIndex)
  //{
  //  std::lock_guard<std::mutex> Lock(Synchronization.TransferAndGraphicsSubmitMutex);
  //
  //  GraphicsQueue.Submit(&SubmitInfo, 1, Fence.GetFenceId());
  //}
  //else
  //  GraphicsQueue.Submit(&SubmitInfo, 1, Fence.GetFenceId());
  //
  //Fence.Wait();
  //
  //Fence.Reset();
  //
  //CommandBuffer.Reset();
}


/**
 * \brief Add draw element function
 * \param[in] Element Pointer to element
 */
VOID render::AddDrawElement( draw_element *Element )
{
  std::lock_guard<std::mutex> Lock(Synchronization.RenderMutex);

  DrawElements.insert(Element);

  SecondaryCommandBuffersVector.clear();
  SecondaryCommandBuffersVector.reserve(DrawElements.size());

  for (draw_element *Element : DrawElements)
    SecondaryCommandBuffersVector.push_back(Element->GetCommandBuffer());

  for (draw_element *Element : DrawElements)
    SecondaryCommandBuffersVector.push_back(Element->GetTransparentCommandBuffer());

  UpdateCommandBuffers();
}

/**
 * \brief Update command buffers
 */
VOID render::UpdateCommandBuffers( VOID )
{
  for (VkCommandBuffer CommandBufferId : DrawCommandBuffers)
    command_buffer(CommandBufferId).Reset();

  CreateCommandBuffers();
}

/**
 * \brief Delete draw element function
 * \param[in] Element Pointer to element
 */
VOID render::DeleteDrawElement( draw_element *Element )
{
  std::lock_guard<std::mutex> Lock(Synchronization.RenderMutex);

  std::set<draw_element *>::iterator It = DrawElements.find(Element);

  if (It == DrawElements.end())
    throw std::runtime_error("element not found");

  DrawElements.erase(It);

  SecondaryCommandBuffersVector.clear();
  SecondaryCommandBuffersVector.reserve(DrawElements.size());

  for (draw_element *Element : DrawElements)
    SecondaryCommandBuffersVector.push_back(Element->GetCommandBuffer());

  for (draw_element *Element : DrawElements)
    SecondaryCommandBuffersVector.push_back(Element->GetTransparentCommandBuffer());

  UpdateCommandBuffers();
}
