#include <string>
#include <string_view>
#include <stdexcept>

#include "vulkan_validation.h"

using std::string_literals::operator""s;

/**
 * \brief Vulkan function result validation
 * \param[in] Result Vulkan function result
 * \param[in] Message Error message
 */
VOID vulkan_validation::Check( const VkResult Result, const std::string_view &Message )
{
#if ENABLE_VULKAN_FUNCTION_RESULT_VALIDATION
  if (Result != VK_SUCCESS)
    throw std::runtime_error("Vulkan error: "s + Message.data());
#endif
}
