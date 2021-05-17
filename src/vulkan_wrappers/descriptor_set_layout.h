#ifndef __descriptor_set_layout_h_
#define __descriptor_set_layout_h_

#include "ext/volk/volk.h"

#include "def.h"

/**
 * \brief Descriptor set layout class
 */
class descriptor_set_layout
{
public:
  /**
   * \brief Default constructor.
   */
  descriptor_set_layout( VOID ) = default;

  /**
   * \brief Descriptor set layout constructor
   * \param[in] Device Device identifier
   * \param[in] NumberOfBindings Number of descriptor set bindings
   * \param[in] Bindings Descriptor set bindings
   */
  descriptor_set_layout( const VkDevice Device, const UINT32 NumberOfBindings,
                         const VkDescriptorSetLayoutBinding *Bindings );

  /**
   * \brief Descriptor set layout destructor
   */
  ~descriptor_set_layout( VOID );

  /**
   * \brief Get descriptor set layout identifier
   * \return Descriptor set layout
   */
  VkDescriptorSetLayout GetSetLayoutId( VOID ) const noexcept;

  /**
   * \brief Move function
   * \param[in] Layout Vulkan descriptor set layout
   * \return Reference to this
   */
  descriptor_set_layout & operator=( descriptor_set_layout &&Layout ) noexcept;

  /**
   * \brief Move function
   * \param[in] Layout Vulkan descriptor set layout
   */
  descriptor_set_layout( descriptor_set_layout &&Layout ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] Layout Vulkan descriptor set layout
   * \return Reference to this
   */
  descriptor_set_layout & operator=( const descriptor_set_layout &Layout ) = delete;

  /**
   * \brief Removed copy function
   * \param[in] Layout Vulkan descriptor set layout
   */
  descriptor_set_layout( const descriptor_set_layout &Layout ) = delete;

  /** Descriptor set layout identifier */
  VkDescriptorSetLayout SetLayoutId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /* __descriptor_set_layout_h_ */
