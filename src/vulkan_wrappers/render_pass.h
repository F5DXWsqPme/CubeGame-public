#ifndef __render_pass_h_
#define __render_pass_h_

#include "ext/volk/volk.h"

#include "def.h"

/**
 * \brief Render pass class
 */
class render_pass
{
public:
  /**
   * \brief Render pass constructor
   */
  render_pass( VOID ) = default;

  /**
   * \brief Render pass constructor
   * \param Device Vulkan device
   * \param NumberOfAttachments Number of attachments
   * \param Attachments Attachments descriptions
   * \param NumberOfSubpasses Number of subpasses
   * \param Subpasses Subpasses description
   * \param NumberOfDependencies Number of dependencies
   * \param Dependencies Dependencies description
   */
  render_pass( VkDevice Device, UINT32 NumberOfAttachments,
               const VkAttachmentDescription *Attachments,
               UINT32 NumberOfSubpasses,
               const VkSubpassDescription *Subpasses,
               UINT32 NumberOfDependencies,
               const VkSubpassDependency *Dependencies );

  /**
   * \brief Render pass description
   */
  ~render_pass( VOID );

  /**
   * \brief Get render pass identifier
   * \return Render pass identifier
   */
  VkRenderPass GetRenderPassId( VOID ) const noexcept;

  /**
   * \brief Removed move function
   * \param[in] Pass Vulkan render pass
   * \return Reference to this
   */
  render_pass & operator=( render_pass &&Pass ) noexcept;

  /**
   * \brief Removed move function
   * \param[in] Pass Vulkan render pass
   */
  render_pass( render_pass &&Pass ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] Pass Vulkan render pass
   * \return Reference to this
   */
  render_pass & operator=( const render_pass &Pass ) = delete;

  /**
   * \brief Removed copy function
   * \param[in] Pass Vulkan render pass
   */
  render_pass( const render_pass &Pass ) = delete;

  /** Render pass identifier */
  VkRenderPass RenderPassId = VK_NULL_HANDLE;

  /** Vulkan device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /* __render_pass_h_ */
