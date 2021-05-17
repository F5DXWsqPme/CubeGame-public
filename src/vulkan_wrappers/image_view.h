#ifndef __image_view_h_
#define __image_view_h_

#include "memory.h"

/**
 * \brief Image view class
 */
class image_view
{
public:
  /**
   * \brief Default constructor.
   */
  image_view( VOID ) = default;

  /**
   * \brief Image view constructor
   * \param[in] Device Device identifier
   * \param[in] CreateInfo Image view create information
   */
  image_view( VkDevice Device, const VkImageViewCreateInfo &CreateInfo );

  /**
   * \brief Image view destructor
   */
  ~image_view( VOID );

  /**
   * \brief Get image view identifier function
   * \return Image view identifier
   */
  VkImageView GetImageViewId( VOID ) const;

  /**
   * \brief Move function
   * \param[in] ImageView Vulkan image view
   * \return Reference to this
   */
  image_view & operator=( image_view &&ImageView ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] ImageView Vulkan buffer
   */
  image_view( image_view &&ImageView ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] ImageView Vulkan image view
   * \return Reference to this
   */
  image_view & operator=( const image_view &ImageView ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] ImageView Vulkan image view
   */
  image_view( const image_view &ImageView ) = delete;

  /** Image view identifier */
  VkImageView ImageViewId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /** __image_view_h_ */
