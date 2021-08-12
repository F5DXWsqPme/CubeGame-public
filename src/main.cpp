#include <stdexcept>
#include <iostream>
#include <cstddef>
#include <optional>

#include "render/render.h"
#include "render/glfw_window.h"
#include "game_objects/event_loop.h"
#include "game_objects/chunk.h"
#include "game_objects/chunks_manager.h"
#include "game_objects/player.h"
#include "render/uniform_buffer.h"

/**
 * \brief Main function in program
 * \param[in] ArgC Number of arguments
 * \param[in] ArgV Array of arguments
 * \return Error code (0-if success)
 */
INT main( INT ArgC, CHAR **ArgV )
{
  try
  {
    glfwSetErrorCallback(glfw_window::MakeError);

    if (glfwInit() != GLFW_TRUE)
      throw std::runtime_error("GLFW not initialized");

    settings Settings;

    vulkan_application VkApp(Settings);
    glfw_window Window("", 800, 600);
    
    glfw_surface Surface(VkApp.GetPhysicalDevice(0), VkApp.GetInstanceId(), Window.GetWindowPtr());

    VkApp.InitApplication(0, Surface.GetSurfaceId());

    INT RenderDistance = 5;
    INT MaxNumberOfChunks = (RenderDistance * 2 + 1) * (RenderDistance * 2 + 1);

    render_synchronization Synchronization;

    memory_manager MemoryManager(VkApp, MaxNumberOfChunks,
      chunk_geometry::VertexBufferSize,
      chunk_geometry::IndexBufferSize,
      chunk_geometry::VertexBufferSize,
      sizeof(uniform_buffer),
      Synchronization);

    render Render(VkApp, Surface, MaxNumberOfChunks * 2, MemoryManager, Synchronization);

    player Player(Render, Window);

    chunks_manager ChunksManager(Render, RenderDistance, Player);

    Player.SetChunksManager(&ChunksManager);

    FLT LogicDelay = 0.05;
    event_loop EventLoop(LogicDelay);

    EventLoop.RegisterExitPredicate([&]( FLT ) -> BOOL
    {
      return Window.UpdateWindow();
    });

    EventLoop.RegisterLoopEvent([&]( FLT Time )
    {
      Render.RenderFrame(Time);
    });

    EventLoop.RegisterLoopEvent([&]( FLT Time )
    {
      Player.Response(Time);
    });

    EventLoop.Run();
  }
  catch ( const std::runtime_error &Err )
  {
    std::cout << "Error:\n  " << Err.what() << "\n";
  }

  glfwTerminate();
  
  return 0;
}
