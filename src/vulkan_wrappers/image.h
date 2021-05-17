#ifndef __image_h_
#define __image_h_

#include "memory.h"

/**
 * \brief Image class
 */
class image
{
public:
  /**
   * \brief Default constructor.
   */
  image( VOID ) = default;

  /**
   * \brief Image constructor
   * \param[in] Device Device identifier
   * \param[in] CreateInfo Image create information
   */
  image( VkDevice Device, const VkImageCreateInfo &CreateInfo );

  /**
   * \brief Image destructor
   */
  ~image( VOID );

  /**
   * \brief Bind memory to image function
   * \param[in] Mem Memory for bind
   * \param[in] Offset Memory offset
   */
  VOID BindMemory( const memory &Mem, const VkDeviceSize Offset ) const;

  /**
   * \brief Get memory requirements function
   * \return Memory requirements
   */
  VkMemoryRequirements GetMemoryRequirements( VOID ) const;

  /**
   * \brief Get image identifier function
   * \return Image identifier
   */
  VkImage GetImageId( VOID ) const noexcept;

  /**
   * \brief Move function
   * \param[in] Image Vulkan image
   * \return Reference to this
   */
  image & operator=( image &&Image ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] Image Vulkan buffer
   */
  image( image &&Image ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] Image Vulkan image
   * \return Reference to this
   */
  image & operator=( const image &Image ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] Image Vulkan image
   */
  image( const image &Image ) = delete;

  /** Image identifier */
  VkImage ImageId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};


#endif /** __image_h_ */
