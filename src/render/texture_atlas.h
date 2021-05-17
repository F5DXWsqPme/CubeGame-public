#ifndef __texture_atlas_h_
#define __texture_atlas_h_

#include <string_view>
#include <map>

#include "def.h"
#include "vulkan_wrappers/vulkan_application.h"
#include "vulkan_wrappers/memory.h"
#include "vulkan_wrappers/command_pool.h"
#include "vulkan_wrappers/image_view.h"
#include "vulkan_wrappers/image.h"
#include "vulkan_wrappers/sampler.h"

/**
 * \brief Texture atlas class
 */
class texture_atlas
{
public:
  /**
   * \brief Texture atlas constructor
   * \param[in] VkApp Vulkan application
   * \param[in] AtlasFileName Name of file with atlas XML-description
   * \param[in] ImageFileName Name of file with atlas image
   */
  texture_atlas( const vulkan_application &VkApp,
                 const std::string_view &AtlasFileName,
                 const std::string_view &ImageFileName );

  /**
   * \brief Texture atlas destructor
   */
  ~texture_atlas( VOID );

  /** Texture sampler */
  sampler Sampler;

  /** Image */
  image Image;

  /** Texture image view */
  image_view ImageView;

private:
  /**
   * \brief Fill texture coords in block types
   */
  VOID FillBlockTypesTexCoords( VOID ) const;

  /**
   * \brief Fill texture coordinates
   * \param[in, out] TexCoords Pointer to texture
   * \param[in] TexName Name of texture
   */
  VOID FillTexCoords( glm::vec2 *TexCoords, const std::string &TexName ) const;

  /** Reference to vulkan application */
  const vulkan_application &VkApp;

  /**
   * \brief Atlas image description from atlas XML-file
   */
  struct IMAGE_DESCRIPTION
  {
    /** X-coordinate offset */
    INT X = 0;

    /** Y-coordinate offset */
    INT Y = 0;

    /** Image width */
    INT W = 0;

    /** Image height */
    INT H = 0;
  };

  /** Atlas width */
  INT Width = 0;

  /** Atlas height */
  INT Height = 0;

  /** Map with atlas images descriptions */
  std::map<std::string, IMAGE_DESCRIPTION> ImageDescriptions;

  /** Image memory */
  memory ImageMemory;
};

#endif /* __texture_atlas_h_ */
