#include <cassert>

#include "descriptor_set_layout.h"
#include "vulkan_validation.h"

/**
 * \brief Descriptor set layout constructor
 * \param[in] Device Device identifier
 * \param[in] NumberOfBindings Number of descriptor set bindings
 * \param[in] Bindings Descriptor set bindings
 */
descriptor_set_layout::descriptor_set_layout( const VkDevice Device,
  const UINT32 NumberOfBindings, const VkDescriptorSetLayoutBinding *Bindings ) :
  DeviceId(Device)
{
  VkDescriptorSetLayoutCreateInfo CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  CreateInfo.bindingCount = NumberOfBindings;
  CreateInfo.pBindings = Bindings;

  vulkan_validation::Check(
    vkCreateDescriptorSetLayout(DeviceId, &CreateInfo, nullptr, &SetLayoutId),
    "descriptor set layout creating failed");
}

/**
 * \brief Descriptor set layout destructor
 */
descriptor_set_layout::~descriptor_set_layout( VOID )
{
  if (SetLayoutId != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(DeviceId, SetLayoutId, nullptr);
}

/**
 * \brief Move function
 * \param[in] Layout Vulkan descriptor set layout
 * \return Reference to this
 */
descriptor_set_layout & descriptor_set_layout::operator=( descriptor_set_layout &&Layout ) noexcept
{
  //assert(SetLayoutId == VK_NULL_HANDLE);

  std::swap(SetLayoutId, Layout.SetLayoutId);
  std::swap(DeviceId, Layout.DeviceId);

  return *this;
}

/**
 * \brief Move function
 * \param[in] Layout Vulkan descriptor set layout
 * \return Reference to this
 */
descriptor_set_layout::descriptor_set_layout( descriptor_set_layout &&Layout ) noexcept
{
  //assert(SetLayoutId == VK_NULL_HANDLE);

  std::swap(SetLayoutId, Layout.SetLayoutId);
  std::swap(DeviceId, Layout.DeviceId);
}

/**
 * \brief Get descriptor set layout identifier
 * \return Descriptor set layout
 */
VkDescriptorSetLayout descriptor_set_layout::GetSetLayoutId( VOID ) const noexcept
{
  return SetLayoutId;
}
