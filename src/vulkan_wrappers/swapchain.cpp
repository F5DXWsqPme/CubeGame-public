#include <stdexcept>

#include "swapchain.h"
#include "vulkan_validation.h"

/**
 * \brief Swapchain constructor
 * \param[in] Device Device identifier
 * \param[in] Format Surface format
 * \param[in] Surface Vulkan surface
 * \param[in] RenderPass Render pass
 * \param[in] Size Surface size
 */
swapchain::swapchain( VkDevice Device, VkSurfaceFormatKHR Format, VkSurfaceKHR Surface, VkRenderPass RenderPass,
                      const VkExtent2D &Size, VkImageView DepthAttachment ) :
  SurfaceFormat(Format), DeviceId(Device), SurfaceId(Surface), RenderPassId(RenderPass), DepthAttachment(DepthAttachment)
{
  Resize(Size);
}

/**
 * \brief Resize swapchain function
 * \param Size New size
 */
VOID swapchain::Resize( const VkExtent2D &Size )
{
  for (VkFramebuffer Framebuffer : Framebuffers)
    if (Framebuffer != VK_NULL_HANDLE)
      vkDestroyFramebuffer(DeviceId, Framebuffer, nullptr);

  VkSwapchainCreateInfoKHR CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  CreateInfo.pNext = nullptr;
  CreateInfo.flags = 0;
  CreateInfo.surface = SurfaceId;
  CreateInfo.minImageCount = 3; // TODO: check triple buffering is available
  CreateInfo.imageFormat = SurfaceFormat.format;
  CreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
  CreateInfo.imageExtent = Size;
  CreateInfo.imageArrayLayers = 1;
  CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  CreateInfo.queueFamilyIndexCount = 0;
  CreateInfo.pQueueFamilyIndices = nullptr;
  CreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  CreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR; // TODO: check triple buffering is available
  CreateInfo.clipped = VK_TRUE;
  CreateInfo.oldSwapchain = SwapchainId;

  vulkan_validation::Check(
    vkCreateSwapchainKHR(DeviceId, &CreateInfo, nullptr, &SwapchainId),
    "swapchain creation failed");

  UINT32 NumberOfImages = 0;

  vulkan_validation::Check(
    vkGetSwapchainImagesKHR(DeviceId, SwapchainId, &NumberOfImages, nullptr),
    "getting number of swapchain images failed");

  Images.resize(NumberOfImages);

  vulkan_validation::Check(
    vkGetSwapchainImagesKHR(DeviceId, SwapchainId, &NumberOfImages, Images.data()),
    "getting swapchain images failed");

  CreateImageViews();
  CreateFramebuffers(Size);
}

/**
 * \brief Get image function
 * \param ImageIndex Index of image
 * \return Image by index
 */
VkImage swapchain::GetImageId( UINT32 ImageIndex ) const
{
  return Images[ImageIndex];
}

/**
 * \brief Get framebuffer function
 * \param FramebufferIndex Index of framebuffer
 * \return Framebuffer by index
 */
VkFramebuffer swapchain::GetFramebufferId( UINT32 FramebufferIndex ) const
{
  return Framebuffers[FramebufferIndex];
}

/**
 * \brief Create image views function
 */
VOID swapchain::CreateImageViews( VOID )
{
  VkImageViewCreateInfo CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  CreateInfo.pNext = nullptr;
  CreateInfo.flags = 0;
  CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  CreateInfo.format = SurfaceFormat.format;
  CreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  CreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  CreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  CreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  CreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  CreateInfo.subresourceRange.baseMipLevel = 0;
  CreateInfo.subresourceRange.levelCount = 1;
  CreateInfo.subresourceRange.baseArrayLayer = 0;
  CreateInfo.subresourceRange.layerCount = 1;

  ImageViews.clear();
  ImageViews.reserve(Images.size());

  for (VkImage Image : Images)
  {
    CreateInfo.image = Image;

    //VkImageView ImageView = VK_NULL_HANDLE;
    //
    //vulkan_validation::Check(
    //  vkCreateImageView(DeviceId, &CreateInfo, nullptr, &ImageView),
    //  "image view creation failed");

    ImageViews.push_back(image_view(DeviceId, CreateInfo));
  }
}

/**
 * \brief Get number of framebuffers function
 * \return Number of framebuffers in swapchain
 */
UINT32 swapchain::GetNumberOfFramebuffers( VOID ) const
{
  return Framebuffers.size();
}

/**
 * \brief Create framebuffers function
 * \param[in] Size Surface size
 */
VOID swapchain::CreateFramebuffers( const VkExtent2D &Size )
{
  VkFramebufferCreateInfo CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  CreateInfo.pNext = nullptr;
  CreateInfo.flags = 0;
  CreateInfo.renderPass = RenderPassId;
  CreateInfo.width = Size.width;
  CreateInfo.height = Size.height;
  CreateInfo.layers = 1;

  Framebuffers.clear();
  Framebuffers.reserve(ImageViews.size());

  for (const image_view &ImageView : ImageViews)
  {
    VkImageView Attachments[] = {ImageView.GetImageViewId(), DepthAttachment};

    if (DepthAttachment == VK_NULL_HANDLE)
    {
      CreateInfo.pAttachments = Attachments;
      CreateInfo.attachmentCount = 1;
    }
    else
    {
      CreateInfo.pAttachments = Attachments;
      CreateInfo.attachmentCount = 2;
    }
    
    VkFramebuffer Framebuffer = VK_NULL_HANDLE;

    vulkan_validation::Check(
      vkCreateFramebuffer(DeviceId, &CreateInfo, nullptr, &Framebuffer),
      "framebuffer creation failed");

    Framebuffers.push_back(Framebuffer);
  }
}

/**
 * \brief Swapchain destructor
 */
swapchain::~swapchain( VOID )
{
  if (SwapchainId != VK_NULL_HANDLE)
  {
    vkDestroySwapchainKHR(DeviceId, SwapchainId, nullptr);

    for (VkFramebuffer Framebuffer : Framebuffers)
      if (Framebuffer != VK_NULL_HANDLE)
        vkDestroyFramebuffer(DeviceId, Framebuffer, nullptr);
  }
}

/**
 * \brief Get swapchain identifier function
 * \return Swapchain identifier
 */
VkSwapchainKHR swapchain::GetSwapchainId( VOID ) const
{
  return SwapchainId;
}

/**
 * \brief Get surface format function
 * \return Surface format
 */
VkSurfaceFormatKHR swapchain::GetSurfaceFormat( VOID ) const noexcept
{
  return SurfaceFormat;
}

/**
 * \brief Move function
 * \param[in] Swapchain Swapchain
 * \return Reference to this
 */
swapchain & swapchain::operator=( swapchain &&Swapchain ) noexcept
{
  std::swap(SurfaceFormat, Swapchain.SurfaceFormat);
  std::swap(SwapchainId, Swapchain.SwapchainId);
  std::swap(DeviceId, Swapchain.DeviceId);
  std::swap(SurfaceId, Swapchain.SurfaceId);
  std::swap(RenderPassId, Swapchain.RenderPassId);
  std::swap(Images, Swapchain.Images);
  std::swap(ImageViews, Swapchain.ImageViews);
  std::swap(Framebuffers, Swapchain.Framebuffers);
  std::swap(DepthAttachment, Swapchain.DepthAttachment);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] Swapchain Swapchain
 */
swapchain::swapchain( swapchain &&Swapchain ) noexcept
{
  std::swap(SurfaceFormat, Swapchain.SurfaceFormat);
  std::swap(SwapchainId, Swapchain.SwapchainId);
  std::swap(DeviceId, Swapchain.DeviceId);
  std::swap(SurfaceId, Swapchain.SurfaceId);
  std::swap(RenderPassId, Swapchain.RenderPassId);
  std::swap(Images, Swapchain.Images);
  std::swap(ImageViews, Swapchain.ImageViews);
  std::swap(Framebuffers, Swapchain.Framebuffers);
  std::swap(DepthAttachment, Swapchain.DepthAttachment);
}