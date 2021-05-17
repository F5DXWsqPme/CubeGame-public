#include "render_pass.h"
#include "vulkan_validation.h"

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
render_pass::render_pass( VkDevice Device, UINT32 NumberOfAttachments,
                          const VkAttachmentDescription *Attachments,
                          UINT32 NumberOfSubpasses,
                          const VkSubpassDescription *Subpasses,
                          UINT32 NumberOfDependencies,
                          const VkSubpassDependency *Dependencies ) :
  DeviceId(Device)
{
  VkRenderPassCreateInfo CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  CreateInfo.pNext = nullptr;
  CreateInfo.flags = 0;
  CreateInfo.attachmentCount = NumberOfAttachments;
  CreateInfo.pAttachments = Attachments;
  CreateInfo.subpassCount = NumberOfSubpasses;
  CreateInfo.pSubpasses = Subpasses;
  CreateInfo.dependencyCount = NumberOfDependencies;
  CreateInfo.pDependencies = Dependencies;

  vulkan_validation::Check(
    vkCreateRenderPass(Device, &CreateInfo, nullptr, &RenderPassId),
    "render pass creation failed");
}

/**
 * \brief Render pass description
 */
render_pass::~render_pass( VOID )
{
  if (RenderPassId != VK_NULL_HANDLE)
    vkDestroyRenderPass(DeviceId, RenderPassId, nullptr);
}

/**
 * \brief Get render pass identifier
 * \return Render pass identifier
 */
VkRenderPass render_pass::GetRenderPassId( VOID ) const noexcept
{
  return RenderPassId;
}

/**
 * \brief Removed move function
 * \param[in] Pass Vulkan render pass
 * \return Reference to this
 */
render_pass & render_pass::operator=( render_pass &&Pass ) noexcept
{
  std::swap(RenderPassId, Pass.RenderPassId);
  std::swap(DeviceId, Pass.DeviceId);

  return *this;
}

/**
 * \brief Removed move function
 * \param[in] Pass Vulkan render pass
 */
render_pass::render_pass( render_pass &&Pass ) noexcept
{
  std::swap(RenderPassId, Pass.RenderPassId);
  std::swap(DeviceId, Pass.DeviceId);
}
