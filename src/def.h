#ifndef __def_h_
#define __def_h_

#include <cstdint>

/* Scalar types */
typedef void VOID;
typedef bool BOOL;
#define TRUE true
#define FALSE false
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef int INT;
typedef unsigned int UINT;
typedef float FLOAT;
typedef double DOUBLE;
typedef FLOAT FLT;
typedef DOUBLE DBL;
typedef int64_t INT64;
typedef uint64_t UINT64;

#define ENABLE_VULKAN_FUNCTION_RESULT_VALIDATION 0
#define ENABLE_VULKAN_VALIDATION_LAYER 0
#define ENABLE_VULKAN_PRINTF_EXTENSION 0
#define ENABLE_VULKAN_FULLSCREEN_SURFACE_EXTENSION 0
#define ENABLE_VULKAN_GLFW_SURFACE 1

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED

#include "ext/glm/glm/glm.hpp"
#include "ext/glm/glm/ext/matrix_clip_space.hpp"
#include "ext/glm/glm/ext/matrix_transform.hpp"
#include "ext/glm/glm/gtx/euler_angles.hpp"

#endif /* __def_h_ */
