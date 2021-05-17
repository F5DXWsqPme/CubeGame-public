#ifndef __fullscreen_surface_h_
#define __fullscreen_surface_h_

#include "ext/volk/volk.h"

#include "def.h"

/**
 * \brief Fullscreen surface class
 */
class fullscreen_surface
{
public:
  /**
   * \brief Default constructor.
   */
  fullscreen_surface( VOID ) = default;

  /**
   * \brief Fullscreen surface constructor
   * \param[in] PhysicalDevice Physical device identifier
   * \parram[in] Instance Vulkan instance.
   */
  fullscreen_surface( VkPhysicalDevice PhysicalDevice, VkInstance Instance );

  /**
   * \brief Fullscreen surface destructor
   */
  ~fullscreen_surface( VOID );

  /**
   * \brief Get surface identifier function
   * \return Fullscreen surface identifier
   */
  VkSurfaceKHR GetSurfaceId( VOID ) const noexcept;

  /**
   * \brief Get surface size function
   * \return Surface size
   */
  VkExtent2D GetSurfaceSize( VOID ) const noexcept;

  /**
   * \brief Move function
   * \param[in] FullscreenSurface Surface
   * \return Reference to this
   */
  fullscreen_surface & operator=( fullscreen_surface &&FullscreenSurface ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] FullscreenSurface Surface
   */
  fullscreen_surface( fullscreen_surface &&FullscreenSurface ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] FullscreenSurface Surface
   * \return Reference to this
   */
  fullscreen_surface & operator=( const fullscreen_surface &FullscreenSurface ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] FullscreenSurface Surface
   */
  fullscreen_surface( const fullscreen_surface &FullscreenSurface ) = delete;

  /** Surface identifier */
  VkSurfaceKHR SurfaceId = VK_NULL_HANDLE;

  /** Instance identifier */
  VkInstance InstanceId = VK_NULL_HANDLE;

  /** Surface size */
  VkExtent2D SurfaceSize = {};
};

#endif /* __fullscreen_surface_h_ */
