#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>

#include "def.h"

#if ENABLE_VULKAN_GLFW_SURFACE
#include "ext/volk/volk.h"
#include "ext/glfw/include/GLFW/glfw3.h"

#include "glfw_surface.h"
#include "render/glfw_window.h"
#endif /* ENABLE_VULKAN_GLFW_SURFACE */

#if ENABLE_VULKAN_FULLSCREEN_SURFACE_EXTENSION
#include "fullscreen_surface.h"
#endif /* ENABLE_VULKAN_GLFW_SURFACE */

#include "vulkan_application.h"
#include "vulkan_validation.h"

/**
 * \brief Class constructor
 */
vulkan_application::vulkan_application( VOID )
{
  Init();
}

/**
 * \brief Initialize vulkan application structure
 * \param[in] SelectedPhysicalDeviceId Physical device index
 * \param[in] Surface Surface for presentation
 */
VOID vulkan_application::InitApplication( UINT SelectedPhysicalDeviceId, VkSurfaceKHR Surface )
{
  SelectedPhysicalDevice = SelectedPhysicalDeviceId;

  vkGetPhysicalDeviceMemoryProperties(PhysicalDevices[*SelectedPhysicalDevice], &DeviceMemoryProperties);
  //SelectMemoryTypes();
  SelectQueueIndices(Surface);
  CreateLogicalDevice();
}

/**
 * \brief Class destructor
 */
vulkan_application::~vulkan_application( VOID )
{
#if ENABLE_VULKAN_VALIDATION_LAYER
  if (DebugMessenger != VK_NULL_HANDLE)
    vkDestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
#endif /* ENABLE_VULKAN_VALIDATION_LAYER */

  if (Device != VK_NULL_HANDLE)
    vkDestroyDevice(Device, nullptr);
  if (Instance != VK_NULL_HANDLE)
    vkDestroyInstance(Instance, nullptr);
}

VkDevice vulkan_application::GetDeviceId( VOID ) const
{
  return Device;
}

/**
 * \brief Vulkan application initialization function
 * \return VK_SUCCESS-if success
 */
VOID vulkan_application::Init( VOID )
{
  vulkan_validation::Check(
    volkInitialize(),
    "vulkan loading failed (volk initialization)");

  // Main application information
  VkApplicationInfo AppInfo = {};

  AppInfo.sType =  VK_STRUCTURE_TYPE_APPLICATION_INFO;
  AppInfo.pApplicationName = "Vulkan_application";
  AppInfo.applicationVersion = 1;
  AppInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

  // Instance creating
  VkInstanceCreateInfo InstanceCreateInfo = {};

  InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  InstanceCreateInfo.pApplicationInfo = &AppInfo;

#if ENABLE_VULKAN_VALIDATION_LAYER
  const CHAR *ValidationLayerName = "VK_LAYER_KHRONOS_validation";

  InstanceCreateInfo.enabledLayerCount = 1;
  InstanceCreateInfo.ppEnabledLayerNames = &ValidationLayerName;
#endif

  {
    UINT32 NumberOfSupportedExtensions = 0;

    vulkan_validation::Check(
      vkEnumerateInstanceExtensionProperties(nullptr, &NumberOfSupportedExtensions, nullptr),
      "number of supported instance extensions request failed");

    std::vector<VkExtensionProperties> SupportedExtensions(NumberOfSupportedExtensions);

    vulkan_validation::Check(
      vkEnumerateInstanceExtensionProperties(nullptr, &NumberOfSupportedExtensions, SupportedExtensions.data()),
      "supported instance extensions request failed");

    std::cout << "Supported instance extensions:\n";

    for (const VkExtensionProperties &Extension : SupportedExtensions)
      std::cout << "  " << Extension.extensionName << " (" << Extension.specVersion << ")\n";

    std::cout << std::endl;
  }

  std::vector<const CHAR *> ExtensionsNames;

#if ENABLE_VULKAN_FULLSCREEN_SURFACE_EXTENSION
  {
    const CHAR *SurfaceExtensionName = "VK_KHR_surface";
    const CHAR *DisplayExtensionName = "VK_KHR_display";
    //const CHAR *SwapchainExtensionName = "VK_KHR_swapchain";
    //const CHAR *DisplaySwapchainExtensionName = "VK_KHR_display_swapchain";

    ExtensionsNames.push_back(SurfaceExtensionName);
    ExtensionsNames.push_back(DisplayExtensionName);
    //ExtensionsNames.push_back(SwapchainExtensionName);
    //ExtensionsNames.push_back(DisplaySwapchainExtensionName);
  }
#endif /* ENABLE_VULKAN_FULLSCREEN_SURFACE_EXTENSION */

#if ENABLE_VULKAN_GLFW_SURFACE
  {
    UINT32 NumberOfGLFWExtensions = 0;
    const CHAR **Extensions = glfwGetRequiredInstanceExtensions(&NumberOfGLFWExtensions);

    for (UINT32 i = 0; i < NumberOfGLFWExtensions; i++)
      ExtensionsNames.push_back(Extensions[i]);
  }
#endif /* ENABLE_VULKAN_GLFW_SURFACE */

#if ENABLE_VULKAN_VALIDATION_LAYER
  ExtensionsNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  std::cout << "Required instance extensions:\n";

  for (const CHAR *ExtensionName : ExtensionsNames)
    std::cout << "  " << ExtensionName << "\n";

  std::cout << std::endl;

  InstanceCreateInfo.enabledExtensionCount = ExtensionsNames.size();
  InstanceCreateInfo.ppEnabledExtensionNames = ExtensionsNames.data();

  vulkan_validation::Check(
    vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance),
    "instance creation error");

  if (Instance == nullptr)
    throw std::runtime_error("vkCreateInstance return success code, but instance not created");

  volkLoadInstanceOnly(Instance);

  // Fill physical devices
  UINT32 NumberOfPhysicalDevices = 0;

  vulkan_validation::Check(
    vkEnumeratePhysicalDevices(Instance, &NumberOfPhysicalDevices, nullptr),
    "physical devices enumeration failed");

  PhysicalDevices.resize(NumberOfPhysicalDevices);

  vulkan_validation::Check(
    vkEnumeratePhysicalDevices(Instance, &NumberOfPhysicalDevices, PhysicalDevices.data()),
    "physical devices enumeration failed");

#if ENABLE_VULKAN_VALIDATION_LAYER
  {
    VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo = {};

    DebugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    DebugMessengerCreateInfo.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    DebugMessengerCreateInfo.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    DebugMessengerCreateInfo.pfnUserCallback = DebugCallback;
    DebugMessengerCreateInfo.pUserData = nullptr;

    vulkan_validation::Check(
      vkCreateDebugUtilsMessengerEXT(Instance, &DebugMessengerCreateInfo, nullptr, &DebugMessenger),
      "debug messenger creation failed");
  }
#endif
}

/**
 * \brief Create logical device function
 */
VOID vulkan_application::CreateLogicalDevice( VOID )
{
  //VkPhysicalDeviceFeatures SupportedFeatures;
  //
  //vkGetPhysicalDeviceFeatures(PhysicalDevices[SelectedPhysicalDevice], &SupportedFeatures);

  VkPhysicalDeviceProperties Properties;
  
  vkGetPhysicalDeviceProperties(PhysicalDevices[*SelectedPhysicalDevice], &Properties);
  std::cout << "Selected device: " << Properties.deviceName << "\n" << std::endl;
  
  VkPhysicalDeviceFeatures RequiredFeatures = {};

  std::vector<VkDeviceQueueCreateInfo> DeviceQueueCreateInfosArray;

  std::vector<UINT32> FamilyIndices;

  FamilyIndices.reserve(4);

  if (TransferQueueFamilyIndex)
    FamilyIndices.push_back(*TransferQueueFamilyIndex);

  if (ComputeQueueFamilyIndex)
    FamilyIndices.push_back(*ComputeQueueFamilyIndex);

  if (GraphicsQueueFamilyIndex)
    FamilyIndices.push_back(*TransferQueueFamilyIndex);

  if (PresentationQueueFamilyIndex)
    FamilyIndices.push_back(*TransferQueueFamilyIndex);

  std::sort(FamilyIndices.begin(), FamilyIndices.end());
  FamilyIndices.erase(std::unique(FamilyIndices.begin(), FamilyIndices.end()), FamilyIndices.end());

  FLT QueuePriorities[] = {1};

  VkDeviceQueueCreateInfo DeviceQueueCreateInfo = {};

  DeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  DeviceQueueCreateInfo.pNext = nullptr;
  DeviceQueueCreateInfo.flags = 0;
  DeviceQueueCreateInfo.queueFamilyIndex = *ComputeQueueFamilyIndex;
  DeviceQueueCreateInfo.queueCount = 1;
  DeviceQueueCreateInfo.pQueuePriorities = QueuePriorities;

  for (UINT32 Index : FamilyIndices)
  {
    DeviceQueueCreateInfo.queueFamilyIndex = Index;
    DeviceQueueCreateInfosArray.push_back(DeviceQueueCreateInfo);
  }

  VkDeviceCreateInfo DeviceCreateInfo = {};

  DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  DeviceCreateInfo.queueCreateInfoCount = DeviceQueueCreateInfosArray.size();
  DeviceCreateInfo.pQueueCreateInfos = DeviceQueueCreateInfosArray.data();
  DeviceCreateInfo.pEnabledFeatures = &RequiredFeatures;

  {
    UINT32 NumberOfSupportedExtensions = 0;

    vulkan_validation::Check(
      vkEnumerateDeviceExtensionProperties(PhysicalDevices[*SelectedPhysicalDevice], nullptr, &NumberOfSupportedExtensions, nullptr),
      "number of supported device extensions request failed");

    std::vector<VkExtensionProperties> SupportedExtensions(NumberOfSupportedExtensions);

    vulkan_validation::Check(
      vkEnumerateDeviceExtensionProperties(PhysicalDevices[*SelectedPhysicalDevice], nullptr, &NumberOfSupportedExtensions, SupportedExtensions.data()),
      "supported device extensions request failed");

    std::cout << "Supported device extensions:\n";

    for (const VkExtensionProperties &Extension : SupportedExtensions)
      std::cout << "  " << Extension.extensionName << " (" << Extension.specVersion << ")\n";

    std::cout << std::endl;
  }

  std::vector<const CHAR *> ExtensionsNames;

#if ENABLE_VULKAN_PRINTF_EXTENSION
  {
    const CHAR *PrintfExtensionName = "VK_KHR_shader_non_semantic_info";

    ExtensionsNames.push_back(PrintfExtensionName);
  }
#endif /* ENABLE_VULKAN_PRINTF_EXTENSION */

#if ENABLE_VULKAN_FULLSCREEN_SURFACE_EXTENSION
  {
    const CHAR *SwapchainExtensionName = "VK_KHR_swapchain";

    ExtensionsNames.push_back(SwapchainExtensionName);
  }
#endif /* ENABLE_VULKAN_FULLSCREEN_SURFACE_EXTENSION */

#if ENABLE_VULKAN_GLFW_SURFACE
  {
    const CHAR *SwapchainExtensionName = "VK_KHR_swapchain";

    ExtensionsNames.push_back(SwapchainExtensionName);
  }
#endif /* ENABLE_VULKAN_GLFW_SURFACE */

  DeviceCreateInfo.enabledExtensionCount = ExtensionsNames.size();
  DeviceCreateInfo.ppEnabledExtensionNames = ExtensionsNames.data();

  std::cout << "Required device extensions:\n";

  for (const CHAR *ExtensionName : ExtensionsNames)
    std::cout << "  " << ExtensionName << "\n";

  std::cout << std::endl;

  vulkan_validation::Check(
    vkCreateDevice(PhysicalDevices[*SelectedPhysicalDevice], &DeviceCreateInfo, nullptr, &Device),
    "logical device creation failed");

  if (Device == nullptr)
    throw std::runtime_error("vkCreateDevice return success code, but device not created");

  volkLoadDevice(Device);
}

/**
 * \brief Get physical device function
 * \param[in] Physical device index
 * \return Physical device
 */
VkPhysicalDevice vulkan_application::GetPhysicalDevice( UINT32 DeviceIndex ) const
{
  return PhysicalDevices[DeviceIndex];
}

/**
 * \brief Get vulkan instance identifier function
 * \return Instance
 */
VkInstance vulkan_application::GetInstanceId( VOID ) const
{
  return Instance;
}

/**
 * \brief Debug callback
 * \param[in] MessageSeverity Message severity
 * \param[in] MessageType Type of message
 * \param[in] CallbackData Debug data
 * \param[in, out] UserData User pointer
 * \return We should return VK_FALSE
 */
VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_application::DebugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT MessageType,
  const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
  void* UserData)
{
  std::cout << CallbackData->pMessage << "\n" << std::endl;

  return VK_FALSE;
}

/**
 * \brief Find memory type index with required memory property flags
 * \param[in] MemoryRequirements Memory requirements
 * \param[in] RequiredProperty Required memory property flags
 * \return Memory type index
 */
std::optional<UINT32> vulkan_application::FindMemoryTypeWithFlags( const VkMemoryRequirements MemoryRequirements,
                                                                   VkMemoryPropertyFlags RequiredProperty ) const
{
  for (UINT32 MemoryTypeId = 0; MemoryTypeId < DeviceMemoryProperties.memoryTypeCount; MemoryTypeId++)
  {
    if (MemoryRequirements.memoryTypeBits & (1 << MemoryTypeId))
    {
      const VkMemoryType &Type = DeviceMemoryProperties.memoryTypes[MemoryTypeId];
      const VkMemoryHeap &Heap = DeviceMemoryProperties.memoryHeaps[Type.heapIndex];

      if ((Type.propertyFlags & RequiredProperty) == RequiredProperty &&
          Heap.size >= MemoryRequirements.size)
      {
        return MemoryTypeId;
      }
    }
  }

  return std::nullopt;
}

/**
 * \brief Select memory types.
 */
/*VOID vulkan_application::SelectMemoryTypes( VOID )
{
  vkGetPhysicalDeviceMemoryProperties(PhysicalDevices[*SelectedPhysicalDevice], &DeviceMemoryProperties);

  std::optional<UINT32> PerfectMemory =
    FindMemoryTypeWithFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  if (PerfectMemory)
  {
    DeviceLocalMemoryIndex = PerfectMemory;
    HostVisibleMemoryIndex = PerfectMemory;
    return;
  }

  HostVisibleMemoryIndex =
    FindMemoryTypeWithFlags(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  if (!HostVisibleMemoryIndex)
    error("not found host visible memory type");

  DeviceLocalMemoryIndex =
    FindMemoryTypeWithFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (DeviceLocalMemoryIndex)
    NeedCopyBuffers = TRUE;
}*/

/**
 * \brief Find queue family index with required queue property flags
 * \param Required Required queue family property flags
 * \return Queue family index
 */
std::optional<UINT32> vulkan_application::FindQueueFamilyWithFlags( VkQueueFlags Required ) const
{
  for (UINT32 QueueFamily = 0; QueueFamily < QueueFamilyProperties.size(); QueueFamily++)
    if ((QueueFamilyProperties[QueueFamily].queueFlags & Required) == Required)
      return QueueFamily;

  return std::nullopt;
}

/**
 * \brief Test queue family for presentation support
 * \param Surface Surface for presentation
 * \param FamilyIndex Queue family index
 * \return Presentation supported flag
 */
BOOL vulkan_application::IsPresentationSupported( VkSurfaceKHR Surface, UINT32 FamilyIndex ) const
{
  VkBool32 Res = FALSE;

  vulkan_validation::Check(
    vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevices[*SelectedPhysicalDevice],
                                         FamilyIndex, Surface, &Res),
    "presentation support request failed");

  return Res;
}

/**
 * \brief Select queue indices.
 * \param[in] Surface Surface for presentation
 */
VOID vulkan_application::SelectQueueIndices( VkSurfaceKHR Surface )
{
  UINT32 NumberOfQueueFamilies;
  
  vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevices[*SelectedPhysicalDevice],
                                           &NumberOfQueueFamilies, nullptr);

  QueueFamilyProperties.resize(NumberOfQueueFamilies);

  vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevices[*SelectedPhysicalDevice],
                                           &NumberOfQueueFamilies, QueueFamilyProperties.data());

  std::optional<UINT32> PerfectFamily =
    FindQueueFamilyWithFlags(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT);

  if (PerfectFamily)
  {
    TransferQueueFamilyIndex = PerfectFamily;
    ComputeQueueFamilyIndex = PerfectFamily;
    GraphicsQueueFamilyIndex = PerfectFamily;

    if (IsPresentationSupported(Surface, *PerfectFamily))
      PresentationQueueFamilyIndex = PerfectFamily;
    else
      for (UINT32 QueueFamily = 0; QueueFamily < QueueFamilyProperties.size(); QueueFamily++)
        if (IsPresentationSupported(Surface, QueueFamily))
          PresentationQueueFamilyIndex = QueueFamily;
  }
  else
  {
    ComputeQueueFamilyIndex = FindQueueFamilyWithFlags(VK_QUEUE_COMPUTE_BIT);

    TransferQueueFamilyIndex = FindQueueFamilyWithFlags(VK_QUEUE_TRANSFER_BIT);

    GraphicsQueueFamilyIndex = FindQueueFamilyWithFlags(VK_QUEUE_GRAPHICS_BIT);

    for (UINT32 QueueFamily = 0; QueueFamily < QueueFamilyProperties.size(); QueueFamily++)
      if (IsPresentationSupported(Surface, QueueFamily))
        PresentationQueueFamilyIndex = QueueFamily;
  }

  if (!ComputeQueueFamilyIndex || !TransferQueueFamilyIndex ||
      !GraphicsQueueFamilyIndex || !PresentationQueueFamilyIndex)
    std::runtime_error("not enough queue families");
}
