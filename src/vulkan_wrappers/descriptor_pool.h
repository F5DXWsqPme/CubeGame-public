#ifndef __descriptor_pool_h_
#define __descriptor_pool_h_

#include "ext/volk/volk.h"

#include "def.h"

/**
 * \brief Descriptor pool class
 */
class descriptor_pool
{
public:
  /**
   * \brief Default constructor.
   */
  descriptor_pool( VOID ) = default;

  /**
   * \brief Descriptor poll constructor
   * \param[in] Device Device identifier
   * \param[in] Flags Descriptor poll flags
   * \param[in] MaxNumberOfSets Maximal number of descriptor sets
   * \param[in] NumberOfPoolSizes Number of pool sizes
   * \param[in] PoolSizes Descriptor pool sizes
   */
  descriptor_pool( const VkDevice Device,
                   const VkDescriptorPoolCreateFlags Flags,
                   const UINT32 MaxNumberOfSets,
                   const UINT32 NumberOfPoolSizes,
                   const VkDescriptorPoolSize *PoolSizes );

  /**
   * \brief Descriptor pool destructor
   */
  ~descriptor_pool( VOID );

  /**
   * \brief Allocate descriptor sets function
   * \param[in, out] DescriptorSets Descriptor sets for allocation
   * \param[in] NumberOfSets Number of descriptor sets
   * \param[in] SetLayouts Descriptor set layouts
   */
  VOID AllocateSets( VkDescriptorSet *DescriptorSets,
                     const UINT32 NumberOfSets,
                     const VkDescriptorSetLayout *SetLayouts ) const;

  /**
   * \brief Free descriptor sets function
   * \param[in] NumberOfSets Number of descriptor sets
   * \param[in] DescriptorSets Descriptor sets for free
   */
  VOID FreeSets( const UINT32 NumberOfSets,
                 const VkDescriptorSet *DescriptorSets ) const;

  /**
   * \brief Reset descriptor pool function
   * \param[in] Flags Descriptor pool reset flags
   */
  VOID Reset( const VkDescriptorPoolResetFlags Flags ) const;

  /**
   * \brief Get descriptor poll identifier function
   * \return Descriptor pool identifier
   */
  VkDescriptorPool GetPoolId( VOID ) const noexcept;

  /**
   * \brief Move function
   * \param[in] Pool Vulkan descriptor pool
   * \return Reference to this
   */
  descriptor_pool & operator=( descriptor_pool &&Pool ) noexcept;

  /**
   * \brief Move function
   * \param[in] Pool Vulkan descriptor pool
   */
  descriptor_pool( descriptor_pool &&Pool ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] Pool Vulkan descriptor pool
   * \return Reference to this
   */
  descriptor_pool & operator=( const descriptor_pool &Pool ) = delete;

  /**
   * \brief Removed copy function
   * \param[in] Pool Vulkan descriptor pool
   */
  descriptor_pool( const descriptor_pool &Pool ) = delete;

  /** Descriptor pool identifier */
  VkDescriptorPool PoolId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /* __descriptor_pool_h_ */
