#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb/stb_image.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "texture_atlas.h"
#include "vulkan_wrappers/buffer.h"
#include "vulkan_wrappers/memory.h"
#include "vulkan_wrappers/vulkan_validation.h"
#include "vulkan_wrappers/command_buffer.h"
#include "vulkan_wrappers/queue.h"
#include "vulkan_wrappers/fence.h"
#include "game_objects/block_type.h"

namespace bpt = boost::property_tree;
using namespace std::string_literals;

/**
 * \brief Texture atlas constructor
 * \param[in] VkApp Vulkan application
 * \param[in] AtlasFileName Name of file with atlas XML-description
 * \param[in] ImageFileName Name of file with atlas image
 */
texture_atlas::texture_atlas( const vulkan_application &VkApp,
                              const std::string_view &AtlasFileName,
                              const std::string_view &ImageFileName ) :
  VkApp(VkApp)
{
  INT AtlasW = 0, AtlasH = 0;

  try
  {
    bpt::ptree Tree;

    bpt::read_xml(AtlasFileName.data(), Tree);

    bpt::ptree AtlasTree = Tree.get_child("TextureAtlas");

    AtlasW = AtlasTree.get<INT>("<xmlattr>.width");
    AtlasH = AtlasTree.get<INT>("<xmlattr>.height");

    for (const auto &[NodeName, NodeSubtree] : AtlasTree)
    {
      if (NodeName == "sprite")
      {
        IMAGE_DESCRIPTION Description;

        std::string Name = NodeSubtree.get<std::string>("<xmlattr>.n");

        Description.X = NodeSubtree.get<INT>("<xmlattr>.x");
        Description.Y = NodeSubtree.get<INT>("<xmlattr>.y");
        Description.W = NodeSubtree.get<INT>("<xmlattr>.w");
        Description.H = NodeSubtree.get<INT>("<xmlattr>.h");

        if (ImageDescriptions.find(Name) != ImageDescriptions.end())
          throw std::runtime_error("image redefinition in texture atlas");

        ImageDescriptions[Name] = Description;
      }
    }
  }
  catch ( const bpt::xml_parser_error &Error )
  {
    throw std::runtime_error("loading texture atlas xml-description failed ("s + Error.what() + ")");
  }

  INT ImageW = 0, ImageH = 0, NumberOfComponents = 0;
  std::unique_ptr<UCHAR, decltype(&stbi_image_free)>
    ImageData(stbi_load(ImageFileName.data(), &ImageW, &ImageH, &NumberOfComponents, STBI_rgb_alpha),
              &stbi_image_free);

  if (ImageData == nullptr || NumberOfComponents != STBI_rgb_alpha)
    throw std::runtime_error("image for texture atlas not loaded");

  if (ImageW != AtlasW || ImageH != AtlasH)
    throw std::runtime_error("texture atlas image and texture atlas description file not compatible");

  Width = ImageW;
  Height = ImageH;

  UINT64 TextureSize = Width * Height * 4;

  buffer CopyBuffer(VkApp.GetDeviceId(), TextureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  VkMemoryRequirements CopyMemoryRequirements = CopyBuffer.GetMemoryRequirements();

  std::optional<UINT32> CopyMemoryIndex =
    VkApp.FindMemoryTypeWithFlags(CopyMemoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  if (!CopyMemoryIndex)
    throw std::runtime_error("memory for copy buffer not found");

  memory CopyMemory(VkApp.GetDeviceId(), CopyMemoryRequirements.size, *CopyMemoryIndex);

  CopyBuffer.BindMemory(CopyMemory, 0);

  {
    VOID *Data = nullptr;

    CopyMemory.MapMemory(0, CopyMemoryRequirements.size, &Data);

    memcpy(Data, ImageData.get(), TextureSize);

    if ((VkApp.DeviceMemoryProperties.memoryTypes[*CopyMemoryIndex].propertyFlags &
           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
    {
      CopyMemory.FlushAllRange(0);
    }

    CopyMemory.UnmapMemory();
  }

  VkImageCreateInfo ImageCreateInfo = {};

  ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ImageCreateInfo.pNext = nullptr;
  ImageCreateInfo.flags = 0;
  ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  ImageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  ImageCreateInfo.extent = {(UINT32)Width, (UINT32)Height, 1};
  ImageCreateInfo.mipLevels = 1;
  ImageCreateInfo.arrayLayers = 1;
  ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  ImageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  ImageCreateInfo.queueFamilyIndexCount = 0;
  ImageCreateInfo.pQueueFamilyIndices = nullptr;
  ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  Image = image(VkApp.GetDeviceId(), ImageCreateInfo);

  VkMemoryRequirements ImageMemoryRequirements = {};

  ImageMemoryRequirements = Image.GetMemoryRequirements();

  std::optional<UINT32> ImageMemoryIndex =
    VkApp.FindMemoryTypeWithFlags(ImageMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (!ImageMemoryIndex)
    ImageMemoryIndex =
      VkApp.FindMemoryTypeWithFlags(ImageMemoryRequirements, 0);

  if (!ImageMemoryIndex)
    throw std::runtime_error("memory for texture atlas image not found");

  ImageMemory = memory(VkApp.GetDeviceId(), ImageMemoryRequirements.size, *ImageMemoryIndex);

  Image.BindMemory(ImageMemory, 0);

  if (!VkApp.TransferQueueFamilyIndex)
    throw std::runtime_error("queue for transfer not found");

  queue TransferQueue(VkApp.GetDeviceId(), *VkApp.TransferQueueFamilyIndex, 0);
  command_pool CopyCommandPool(VkApp.GetDeviceId(), *VkApp.TransferQueueFamilyIndex, 0);

  VkCommandBuffer CommandBufferId = VK_NULL_HANDLE;

  CopyCommandPool.AllocateCommandBuffers(&CommandBufferId);

  command_buffer CommandBuffer(CommandBufferId);

  CommandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  {
    VkBufferMemoryBarrier Barrier = {};

    Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    Barrier.pNext = nullptr;
    Barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    Barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.buffer = CopyBuffer.GetBufferId();
    Barrier.offset = 0;
    Barrier.size = CopyMemoryRequirements.size;

    vkCmdPipelineBarrier(CommandBufferId, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 1, &Barrier, 0, nullptr);
  }

  {
    VkImageMemoryBarrier Barrier = {};

    Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.pNext = nullptr;
    Barrier.srcAccessMask = 0;
    Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    Barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.image = Image.GetImageId();
    Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Barrier.subresourceRange.baseMipLevel = 0;
    Barrier.subresourceRange.levelCount = 1;
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(CommandBufferId, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &Barrier);
  }

  VkBufferImageCopy ImageCopyRegion = {};

  ImageCopyRegion.bufferOffset = 0;
  ImageCopyRegion.bufferRowLength = 0;  // non-interrupting data in the buffer
  ImageCopyRegion.bufferImageHeight = 0;  // information from imageExtent
  ImageCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ImageCopyRegion.imageSubresource.mipLevel = 0;
  ImageCopyRegion.imageSubresource.baseArrayLayer = 0;
  ImageCopyRegion.imageSubresource.layerCount = 1;
  ImageCopyRegion.imageOffset.x = 0;
  ImageCopyRegion.imageOffset.y = 0;
  ImageCopyRegion.imageOffset.z = 0;
  ImageCopyRegion.imageExtent = {(UINT32)Width, (UINT32)Height, 1};

  vkCmdCopyBufferToImage(CommandBufferId, CopyBuffer.GetBufferId(), Image.GetImageId(),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageCopyRegion);

  {
    VkImageMemoryBarrier Barrier = {};

    Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.pNext = nullptr;
    Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (VkApp.TransferQueueFamilyIndex != VkApp.GraphicsQueueFamilyIndex)
    {
      Barrier.srcQueueFamilyIndex = *VkApp.TransferQueueFamilyIndex;
      Barrier.dstQueueFamilyIndex = *VkApp.GraphicsQueueFamilyIndex;
    }
    else
    {
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }
    
    Barrier.image = Image.GetImageId();
    Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Barrier.subresourceRange.baseMipLevel = 0;
    Barrier.subresourceRange.levelCount = 1;
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(CommandBufferId, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &Barrier);
  }

  CommandBuffer.End();

  fence Fence(VkApp.GetDeviceId());

  VkSubmitInfo SubmitInfo = {};

  SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  SubmitInfo.pNext = nullptr;
  SubmitInfo.waitSemaphoreCount = 0;
  SubmitInfo.pWaitSemaphores = nullptr;
  SubmitInfo.pWaitDstStageMask = nullptr;
  SubmitInfo.commandBufferCount = 1;
  SubmitInfo.pCommandBuffers = &CommandBufferId;
  SubmitInfo.signalSemaphoreCount = 0;
  SubmitInfo.pSignalSemaphores = nullptr;

  TransferQueue.Submit(&SubmitInfo, 1, Fence.GetFenceId());

  Fence.Wait();

  VkSamplerCreateInfo SamplerCreateInfo = {};

  SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  SamplerCreateInfo.pNext = nullptr;
  SamplerCreateInfo.flags = 0;
  SamplerCreateInfo.magFilter = VK_FILTER_NEAREST;
  SamplerCreateInfo.minFilter = VK_FILTER_NEAREST;
  SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  SamplerCreateInfo.mipLodBias = 0;
  SamplerCreateInfo.anisotropyEnable = VK_FALSE;
  SamplerCreateInfo.maxAnisotropy = 1;
  SamplerCreateInfo.compareEnable = VK_FALSE;
  SamplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
  SamplerCreateInfo.minLod = 0;
  SamplerCreateInfo.maxLod = 1;
  SamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

  Sampler = sampler(VkApp.GetDeviceId(), SamplerCreateInfo);

  VkImageViewCreateInfo ImageViewCreateInfo = {};

  ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ImageViewCreateInfo.pNext = nullptr;
  ImageViewCreateInfo.flags = 0;
  ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ImageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
  ImageViewCreateInfo.subresourceRange.levelCount = 1;
  ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
  ImageViewCreateInfo.subresourceRange.layerCount = 1;
  ImageViewCreateInfo.image = Image.GetImageId();

  ImageView = image_view(VkApp.GetDeviceId(), ImageViewCreateInfo);

  FillBlockTypesTexCoords();
}

/**
 * \brief Fill texture coords in block types
 */
VOID texture_atlas::FillBlockTypesTexCoords( VOID ) const
{
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::StoneId].TexCoordRight, "stone.bmp");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::StoneId].TexCoordLeft, "stone.bmp");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::StoneId].TexCoordUp, "stone.bmp");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::StoneId].TexCoordDown, "stone.bmp");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::StoneId].TexCoordFront, "stone.bmp");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::StoneId].TexCoordBack, "stone.bmp");

  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::GrassId].TexCoordRight, "grass_side.bmp");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::GrassId].TexCoordLeft, "grass_side.bmp");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::GrassId].TexCoordUp, "grass_up.bmp");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::GrassId].TexCoordDown, "grass_down.bmp");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::GrassId].TexCoordFront, "grass_side.bmp");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::GrassId].TexCoordBack, "grass_side.bmp");

  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::GlassId].TexCoordRight, "glass.png");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::GlassId].TexCoordLeft, "glass.png");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::GlassId].TexCoordUp, "glass.png");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::GlassId].TexCoordDown, "glass.png");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::GlassId].TexCoordFront, "glass.png");
  FillTexCoords(BLOCK_TYPE::Table[BLOCK_TYPE::GlassId].TexCoordBack, "glass.png");
}

/**
 * \brief Fill texture coordinates
 * \param[in, out] TexCoords Pointer to texture
 * \param[in] TexName Name of texture
 */
VOID texture_atlas::FillTexCoords( glm::vec2 *TexCoords, const std::string &TexName ) const
{
  std::map<std::string, IMAGE_DESCRIPTION>::const_iterator It = ImageDescriptions.find(TexName);

  if (It == ImageDescriptions.cend())
    throw std::runtime_error("image description with name \"" + TexName + "\" not found");

  const IMAGE_DESCRIPTION &Description = It->second;

  glm::vec2 Offset(Description.X / (FLT)Width, Description.Y / (FLT)Height);
  FLT SizeX = Description.W / (FLT)Width;
  FLT SizeY = Description.H / (FLT)Height;

  TexCoords[0] = Offset + glm::vec2(SizeX, SizeY);;
  TexCoords[1] = Offset + glm::vec2(SizeX, 0);
  TexCoords[2] = Offset + glm::vec2(0, 0);
  TexCoords[3] = Offset + glm::vec2(0, SizeY);
}

/**
 * \brief Texture atlas destructor
 */
texture_atlas::~texture_atlas( VOID )
{
}
