#include "block_type.h"

/**
 * \brief Block type constructor
 * \param Alpha Block transparency
 * \param TexCoord1 Texture coord for 1 vertex
 * \param TexCoord2 Texture coord for 2 vertex
 * \param TexCoord3 Texture coord for 3 vertex
 * \param TexCoord4 Texture coord for 4 vertex
 */
BLOCK_TYPE::BLOCK_TYPE( FLT Alpha ) :
  Alpha(Alpha)
{
}

/** Air type identifier */
const UINT32 BLOCK_TYPE::AirId = 0;

/** Stone type identifier */
const UINT32 BLOCK_TYPE::StoneId = 1;

/** Grass type identifier */
const UINT32 BLOCK_TYPE::GrassId = 2;

/** Glass type identifier */
const UINT32 BLOCK_TYPE::GlassId = 3;

/** Block types table */
std::vector<BLOCK_TYPE> BLOCK_TYPE::Table =
{
  BLOCK_TYPE(0),    // Air
  BLOCK_TYPE(1),    // Stone
  BLOCK_TYPE(1),    // Grass
  BLOCK_TYPE(0.99), // Glass
};
