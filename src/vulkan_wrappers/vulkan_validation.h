#ifndef __vulkan_validation_h_
#define __vulkan_validation_h_

#include "ext/volk/volk.h"
#include <string_view>

#include "def.h"

/**
 * \brief Vulkan errors validation class
 */
class vulkan_validation
{
public:
  /**
   * \brief Vulkan function result validation
   * \param[in] Result Vulkan function result
   * \param[in] Message Error message
   */
  static VOID Check( const VkResult Result, const std::string_view &Message = "" );
};

#endif /* __vulkan_validation_h_ */
