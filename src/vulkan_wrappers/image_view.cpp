#include "image_view.h"
#include "vulkan_validation.h"

/**
 * \brief Image view constructor
 * \param[in] Device Device identifier
 * \param[in] CreateInfo Image view create information
 */
image_view::image_view( VkDevice Device, const VkImageViewCreateInfo &CreateInfo ) :
  DeviceId(Device)
{
  vulkan_validation::Check(
    vkCreateImageView(DeviceId, &CreateInfo, nullptr, &ImageViewId),
    "image view creation failed");
}

/**
 * \brief Image view destructor
 */
image_view::~image_view( VOID )
{
  if (ImageViewId != VK_NULL_HANDLE)
    vkDestroyImageView(DeviceId, ImageViewId, nullptr);
}

/**
 * \brief Get image view identifier function
 * \return Image view identifier
 */
VkImageView image_view::GetImageViewId( VOID ) const
{
  return ImageViewId;
}

/**
 * \brief Move function
 * \param[in] ImageView Vulkan image view
 * \return Reference to this
 */
image_view & image_view::operator=( image_view &&ImageView ) noexcept
{
  std::swap(ImageViewId, ImageView.ImageViewId);
  std::swap(DeviceId, ImageView.DeviceId);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] ImageView Vulkan buffer
 */
image_view::image_view( image_view &&ImageView ) noexcept
{
  std::swap(ImageViewId, ImageView.ImageViewId);
  std::swap(DeviceId, ImageView.DeviceId);
}
