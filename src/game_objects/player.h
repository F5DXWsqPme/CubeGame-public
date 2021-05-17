#ifndef __player_h_
#define __player_h_

#include "render/render.h"
#include "render/glfw_window.h"
#include "chunks_manager.h"
#include "block_type.h"

/**
 * \brief Player class
 */
class player
{
public:
  /**
   * \brief Player class constructor
   * \param[in, out] Render Reference to render
   * \param[in] Window Reference to window
   */
  player( render &Render, const glfw_window &Window );

  /**
   * \brief Response function
   * \param Time Current time
   */
  VOID Response( FLT Time );

  /**
   * \brief Get currrent player chunk
   * \return Chunk position
   */
  std::pair<INT32, INT32> GetCurrentChunk( VOID );

  /**
   * \brief Set chunks manager function
   * \param ChunksManager Chunks manager
   */
  VOID SetChunksManager( chunks_manager *NewChunksManager );

  /**
   * \brief Set block function
   */
  VOID SetBlock( VOID );

  /**
   * \brief Remove block function
   */
  VOID RemoveBlock( VOID );

private:
  /** Pointer to chunks manager */
  chunks_manager *ChunksManager = nullptr;

  /** Old response time */
  FLT OldResponseTime = -1;

  /** Reference to render */
  render &Render;

  /** Reference to window */
  const glfw_window &Window;

  /** Horizontal rotation coordinate */
  DBL HorizontalRotation = 0;

  /** Vertical rotation coordinate */
  DBL VerticalRotation = 0;

  /** Speed of vertical rotation */
  DBL VerticalRotationSpeed = 0.001;

  /** Speed of horizontal rotation */
  DBL HorizontalRotationSpeed = 0.001;

  /** Player speed */
  FLT Speed = 10.8;

  /** Player direction */
  glm::vec3 Direction = glm::vec3(1, 0, 0);

  /** Player move direction */
  glm::vec3 Forward = glm::vec3(1, 0, 0);

  /** Player right direction */
  glm::vec3 Right = glm::vec3(0, 0, 1);

  /** Camera position */
  glm::vec3 Position = glm::vec3(0, 80, 0);

  /** Default cursor position */
  std::pair<DBL, DBL> DefaultCursorPos = std::pair(100, 100);

  /** Mutex for position */
  std::mutex PositionMutex;

  /** Maximal distance for block set */
  INT MaxBlockSetDistance = 5;

  /** Old left mouse button state */
  INT OldLeftMouseButtonState = GLFW_RELEASE;

  /** Old right mouse button state */
  INT OldRightMouseButtonState = GLFW_RELEASE;

  /** Current block type for set */
  UINT32 CurBlockId = BLOCK_TYPE::GlassId;

  /** Q was pressed last time flag */
  BOOL OldQPressed = FALSE;

  /** E was pressed last time flag */
  BOOL OldEPressed = FALSE;
};

#endif /* __player_h_ */
