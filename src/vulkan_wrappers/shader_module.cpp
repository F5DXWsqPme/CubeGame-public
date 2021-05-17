#include <cassert>
#include <fstream>
#include <vector>
#include <exception>

#include "shader_module.h"
#include "vulkan_validation.h"

using namespace std::string_literals;

/**
 * \brief Shader module constructor
 * \param[in] Device Device identifier
 * \param[in] FileName Name of file with shader
 */
shader_module::shader_module( const VkDevice Device, const std::string_view &FileName ) :
  DeviceId(Device)
{
  std::ifstream File(FileName.data(), std::ios::binary | std::ios::ate);
  
  if (!File)
    throw std::runtime_error("File "s + FileName.data() + " not found");
  
  INT64 Size = File.tellg();
  std::vector<CHAR> Bytes(Size);
  
  File.seekg(std::ios::beg);
  File.read(Bytes.data(), Size);
  
  VkShaderModuleCreateInfo CreateInfo = {};
  
  CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  CreateInfo.codeSize = Size * sizeof(CHAR);
  CreateInfo.pCode = reinterpret_cast<UINT32 *>(Bytes.data());
  
  vulkan_validation::Check(
    vkCreateShaderModule(DeviceId, &CreateInfo, nullptr, &ShaderModuleId),
    "shader module creating failed");
}

/**
 * \brief Get shader module identifier
 * \return Shader module identifier
 */
VkShaderModule shader_module::GetShaderModuleId( VOID ) const noexcept
{
  return ShaderModuleId;
}

/**
 * \brief Shader module destructor
 */
shader_module::~shader_module( VOID )
{
  if (ShaderModuleId != VK_NULL_HANDLE)
    vkDestroyShaderModule(DeviceId, ShaderModuleId, nullptr);
}

/**
 * \brief Move function
 * \param[in] Shader Vulkan shader module
 * \return Reference to this
 */
shader_module & shader_module::operator=( shader_module &&Shader ) noexcept
{
  //assert(ShaderModuleId == VK_NULL_HANDLE);

  std::swap(ShaderModuleId, Shader.ShaderModuleId);
  std::swap(DeviceId, Shader.DeviceId);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] Shader Vulkan shader module
 * \return Reference to this
 */
shader_module::shader_module( shader_module &&Shader ) noexcept
{
  //assert(ShaderModuleId == VK_NULL_HANDLE);

  std::swap(ShaderModuleId, Shader.ShaderModuleId);
  std::swap(DeviceId, Shader.DeviceId);
}
