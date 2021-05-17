#ifndef __render_h_
#define __render_h_

#include <set>
#include <optional>

#include "def.h"
#include "vulkan_wrappers/glfw_surface.h"
#include "vulkan_wrappers/vulkan_application.h"
#include "vulkan_wrappers/fence.h"
#include "vulkan_wrappers/semaphore.h"
#include "vulkan_wrappers/queue.h"
#include "vulkan_wrappers/command_pool.h"
#include "vulkan_wrappers/swapchain.h"
#include "vulkan_wrappers/pipeline_layout.h"
#include "vulkan_wrappers/descriptor_set_layout.h"
#include "vulkan_wrappers/shader_module.h"
#include "vulkan_wrappers/graphics_pipeline.h"
#include "vulkan_wrappers/render_pass.h"
#include "vulkan_wrappers/descriptor_pool.h"
#include "render_synchronization.h"
#include "draw_element.h"
#include "memory_manager.h"
#include "camera.h"
#include "texture_atlas.h"
#include "uniform_buffer.h"

/**
 * \brief Render class
 */
class render
{
public:
  /**
   * \brief Create render function
   * \param[in, out] VkApp Vulkan application
   * \param[in] Surface Surface for presentation
   * \param[in] MaxNumberOfSecondaryBuffers Number of secondary buffers (number of draw elements)
   * \param[in, out] MemoryManager Memory manager
   * \param[in, out] Synchronization Synchronization object
   */
  render( vulkan_application &VkApp, const glfw_surface &Surface, UINT32 MaxNumberOfSecondaryBuffers,
          memory_manager &MemoryManager, render_synchronization &Synchronization );

  /**
   * \brief Render frame function
   * \param[in] Time Current time
   */
  VOID RenderFrame( FLT Time );

  /**
   * \brief Create command buffers for every framebuffer
   */
  VOID CreateCommandBuffers( VOID );

  /**
   * \brief Get secondary command buffer function
   * \return New command buffer
   */
  VkCommandBuffer GetSecondaryCommandBuffer( VOID );

  /**
   * \brief Revert secondary command buffer function
   * \param[in] CommandBuffer Buffer for return in pool
   */
  VOID ReturnSecondaryCommandBuffer( VkCommandBuffer CommandBuffer );

  /**
   * \brief Add draw element function
   * \param[in] Element Pointer to element
   */
  VOID AddDrawElement( draw_element *Element );

  /**
   * \brief Delete draw element function
   * \param[in] Element Pointer to element
   */
  VOID DeleteDrawElement( draw_element *Element );

  /**
   * \brief Update WVP matrix
   * \param[in] World World matrix
   */
  VOID UpdateWVP( const glm::mat4 &World );

  /**
   * \brief Update WVP matrix without world matrix
   */
  VOID UpdateWVP( VOID );

  /**
   * \brief Update command buffers
   */
  VOID UpdateCommandBuffers( VOID );

  /**
   * \brief Render destructor
   */
  ~render( VOID );

  /** Memory manager */
  memory_manager &MemoryManager;

  /** Default graphics pipeline */
  graphics_pipeline DefaultGraphicsPipeline;

  /** Camera object */
  camera Camera;

  /** Render pass */
  render_pass RenderPass;

  /** Saved matrix WVP */
  //glm::mat4 AppliedMatrWVP;

  /** Default pipeline layout */
  pipeline_layout DefaultPipelineLayout;

  /** Texture atlas object */
  texture_atlas TextureAtlas;

  /** Default descriptor set */
  VkDescriptorSet DefaultDescriptorSet;

  /** Synchronization object */
  render_synchronization &Synchronization;
  
  /** Vulkan application */
  vulkan_application &VkApp;

private:
  /**
   * \brief Create depth buffer function
   */
  VOID CreateDepthBuffer( VOID );

  /**
   * \brief Create render pass function
   */
  VOID CreateRenderPass( VOID );

  /**
   * \brief Create default graphics pipeline function
   */
  VOID CreateDefaultGraphicsPipeline( VOID );

  /**
   * \brief Create descriptor pool and allocate descriptor sets function.
   */
  VOID CreateDescriptorPoolAndAllocateSets( VOID );

  /**
   * \brief Write descriptor sets function.
   */
  VOID WriteDescriptorSets( VOID );

  /**
   * \brief Evaluate and print FPS function
   * \param[in] Time Current time
   */
  VOID DetectFPS( FLT Time );

  /** Old fps evaluation time */
  FLT OldFPSEvaluationTime = -1;

  /** Number of frames in current second */
  UINT64 NumberOfFrames = 0;

  /** Semaphore wich will signaled after render and wait before presentation */
  semaphore RenderToPresentationSemaphore;

  /** Semaphore handle wich will signaled after render and wait before presentation */
  VkSemaphore RenderToPresentationSemaphoreId;

  /** Fence for waiting next image */
  fence NextImageFence;

  /** Fence for waiting rendering and initializing */
  fence RenderFence;

  /** Queue for graphics */
  queue GraphicsQueue;

  /** Queue for presentation */
  queue PresentationQueue;

  /** Clear color */
  VkClearValue ClearColor = {0.3, 0.5, 0.7};

  /** Get next image timeout (in ns) before recreation swapchain */
  UINT64 NextImageTimeout = 200000;

  /** Graphics command pool */
  command_pool GraphicsCommandPool;

  /** Graphics command pool for secondary buffers */
  command_pool SecondaryGraphicsCommandPool;

  /** Presentation swapchain */
  swapchain Swapchain;

  /** Swapchain handle */
  VkSwapchainKHR SwapchainId;

  /** Default descriptor set layout */
  descriptor_set_layout DefaultDescriptorSetLayout;

  /** Number of samples */
  VkSampleCountFlagBits NumberOfSamples = VK_SAMPLE_COUNT_1_BIT;

  /** Default vertex shader */
  shader_module DefaultVertexShader;

  /** Default fragment shader */
  shader_module DefaultFragmentShader;

  /** Surface */
  const glfw_surface &Surface;

  /** Surface size */
  VkExtent2D SurfaceSize = {};

  /** Command buffers for every framebuffer */
  std::vector<VkCommandBuffer> DrawCommandBuffers;

  /** Present informations for every framebuffer */
  std::vector<VkPresentInfoKHR> PresentInfos;

  /** Submit informations for every framebuffer */
  std::vector<VkSubmitInfo> SubmitInfos;

  /** Image indices */
  std::vector<UINT32> ImageIndices;

  /** Presentation result */
  VkResult PresentResult = VK_ERROR_UNKNOWN;

  /** Secondary buffers pool */
  std::vector<VkCommandBuffer> UnusedSecondaryCommandBuffers;

  /** Draw elements set */
  std::set<draw_element *> DrawElements;

  /** Default descriptor pool */
  descriptor_pool DefaultDescriptorPool;

  /** Secondary command buffers for draw */
  std::vector<VkCommandBuffer> SecondaryCommandBuffersVector;

  /** Depth buffer */
  image DepthBuffer;

  /** Depth buffer view */
  image_view DepthBufferView;

  /** Memory for depth buffer */
  memory DepthBufferMemory;

  /** Depth buffer format */
  VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT;

  /** Copy of uniform buffer */
  uniform_buffer UniformBufferData;
};

#endif /* __render_h_ */
