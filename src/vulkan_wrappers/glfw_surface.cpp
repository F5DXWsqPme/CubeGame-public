#include <vector>
#include <stdexcept>

#include "glfw_surface.h"
#include "vulkan_validation.h"

/**
 * \brief Surface constructor
 * \param[in] PhysicalDevice Physical device identifier
 * \param[in] Instance Vulkan instance.
 * \param[in] WindowPtr Pointer to window structure
 */
glfw_surface::glfw_surface( VkPhysicalDevice PhysicalDevice, VkInstance Instance, GLFWwindow *WindowPtr ) :
  InstanceId(Instance), PhysicalDeviceId(PhysicalDevice)
{
  vulkan_validation::Check(
    glfwCreateWindowSurface(InstanceId, WindowPtr, nullptr, &SurfaceId),
    "vulkan surface for GLFW window creation failed");

  UINT32 NumberOfSurfaceFormats = 0;

  vulkan_validation::Check(
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, SurfaceId, &NumberOfSurfaceFormats, nullptr),
    "getting number of surface formats failed");

  std::vector<VkSurfaceFormatKHR> Formats(NumberOfSurfaceFormats);

  vulkan_validation::Check(
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, SurfaceId, &NumberOfSurfaceFormats, Formats.data()),
    "getting surface formats failed");

  if (NumberOfSurfaceFormats < 1)
    throw std::runtime_error("not exists supported surface formats");

  SurfaceFormat = Formats[0];
}

/**
 * \brief Surface destructor
 */
glfw_surface::~glfw_surface( VOID )
{
  if (SurfaceId != VK_NULL_HANDLE)
    vkDestroySurfaceKHR(InstanceId, SurfaceId, nullptr);
}

/**
 * \brief Get surface identifier function
 * \return Fullscreen surface identifier
 */
VkSurfaceKHR glfw_surface::GetSurfaceId( VOID ) const noexcept
{
  return SurfaceId;
}

/**
 * \brief Get surface size function
 * \return Surface size
 */
VkExtent2D glfw_surface::GetSurfaceSize( VOID ) const
{
  VkSurfaceCapabilitiesKHR SurfaceCapabilities;

  vulkan_validation::Check(
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDeviceId, SurfaceId, &SurfaceCapabilities),
    "surface size request failed");

  return SurfaceCapabilities.currentExtent;
}

/**
 * \brief Get surface format function
 * \return Surface format
 */
VkSurfaceFormatKHR glfw_surface::GetSurfaceFormat( VOID ) const noexcept
{
  return SurfaceFormat;
}

/**
 * \brief Move function
 * \param[in] Surface Surface
 * \return Reference to this
 */
glfw_surface & glfw_surface::operator=( glfw_surface &&Surface ) noexcept
{
  std::swap(SurfaceFormat, Surface.SurfaceFormat);
  std::swap(InstanceId, Surface.InstanceId);
  std::swap(SurfaceId, Surface.SurfaceId);
  std::swap(PhysicalDeviceId, Surface.PhysicalDeviceId);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] Surface Surface
 */
glfw_surface::glfw_surface( glfw_surface &&Surface ) noexcept
{
  std::swap(SurfaceFormat, Surface.SurfaceFormat);
  std::swap(InstanceId, Surface.InstanceId);
  std::swap(SurfaceId, Surface.SurfaceId);
  std::swap(PhysicalDeviceId, Surface.PhysicalDeviceId);
}
