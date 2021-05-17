#ifndef __swapchain_h_
#define __swapchain_h_

#include <vector>

#include "image_view.h"

/**
 * \brief Swapchain class
 */
class swapchain
{
public:
  /**
   * \brief Default constructor.
   */
  swapchain( VOID ) = default;

  /**
   * \brief Swapchain constructor
   * \param[in] Device Device identifier
   * \param[in] Format Surface format
   * \param[in] Surface Vulkan surface
   * \param[in] RenderPass Render pass
   * \param[in] Size Surface size
   * \param[in] DepthAttachment Additional attachment
   */
  swapchain( VkDevice Device, VkSurfaceFormatKHR Format, VkSurfaceKHR Surface, VkRenderPass RenderPass, const VkExtent2D &Size,
             VkImageView DepthAttachment = VK_NULL_HANDLE );

  /**
   * \brief Resize swapchain function
   * \param Size New size
   */
  VOID Resize( const VkExtent2D &Size );

  /**
   * \brief Swapchain destructor
   */
  ~swapchain( VOID );

  /**
   * \brief Get swapchain identifier function
   * \return Swapchain identifier
   */
  VkSwapchainKHR GetSwapchainId( VOID ) const;

  /**
   * \brief Move function
   * \param[in] Swapchain Swapchain
   * \return Reference to this
   */
  swapchain & operator=( swapchain &&Swapchain ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] Swapchain Swapchain
   */
  swapchain( swapchain &&Swapchain ) noexcept;

  /**
   * \brief Get framebuffer function
   * \param FramebufferIndex Index of framebuffer
   * \return Framebuffer by index
   */
  VkFramebuffer GetFramebufferId( UINT32 FramebufferIndex ) const;

  /**
   * \brief Get number of framebuffers function
   * \return Number of framebuffers in swapchain
   */
  UINT32 GetNumberOfFramebuffers( VOID ) const;

  /**
   * \brief Get image function
   * \param ImageIndex Index of image
   * \return Image by index
   */
  VkImage GetImageId( UINT32 ImageIndex ) const;

  /**
   * \brief Get surface format function
   * \return Surface format
   */
  VkSurfaceFormatKHR GetSurfaceFormat( VOID ) const noexcept;

private:
  /**
   * \brief Create image views function
   */
  VOID CreateImageViews( VOID );

  /**
   * \brief Create framebuffers function
   * \param[in] Size Surface size
   */
  VOID CreateFramebuffers( const VkExtent2D &Size );

  /**
   * \brief Removed copy function
   * \param[in] Swapchain Swapchain
   * \return Reference to this
   */
  swapchain & operator=( const swapchain &Swapchain ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] Swapchain Swapchain
   */
  swapchain( const swapchain &Swapchain ) = delete;

  /** Format of surface */
  VkSurfaceFormatKHR SurfaceFormat = {};

  /** Swapchain identifier */
  VkSwapchainKHR SwapchainId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;

  /** Surface identifier */
  VkSurfaceKHR SurfaceId = VK_NULL_HANDLE;

  /** Render pass identifier */
  VkRenderPass RenderPassId = VK_NULL_HANDLE;

  /** Swapchain images */
  std::vector<VkImage> Images;

  /** Swapchain images views */
  std::vector<image_view> ImageViews;

  /** Swapchain framebuffers */
  std::vector<VkFramebuffer> Framebuffers;

  /** Additional attachment */
  VkImageView DepthAttachment = VK_NULL_HANDLE;
};

#endif /* __swapchain_h_ */
