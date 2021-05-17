#ifndef __shader_module_h_
#define __shader_module_h_

#include "ext/volk/volk.h"
#include <string_view>

#include "def.h"

/**
 * \brief Vulkan shader module class
 */
class shader_module
{
public:
  /**
   * \brief Default constructor.
   */
  shader_module( VOID ) = default;

  /**
   * \brief Shader module constructor
   * \param[in] Device Device identifier
   * \param[in] FileName Name of file with shader
   */
  shader_module( const VkDevice Device, const std::string_view &FileName );

  /**
   * \brief Get shader module identifier
   * \return Shader module identifier
   */
  VkShaderModule GetShaderModuleId( VOID ) const noexcept;

  /**
   * \brief Shader module destructor
   */
  ~shader_module( VOID );

  /**
   * \brief Move function
   * \param[in] Shader Vulkan shader module
   * \return Reference to this
   */
  shader_module & operator=( shader_module &&Shader ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] Shader Vulkan shader module
   * \return Reference to this
   */
  shader_module( shader_module &&Shader ) noexcept;

private:
  /**
   * \brief Removed copy function
   * \param[in] Shader Vulkan shader module
   * \return Reference to this
   */
  shader_module & operator=( const shader_module &Shader ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] Shader Vulkan shader module
   */
  shader_module( const shader_module &Shader ) = delete;

  /** Shader module identifier */
  VkShaderModule ShaderModuleId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /* __shader_module_h_ */
