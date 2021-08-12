#ifndef __settings_h_
#define __settings_h_

#include "def.h"

/**
 * \brief Settings class
 */
class settings
{
public:
  /** Enable ray tracing lighting flag */
  BOOL EnableRayTracedLighting = TRUE;

  /** Enable Vulkan GLFW surface flag */
  BOOL EnableVulkanGLFWSurface = TRUE;

  /** Enable Vulkan validation layer flag */
  BOOL EnableVulkanValidationLayer = TRUE;

  /** Enable Vulkan printf extension flag */
  BOOL EnableVulkanPrintfExtension = FALSE;

  /** Enable Vulkan fullscreen surface extension flag */
  BOOL EnableVulkanFullscreenSurfaceExtension = FALSE;
};

#endif /* __settings_h_ */
