#ifndef __glfw_surface_h_
#define __glfw_surface_h_

#include "ext/volk/volk.h"
#include "ext/glfw/include/glfw/glfw3.h"

#include "def.h"

/**
 * \brief GLFW surface
 */
class glfw_surface
{
public:
  /**
   * \brief Default constructor.
   */
  glfw_surface( VOID ) = default;

  /**
   * \brief Surface constructor
   * \param[in] PhysicalDevice Physical device identifier
   * \param[in] Instance Vulkan instance.
   * \param[in] WindowPtr Pointer to window structure
   */
  glfw_surface( VkPhysicalDevice PhysicalDevice, VkInstance Instance, GLFWwindow *WindowPtr );

  /**
   * \brief Surface destructor
   */
  ~glfw_surface( VOID );

  /**
   * \brief Get surface identifier function
   * \return Fullscreen surface identifier
   */
  VkSurfaceKHR GetSurfaceId( VOID ) const noexcept;

  /**
   * \brief Get surface size function
   * \return Surface size
   */
  VkExtent2D GetSurfaceSize( VOID ) const;

  /**
   * \brief Move function
   * \param[in] Surface Surface
   * \return Reference to this
   */
  glfw_surface & operator=( glfw_surface &&Surface ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] Surface Surface
   */
  glfw_surface( glfw_surface &&Surface ) noexcept;

  /**
   * \brief Get surface format function
   * \return Surface format
   */
  VkSurfaceFormatKHR GetSurfaceFormat( VOID ) const noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] Surface Surface
   * \return Reference to this
   */
  glfw_surface & operator=( const glfw_surface &Surface ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] Surface Surface
   */
  glfw_surface( const glfw_surface &Surface ) = delete;

  /** Format of surface */
  VkSurfaceFormatKHR SurfaceFormat = {};

  /** Surface identifier */
  VkSurfaceKHR SurfaceId = VK_NULL_HANDLE;

  /** Instance identifier */
  VkInstance InstanceId = VK_NULL_HANDLE;

  /** Physical device identifier */
  VkPhysicalDevice PhysicalDeviceId = VK_NULL_HANDLE;
};

#endif /* __glfw_surface_h_ */
