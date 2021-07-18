#include <ctime>
#include <iostream>
#include <exception>

#include "glfw_window.h"

/**
 * \brief Error callback
 * \param[in] ErrorCode Error code
 * \param[in] Message Error message string
 */
VOID glfw_window::MakeError( INT ErrorCode, const CHAR *Message )
{
  throw std::runtime_error(Message);
}

/**
 * \brief Resize framebuffer function
 * \param[in] W Framebuffer width
 * \param[in] H Framebuffer height
 */
VOID glfw_window::ResizeFrame( INT W, INT H )
{
  FrameW = W;
  FrameH = H;
}

/**
 * \brief Update window function
 * \return Window closed flag
 */
BOOL glfw_window::UpdateWindow( VOID )
{
  if (glfwWindowShouldClose(WindowPtr) == GLFW_TRUE)
    return TRUE;
  else
  {
    glfwPollEvents();

    return FALSE;
  }
}

/**
 * \brief Get pointer to window function
 * \return Pointer to GLFW window
 */
GLFWwindow * glfw_window::GetWindowPtr( VOID ) const
{
  return WindowPtr;
}

/**
 * \brief Class constructor
 * \param[in] Name Window title
 * \param[in] W Window width
 * \param[in] H Window height
 */
glfw_window::glfw_window( const std::string &Name, INT W, INT H ) : Name(Name)
{
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  GLFWmonitor *Monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode *Mode = glfwGetVideoMode(Monitor);

  glfwWindowHint(GLFW_RED_BITS, Mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, Mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, Mode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, Mode->refreshRate);

  //WindowPtr = glfwCreateWindow(Mode->width, Mode->height, Name.c_str(), Monitor, nullptr);

  WindowPtr = glfwCreateWindow(W, H, Name.c_str(), nullptr, nullptr);

  if (WindowPtr == nullptr)
    throw std::runtime_error("Window don't created");

  glfwSetInputMode(WindowPtr, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

  glfwSetWindowUserPointer(WindowPtr, this);

  glfwSetFramebufferSizeCallback(WindowPtr, []( GLFWwindow *Wnd, INT W, INT H )
  {
    glfw_window *WindowClass = reinterpret_cast<glfw_window *>(glfwGetWindowUserPointer(Wnd));

    WindowClass->ResizeFrame(W, H);
  });

  INT NewFrameW, NewFrameH;

  glfwGetFramebufferSize(WindowPtr, &NewFrameW, &NewFrameH);
  ResizeFrame(NewFrameW, NewFrameH);

  OldFPSEvaluationTime = glfwGetTime();
}

/**
 * \brief Key pressed indicator
 * \param[in] Key Key identifier
 * \return Key is pressed flag
*/
BOOL glfw_window::IsKeyPressed( INT Key ) const
{
  return glfwGetKey(WindowPtr, Key) == GLFW_PRESS;
}

/**
 * \brief Mouse position function
 * \return Mouse position
 */
std::pair<DBL, DBL> glfw_window::GetMousePosition( VOID ) const
{
  DBL x, y;

  glfwGetCursorPos(WindowPtr, &x, &y);

  return std::pair(x, y);
}

/**
 * \brief Mouse position function
 * \param[in] x X Mouse position
 * \param[in] y Y Mouse position
 */
VOID glfw_window::SetMousePosition( DBL x, DBL y ) const
{
  glfwSetCursorPos(WindowPtr, x, y);
}

/**
 * \brief Class destructor
 */
glfw_window::~glfw_window( VOID )
{
  glfwDestroyWindow(WindowPtr);
}
