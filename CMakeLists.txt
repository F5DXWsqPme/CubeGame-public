cmake_minimum_required(VERSION 3.16)

set(CURRENT_PROJECT_NAME Graphics-pipeline)

project(${CURRENT_PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

IF (UNIX)
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -pthread)
ENDIF (UNIX)

set(PROJECT_SOURCES
  src/ext/volk/volk.h
  src/ext/volk/volk.c

  src/vulkan_wrappers/buffer.h
  src/vulkan_wrappers/command_buffer.h
  src/vulkan_wrappers/command_pool.h
  src/vulkan_wrappers/compute_pipeline.h
  src/vulkan_wrappers/descriptor_pool.h
  src/vulkan_wrappers/descriptor_set_layout.h
  src/vulkan_wrappers/memory.h
  src/vulkan_wrappers/pipeline_cache.h
  src/vulkan_wrappers/pipeline_layout.h
  src/vulkan_wrappers/queue.h
  src/vulkan_wrappers/shader_module.h
  src/vulkan_wrappers/vulkan_application.h
  src/vulkan_wrappers/vulkan_validation.h
  src/vulkan_wrappers/fullscreen_surface.h
  src/render/glfw_window.h
  src/vulkan_wrappers/glfw_surface.h
  src/vulkan_wrappers/swapchain.h
  src/game_objects/event_loop.h
  src/vulkan_wrappers/buffer.cpp
  src/vulkan_wrappers/command_buffer.cpp
  src/vulkan_wrappers/command_pool.cpp
  src/vulkan_wrappers/compute_pipeline.cpp
  src/vulkan_wrappers/descriptor_pool.cpp
  src/vulkan_wrappers/descriptor_set_layout.cpp
  src/vulkan_wrappers/memory.cpp
  src/vulkan_wrappers/pipeline_cache.cpp
  src/vulkan_wrappers/pipeline_layout.cpp
  src/vulkan_wrappers/queue.cpp
  src/vulkan_wrappers/shader_module.cpp
  src/vulkan_wrappers/vulkan_application.cpp
  src/vulkan_wrappers/vulkan_validation.cpp
  src/vulkan_wrappers/fence.h
  src/vulkan_wrappers/fence.cpp
  src/vulkan_wrappers/semaphore.h
  src/vulkan_wrappers/semaphore.cpp
  src/vulkan_wrappers/event.h
  src/vulkan_wrappers/event.cpp
  src/vulkan_wrappers/render_pass.h
  src/vulkan_wrappers/render_pass.cpp
  src/vulkan_wrappers/graphics_pipeline.h
  src/vulkan_wrappers/graphics_pipeline.cpp
  src/vulkan_wrappers/fullscreen_surface.cpp
  src/render/glfw_window.cpp
  src/vulkan_wrappers/glfw_surface.cpp
  src/vulkan_wrappers/swapchain.cpp
  src/game_objects/event_loop.cpp
  src/render/render.h
  src/render/render.cpp
  src/render/vertex.h
  src/render/draw_element.h
  src/render/draw_element.cpp
  src/render/chunk_geometry.h
  src/render/memory_manager.h
  src/render/memory_manager.cpp
  src/render/render_synchronization.h
  src/game_objects/chunk.h
  src/game_objects/chunk.cpp
  src/game_objects/block.cpp
  src/game_objects/block.h
  src/game_objects/block_type.cpp
  src/game_objects/block_type.h
  src/game_objects/chunks_manager.cpp
  src/game_objects/chunks_manager.h
  src/render/chunk_geometry.cpp
  src/render/camera.cpp
  src/render/camera.h
  src/render/texture_atlas.cpp
  src/render/texture_atlas.h src/game_objects/player.cpp src/game_objects/player.h src/vulkan_wrappers/image.cpp src/vulkan_wrappers/image.h src/vulkan_wrappers/image_view.cpp src/vulkan_wrappers/image_view.h src/vulkan_wrappers/sampler.cpp src/vulkan_wrappers/sampler.h src/render/uniform_buffer.h src/utils/aabb.cpp src/utils/aabb.h src/utils/ray.h src/utils/ray.cpp src/utils/settings.h)

add_executable(${CURRENT_PROJECT_NAME}
  ${PROJECT_SOURCES}
  src/main.cpp)

target_include_directories(${CURRENT_PROJECT_NAME} PRIVATE src)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(src/ext/glfw)
target_link_libraries(${CURRENT_PROJECT_NAME} PRIVATE glfw)

add_subdirectory(src/ext/glm)

add_subdirectory(src/ext/volk)
target_link_libraries(${CURRENT_PROJECT_NAME} PRIVATE volk_headers)

find_package(Vulkan)

IF (Vulkan_FOUND)
  message(STATUS "Vulkan include directory: ${Vulkan_INCLUDE_DIRS}")
  message(STATUS "Vulkan library: ${Vulkan_LIBRARY}")
  target_include_directories(${CURRENT_PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
ELSE (Vulkan_FOUND)
  message(FATAL_ERROR "Vulkan not found.")
ENDIF (Vulkan_FOUND)

IF (DEFINED ENV{VULKAN_SDK})
  IF (WIN32)
    set(GLSL_COMPILER $ENV{VULKAN_SDK}/Bin/glslangValidator.exe)
  ELSE (WIN32)
    set(GLSL_COMPILER $ENV{VULKAN_SDK}/bin/glslangValidator)
  ENDIF (WIN32)
ELSEIF (DEFINED VULKAN_SDK)
  IF (WIN32)
    set(GLSL_COMPILER ${VULKAN_SDK}/Bin/glslangValidator.exe)
  ELSE (WIN32)
    set(GLSL_COMPILER ${VULKAN_SDK}/bin/glslangValidator)
  ENDIF (WIN32)
ENDIF (DEFINED ENV{VULKAN_SDK})

IF (DEFINED GLSL_COMPILER)
  message(STATUS "GLSL compiler: ${GLSL_COMPILER}")

  file(GLOB_RECURSE SHADERS_SOURCES_VERT "shaders/*.vert")
  file(GLOB_RECURSE SHADERS_SOURCES_CTRL "shaders/*.ctrl")
  file(GLOB_RECURSE SHADERS_SOURCES_EVAL "shaders/*.eval")
  file(GLOB_RECURSE SHADERS_SOURCES_GEOM "shaders/*.geom")
  file(GLOB_RECURSE SHADERS_SOURCES_FRAG "shaders/*.frag")
  file(GLOB_RECURSE SHADERS_SOURCES_COMP "shaders/*.comp")

  set(SHADERS_SOURCES
    ${SHADERS_SOURCES_VERT}
    ${SHADERS_SOURCES_CTRL}
    ${SHADERS_SOURCES_EVAL}
    ${SHADERS_SOURCES_GEOM}
    ${SHADERS_SOURCES_FRAG}
    ${SHADERS_SOURCES_COMP})
  
  message(STATUS "Shaders ${SHADERS_SOURCES}")

  set(SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
  set(SHADERS_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders-build)

  FOREACH (SHADER ${SHADERS_SOURCES})
    file(RELATIVE_PATH SHADER_RELATIVE_PATH ${SHADERS_DIR} ${SHADER})
    message(STATUS "Compile command: ${GLSL_COMPILER} -V ${SHADER} -o ${SHADERS_BUILD_DIR}/${SHADER_RELATIVE_PATH}.spv")
    execute_process(COMMAND ${GLSL_COMPILER} -V ${SHADER} -o ${SHADERS_BUILD_DIR}/${SHADER_RELATIVE_PATH}.spv RESULT_VARIABLE EXIT_CODE)

    IF (${EXIT_CODE} EQUAL 0)
      message(STATUS "Shader successfully builded.")
    ELSE (${EXIT_CODE} EQUAL 0)
      message(STATUS "Shader not builded.")
    ENDIF (${EXIT_CODE} EQUAL 0)

  ENDFOREACH(SHADER)
ELSE (DEFINED GLSL_COMPILER)
  message(STATUS "GLSL compiler not found. Shaders not compiled.")
ENDIF (DEFINED GLSL_COMPILER)

set(Boost_ARCHITECTURE "-x64")
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

find_package(Boost COMPONENTS thread unit_test_framework iostreams REQUIRED)

IF (Boost_FOUND)
  message(STATUS "Boost include directory: ${Boost_INCLUDE_DIRS}")
  message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
  target_include_directories(${CURRENT_PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(${CURRENT_PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})
ELSE (Boost_FOUND)
  message(FATAL_ERROR "Boost not found.")
ENDIF (Boost_FOUND)

add_subdirectory(src/ext/zlib ${PROJECT_SOURCE_DIR}/src/ext/zlib)

target_link_libraries(${CURRENT_PROJECT_NAME} PRIVATE zlibstatic)

add_executable(Tests-run
  ${PROJECT_SOURCES}
  tests/tests_main.cpp)

target_include_directories(Tests-run PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(Tests-run PRIVATE src)

target_link_libraries(Tests-run PRIVATE volk_headers)
target_link_libraries(Tests-run PRIVATE glfw)
target_link_libraries(Tests-run PRIVATE ${Boost_LIBRARIES})
target_link_libraries(Tests-run PRIVATE zlibstatic)

IF (Vulkan_FOUND)
  target_include_directories(Tests-run PRIVATE ${Vulkan_INCLUDE_DIRS})
ELSE (Vulkan_FOUND)
  message(FATAL_ERROR "Vulkan not found.")
ENDIF (Vulkan_FOUND)
