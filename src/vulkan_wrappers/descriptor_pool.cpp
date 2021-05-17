#include <cassert>

#include "descriptor_pool.h"
#include "vulkan_validation.h"

/**
 * \brief Descriptor poll constructor
 * \param[in] Device Device identifier
 * \param[in] Flags Descriptor poll flags
 * \param[in] MaxNumberOfSets Maximal number of descriptor sets
 * \param[in] NumberOfPoolSizes Number of pool sizes
 * \param[in] PoolSizes Descriptor pool sizes
 */
descriptor_pool::descriptor_pool( const VkDevice Device,
                                  const VkDescriptorPoolCreateFlags Flags,
                                  const UINT32 MaxNumberOfSets,
                                  const UINT32 NumberOfPoolSizes,
                                  const VkDescriptorPoolSize *PoolSizes ) :
  DeviceId(Device)
{
  VkDescriptorPoolCreateInfo CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  CreateInfo.flags = Flags;
  CreateInfo.maxSets = MaxNumberOfSets;
  CreateInfo.poolSizeCount = NumberOfPoolSizes;
  CreateInfo.pPoolSizes = PoolSizes;

  vulkan_validation::Check(
    vkCreateDescriptorPool(DeviceId, &CreateInfo, nullptr, &PoolId),
    "descriptor pool creating failed");
}

/**
 * \brief Descriptor pool destructor
 */
descriptor_pool::~descriptor_pool( VOID )
{
  if (PoolId != VK_NULL_HANDLE)
    vkDestroyDescriptorPool(DeviceId, PoolId, nullptr);
}

/**
 * \brief Move function
 * \param[in] Pool Vulkan descriptor pool
 * \return Reference to this
 */
descriptor_pool & descriptor_pool::operator=( descriptor_pool &&Pool ) noexcept
{
  //assert(PoolId == VK_NULL_HANDLE);

  std::swap(PoolId, Pool.PoolId);
  std::swap(DeviceId, Pool.DeviceId);

  return *this;
}

/**
 * \brief Move function
 * \param[in] Pool Vulkan descriptor pool
 * \return Reference to this
 */
descriptor_pool::descriptor_pool( descriptor_pool &&Pool ) noexcept
{
  //assert(PoolId == VK_NULL_HANDLE);

  std::swap(PoolId, Pool.PoolId);
  std::swap(DeviceId, Pool.DeviceId);
}

/**
 * \brief Allocate descriptor sets function
 * \param[in, out] DescriptorSets Descriptor sets for allocation
 * \param[in] NumberOfSets Number of descriptor sets
 * \param[in] SetLayouts Descriptor set layouts
 */
VOID descriptor_pool::AllocateSets( VkDescriptorSet *DescriptorSets,
                                    const UINT32 NumberOfSets,
                                    const VkDescriptorSetLayout *SetLayouts ) const
{
  VkDescriptorSetAllocateInfo AllocateInfo = {};

  AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  AllocateInfo.descriptorPool = PoolId;
  AllocateInfo.descriptorSetCount = NumberOfSets;
  AllocateInfo.pSetLayouts = SetLayouts;

  vulkan_validation::Check(
    vkAllocateDescriptorSets(DeviceId, &AllocateInfo, DescriptorSets),
    "descriptor sets allocation failed");
}

/**
 * \brief Free descriptor sets function
 * \param[in] NumberOfSets Number of descriptor sets
 * \param[in] DescriptorSets Descriptor sets for free
 */
VOID descriptor_pool::FreeSets( const UINT32 NumberOfSets,
                                const VkDescriptorSet *DescriptorSets ) const
{
  vulkan_validation::Check(
    vkFreeDescriptorSets(DeviceId, PoolId, NumberOfSets, DescriptorSets),
    "descriptor sets free failed");
}

/**
 * \brief Reset descriptor pool function
 * \param[in] Flags Descriptor pool reset flags
 */
VOID descriptor_pool::Reset( const VkDescriptorPoolResetFlags Flags ) const   
{
  vulkan_validation::Check(
    vkResetDescriptorPool(DeviceId, PoolId, Flags),
    "descriptor pool reset failed");
}

/**
 * \brief Get descriptor poll identifier function
 * \return Descriptor pool identifier
 */
VkDescriptorPool descriptor_pool::GetPoolId( VOID ) const noexcept
{
  return PoolId;
}
