#include "image.h"
#include "vulkan_validation.h"

/**
 * \brief Image constructor
 * \param[in] Device Device identifier
 * \param[in] CreateInfo Image create information
 */
image::image( VkDevice Device, const VkImageCreateInfo &CreateInfo ) :
  DeviceId(Device)
{
  vulkan_validation::Check(
    vkCreateImage(DeviceId, &CreateInfo, nullptr, &ImageId),
    "image creation failed");
}

/**
 * \brief Image destructor
 */
image::~image( VOID )
{
  if (ImageId != VK_NULL_HANDLE)
    vkDestroyImage(DeviceId, ImageId, nullptr);
}

/**
 * \brief Bind memory to image function
 * \param[in] Mem Memory for bind
 * \param[in] Offset Memory offset
 */
VOID image::BindMemory( const memory &Mem, const VkDeviceSize Offset ) const
{
  vulkan_validation::Check(
    vkBindImageMemory(DeviceId, ImageId, Mem.GetMemoryId(), Offset),
    "bind image memory failed");
}

/**
 * \brief Get memory requirements function
 * \return Memory requirements
 */
VkMemoryRequirements image::GetMemoryRequirements( VOID ) const
{
  VkMemoryRequirements MemoryRequirements = {};

  vkGetImageMemoryRequirements(DeviceId, ImageId, &MemoryRequirements);

  return MemoryRequirements;
}

/**
 * \brief Get image identifier function
 * \return Image identifier
 */
VkImage image::GetImageId( VOID ) const noexcept
{
  return ImageId;
}

/**
 * \brief Move function
 * \param[in] Image Vulkan image
 * \return Reference to this
 */
image & image::operator=( image &&Image ) noexcept
{
  std::swap(ImageId, Image.ImageId);
  std::swap(DeviceId, Image.DeviceId);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] Image Vulkan buffer
 */
image::image( image &&Image ) noexcept
{
  std::swap(ImageId, Image.ImageId);
  std::swap(DeviceId, Image.DeviceId);
}
