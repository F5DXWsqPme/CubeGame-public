#include <vector>
#include <iostream>

#include "fullscreen_surface.h"
#include "vulkan_validation.h"

/**
 * \brief Fullscreen surface constructor
 * \param[in] PhysicalDevice Physical device identifier
 * \parram[in] Instance Vulkan instance.
 */
fullscreen_surface::fullscreen_surface( VkPhysicalDevice PhysicalDevice, VkInstance Instance ) :
  InstanceId(Instance)
{
  UINT32 NumberOfDisplayProperties = 0;

  vulkan_validation::Check(
    vkGetPhysicalDeviceDisplayPropertiesKHR(PhysicalDevice, &NumberOfDisplayProperties, nullptr),
    "getting number of display properties failed");

  std::vector<VkDisplayPropertiesKHR> DisplayProperties(NumberOfDisplayProperties);

  vulkan_validation::Check(
    vkGetPhysicalDeviceDisplayPropertiesKHR(PhysicalDevice, &NumberOfDisplayProperties, DisplayProperties.data()),
    "getting display properties failed");

  UINT32 SelectedDisplayNumber = 0;
  VkDisplayKHR SelectedDisplay = DisplayProperties[SelectedDisplayNumber].display;

  std::cout << "Selected display: " << DisplayProperties[SelectedDisplayNumber].displayName;

  UINT32 NumberOfDisplayModeProperties = 0;

  vulkan_validation::Check(
    vkGetDisplayModePropertiesKHR(PhysicalDevice, SelectedDisplay, &NumberOfDisplayModeProperties, nullptr),
    "getting number of display mode properties failed");

  std::vector<VkDisplayModePropertiesKHR> DisplayModeProperties(NumberOfDisplayModeProperties);

  vulkan_validation::Check(
    vkGetDisplayModePropertiesKHR(PhysicalDevice, SelectedDisplay, &NumberOfDisplayModeProperties, DisplayModeProperties.data()),
    "getting display mode properties failed");

  UINT32 SelectedDisplayModeNumber = 0;
  VkDisplayModeKHR SelectedDisplayMode = DisplayModeProperties[SelectedDisplayModeNumber].displayMode;

  SurfaceSize = DisplayModeProperties[SelectedDisplayModeNumber].parameters.visibleRegion;

  VkDisplaySurfaceCreateInfoKHR CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
  CreateInfo.pNext = nullptr;
  CreateInfo.flags = 0;
  CreateInfo.displayMode = SelectedDisplayMode;
  CreateInfo.planeIndex = 0;
  CreateInfo.planeStackIndex = 0;
  CreateInfo.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  CreateInfo.globalAlpha = 1;
  CreateInfo.alphaMode = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR;
  CreateInfo.imageExtent = SurfaceSize;

  vulkan_validation::Check(
    vkCreateDisplayPlaneSurfaceKHR(InstanceId, &CreateInfo, nullptr, &SurfaceId),
    "fullscreen surface creation failed");
}

/**
 * \brief Fullscreen surface destructor
 */
fullscreen_surface::~fullscreen_surface( VOID )
{
  if (SurfaceId != VK_NULL_HANDLE)
    vkDestroySurfaceKHR(InstanceId, SurfaceId, nullptr);
}

/**
 * \brief Get surface identifier function
 * \return Fullscreen surface identifier
 */
VkSurfaceKHR fullscreen_surface::GetSurfaceId( VOID ) const noexcept
{
  return SurfaceId;
}

/**
 * \brief Get surface size function
 * \return Surface size
 */
VkExtent2D fullscreen_surface::GetSurfaceSize( VOID ) const noexcept
{
  return SurfaceSize;
}

/**
 * \brief Move function
 * \param[in] FullscreenSurface Surface
 * \return Reference to this
 */
fullscreen_surface & fullscreen_surface::operator=( fullscreen_surface &&FullscreenSurface ) noexcept
{
  std::swap(InstanceId, FullscreenSurface.InstanceId);
  std::swap(SurfaceId, FullscreenSurface.SurfaceId);
  std::swap(SurfaceSize, FullscreenSurface.SurfaceSize);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] FullscreenSurface Surface
 */
fullscreen_surface::fullscreen_surface( fullscreen_surface &&FullscreenSurface ) noexcept
{
  std::swap(InstanceId, FullscreenSurface.InstanceId);
  std::swap(SurfaceId, FullscreenSurface.SurfaceId);
  std::swap(SurfaceSize, FullscreenSurface.SurfaceSize);
}
