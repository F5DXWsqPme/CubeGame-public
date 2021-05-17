#ifndef __window_h_
#define __window_h_

#include "ext/volk/volk.h"
#include "ext/glfw/include/glfw/glfw3.h"
#include <string>

#include "def.h"

/**
 * \brief Window and event loop class
 */
class glfw_window
{
private:
  /** Pointer to window */
  GLFWwindow *WindowPtr = nullptr;

  /** Framebuffer width */
  INT FrameW = 0;

  /** Framebuffer height */
  INT FrameH = 0;

  /** Window name */
  std::string Name;

  /** Frames counter */
  INT NumberOfFrames = 0;

  /** Old FPS evaluation */
  FLT OldFPSEvaluationTime = 0;

public:
  /** Reference to framebuffer width */
  const INT &FrameWidth = FrameW;

  /** Reference to framebuffer height */
  const INT &FrameHeight = FrameH;

  /**
   * \brief Resize framebuffer function
   * \param[in] W Frammebuffer width
   * \param[in] H Framebuffer height
   */
  VOID ResizeFrame( INT W, INT H );

  /**
   * \brief Update window function
   * \return Window closed flag
   */
  BOOL UpdateWindow( VOID );

  /**
   * \brief Get pointer to window function
   * \return Pointer to GLFW window
   */
  GLFWwindow * GetWindowPtr( VOID ) const;

  /**
   * \brief Class constructor
   * \param[in] Name Window title
   * \param[in] W Window width
   * \param[in] H Window height
   */
  glfw_window( const std::string &Name, INT W, INT H );

  /**
   * \brief Key pressed indicator
   * \param[in] Key Key identifier
   * \return Key is pressed flag
  */
  BOOL IsKeyPressed( INT Key ) const;

  /**
   * \brief Mouse position function
   * \return Mouse position
   */
  std::pair<DBL, DBL> GetMousePosition( VOID ) const;

  /**
   * \brief Mouse position function
   * \param[in] x X Mouse position
   * \param[in] y Y Mouse position
   */
  VOID SetMousePosition( DBL x, DBL y ) const;

  /**
   * \brief Removed copy function
   * \param[in] Other Other window
   */
  glfw_window( const glfw_window &Other ) = delete;

  /**
   * \brief Removed copy function
   * \param[in] Other Other window
   * \return Reference to this
   */
  glfw_window & operator=( const glfw_window &Other ) = delete;

  /**
   * \brief Class destructor
   */
  virtual ~glfw_window( VOID );

  /**
   * \brief Error callback
   * \param[in] ErrorCode Error code
   * \param[in] Message Error message string
   */
  static VOID MakeError( INT ErrorCode, const CHAR *Message );
};

#endif /* __window_h_ */
