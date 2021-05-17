#include <cassert>

#include "command_pool.h"
#include "vulkan_validation.h"

/**
 * \brief Command pool constructor
 * \param[in] Device Device identifier
 */
command_pool::command_pool( const VkDevice Device, const UINT32 QueueFamilyIndex,
                            const VkCommandPoolCreateFlags Flags ) :
  DeviceId(Device)
{
  VkCommandPoolCreateInfo CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  CreateInfo.flags = Flags;
  CreateInfo.queueFamilyIndex = QueueFamilyIndex;

  vulkan_validation::Check(
    vkCreateCommandPool(DeviceId, &CreateInfo, nullptr, &CommandPoolId),
    "command pool creation failed");
}

/**
 * \brief Command pool destructor
 */
command_pool::~command_pool( VOID )
{
  if (CommandPoolId != VK_NULL_HANDLE)
    vkDestroyCommandPool(DeviceId, CommandPoolId, nullptr);
}

/**
 * \brief Move function
 * \param[in] Pool Vulkan command pool
 * \return Reference to this
 */
command_pool & command_pool::operator=( command_pool &&Pool ) noexcept
{
  //assert(CommandPoolId == VK_NULL_HANDLE);

  std::swap(CommandPoolId, Pool.CommandPoolId);
  std::swap(DeviceId, Pool.DeviceId);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] Pool Vulkan command pool
 * \return Reference to this
 */
command_pool::command_pool( command_pool &&Pool ) noexcept
{
  //assert(CommandPoolId == VK_NULL_HANDLE);

  std::swap(CommandPoolId, Pool.CommandPoolId);
  std::swap(DeviceId, Pool.DeviceId);
}

/**
 * \brief Allocate command buffers function
 * \param[in, out] CommandBuffers Pointer to command buffers
 * \param[in] NumberOfBuffers Number of command buffers
 * \param[in] Level Command buffers level
 */
VOID command_pool::AllocateCommandBuffers( VkCommandBuffer *CommandBuffers, const UINT32 NumberOfBuffers,
                                           const VkCommandBufferLevel Level ) const
{
  VkCommandBufferAllocateInfo AllocateInfo = {};

  AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  AllocateInfo.commandPool = CommandPoolId;
  AllocateInfo.level = Level;
  AllocateInfo.commandBufferCount = NumberOfBuffers;

  vulkan_validation::Check(
    vkAllocateCommandBuffers(DeviceId, &AllocateInfo, CommandBuffers),
    "command buffers allocation failed");
}

/**
 * \brief Free command buffers function
 * \param[in] CommandBuffers Pointer to command buffers
 * \param[in] NumberOfBuffers Number of command buffers
 */
VOID command_pool::FreeCommandBuffers( const VkCommandBuffer *CommandBuffers,
                                       const UINT32 NumberOfBuffers ) const
{
  vkFreeCommandBuffers(DeviceId, CommandPoolId, NumberOfBuffers, CommandBuffers);
}

/**
 * \brief Reset command pool function
 * \param[in] Flags Reset flags
 */
VOID command_pool::Reset( const VkCommandPoolResetFlags Flags ) const
{
  vulkan_validation::Check(
    vkResetCommandPool(DeviceId, CommandPoolId, Flags),
    "reset command pool failed");
}
