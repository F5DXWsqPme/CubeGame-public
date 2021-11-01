#include <iostream>
#include <iomanip>

#include "player.h"
#include "chunk.h"
#include "block_type.h"

/**
 * \brief Player class constructor
 * \param[in, out] Render Reference to render
 * \param[in] Window Reference to window
 */
player::player( render &Render, const glfw_window &Window ) :
  Render(Render), Window(Window)
{
}

/**
 * \brief Set chunks manager function
 * \param ChunksManager Chunks manager
 */
VOID player::SetChunksManager( chunks_manager *NewChunksManager )
{
  ChunksManager = NewChunksManager;
}

/**
 * \brief Response function
 * \param Time Current time
 */
VOID player::Response( FLT Time )
{
  if (OldResponseTime < 0)
  {
    Window.SetMousePosition(DefaultCursorPos.first, DefaultCursorPos.second);
    OldResponseTime = Time;
    Render.Camera.SetLocAtUp(Position, Position + Direction, glm::vec3(0, -1, 0));
    Render.UpdateWVP();

    return;
  }

  FLT DeltaTime = Time - OldResponseTime;

  OldResponseTime = Time;

  std::pair<DBL, DBL> CursorPos = Window.GetMousePosition();

  BOOL IsPressedW = Window.IsKeyPressed(GLFW_KEY_W);
  BOOL IsPressedA = Window.IsKeyPressed(GLFW_KEY_A);
  BOOL IsPressedS = Window.IsKeyPressed(GLFW_KEY_S);
  BOOL IsPressedD = Window.IsKeyPressed(GLFW_KEY_D);
  BOOL IsPressedLeftShift = Window.IsKeyPressed(GLFW_KEY_LEFT_SHIFT);
  BOOL IsPressedSpace = Window.IsKeyPressed(GLFW_KEY_SPACE);

  if (CursorPos != DefaultCursorPos || IsPressedW ||
      IsPressedS || IsPressedD || IsPressedA ||
      IsPressedLeftShift || IsPressedSpace)
  {
    if (CursorPos != DefaultCursorPos)
      Window.SetMousePosition(DefaultCursorPos.first, DefaultCursorPos.second);

    glm::vec3 MoveDirection(0, 0, 0);

    if (IsPressedW)
      MoveDirection += Forward;

    if (IsPressedS)
      MoveDirection -= Forward;

    if (IsPressedD)
      MoveDirection += Right;

    if (IsPressedA)
      MoveDirection -= Right;

    if (IsPressedSpace)
      MoveDirection += glm::vec3(0, 1, 0);

    if (IsPressedLeftShift)
      MoveDirection -= glm::vec3(0, 1, 0);

    if (glm::length(MoveDirection) > FLT_EPSILON)
      MoveDirection = glm::normalize(MoveDirection);

    {
      std::lock_guard<std::mutex> Lock(PositionMutex);
      Position += MoveDirection * Speed * DeltaTime;
    }
    
    std::pair<DBL, DBL> DeltaPos = std::pair(DefaultCursorPos.first - CursorPos.first,
                                             DefaultCursorPos.second - CursorPos.second);

    HorizontalRotation += DeltaPos.first * HorizontalRotationSpeed;
    VerticalRotation += DeltaPos.second * VerticalRotationSpeed;

    VerticalRotation = std::min(VerticalRotation, 0.99);
    VerticalRotation = std::max(VerticalRotation, -0.99);

    DBL VerticalAngle = 0.5 * glm::pi<DBL>() * VerticalRotation;
    DBL HorizontalAngle = 0.5 * glm::pi<DBL>() * HorizontalRotation;

    glm::mat4 DirectionTransform = glm::yawPitchRoll(HorizontalAngle, 0.0, VerticalAngle);
    glm::mat4 ForwardTransform = glm::yawPitchRoll(HorizontalAngle, 0.0, 0.0);

    glm::vec4 NewForward = (ForwardTransform * glm::vec4(1, 0, 0, 1));
    glm::vec4 NewDirection = (DirectionTransform * glm::vec4(1, 0, 0, 1));
    glm::vec4 NewRight = (DirectionTransform * glm::vec4(0, 0, 1, 1));

    Forward = glm::vec3(NewForward.x, NewForward.y, NewForward.z);
    Direction = glm::vec3(NewDirection.x, NewDirection.y, NewDirection.z);
    Right = glm::vec3(NewRight.x, NewRight.y, NewRight.z);

    Render.Camera.SetLocAtUp(Position, Position + Direction, glm::vec3(0, -1, 0));

    Render.UpdateWVP();
  }

  INT LeftMouseButtonState = glfwGetMouseButton(Window.GetWindowPtr(), GLFW_MOUSE_BUTTON_LEFT);

  if (LeftMouseButtonState == GLFW_PRESS && OldLeftMouseButtonState != GLFW_PRESS)
    RemoveBlock();

  OldLeftMouseButtonState = LeftMouseButtonState;

  INT RightMouseButtonState = glfwGetMouseButton(Window.GetWindowPtr(), GLFW_MOUSE_BUTTON_RIGHT);

  if (RightMouseButtonState == GLFW_PRESS && OldRightMouseButtonState != GLFW_PRESS)
    SetBlock();

  OldRightMouseButtonState = RightMouseButtonState;

  BOOL IsQPressed = Window.IsKeyPressed(GLFW_KEY_Q);
  BOOL IsEPressed = Window.IsKeyPressed(GLFW_KEY_E);

  if (IsQPressed && !OldQPressed)
  {
    CurBlockId--;

    if (CurBlockId == 0)
      CurBlockId = BLOCK_TYPE::Table.size() - 1;
  }

  if (IsEPressed && !OldEPressed)
  {
    CurBlockId++;

    if (CurBlockId == BLOCK_TYPE::Table.size())
      CurBlockId = 1;
  }
}

/**
 * \brief Set block function
 */
VOID player::SetBlock( VOID )
{
  if (ChunksManager != nullptr)
  {
    std::pair<INT32, INT32> PlayerChunkPos = GetCurrentChunk();
    glm::vec3 Pos;

    {
      std::lock_guard<std::mutex> Lock(PositionMutex);

      Pos = Position;
    }

    glm::ivec3 BlockPos(0, 0, 0);
    FLT Near = INFINITY;
    std::pair<INT32, INT32> IntersectionChunkPos;
    glm::ivec3 Normal(0, 0, 0);

    BOOL IsIntersected = ChunksManager->GetSelectedBlock(Pos, Direction, PlayerChunkPos, MaxBlockSetDistance,
                                                         BlockPos, Near, IntersectionChunkPos, Normal);

    if (IsIntersected)
    {
      BlockPos += Normal;

      if (BlockPos.x == -1)
      {
        IntersectionChunkPos.first--;
        BlockPos.x = chunk::ChunkSizeX - 1;
      }

      if (BlockPos.x == chunk::ChunkSizeX)
      {
        IntersectionChunkPos.first++;
        BlockPos.x = 0;
      }

      if (BlockPos.y == -1 || BlockPos.y == chunk::ChunkSizeY)
        return;

      if (BlockPos.z == -1)
      {
        IntersectionChunkPos.second--;
        BlockPos.z = chunk::ChunkSizeZ - 1;
      }

      if (BlockPos.z == chunk::ChunkSizeZ)
      {
        IntersectionChunkPos.second++;
        BlockPos.z = 0;
      }

      INT32 BlockInd = BlockPos.z * chunk::ChunkSizeY * chunk::ChunkSizeX +
                       BlockPos.y * chunk::ChunkSizeX + BlockPos.x;
      chunk *IntersectionChunkPtr = nullptr;
      std::lock_guard<std::mutex> Lock(ChunksManager->GetActiveChunksMutex());

      if (ChunksManager->GetChunk(IntersectionChunkPos, IntersectionChunkPtr))
      {
        if (IntersectionChunkPtr->Blocks[BlockInd].BlockTypeId == BLOCK_TYPE::AirId)
        {
          IntersectionChunkPtr->Blocks[BlockInd].BlockTypeId = CurBlockId;
          ChunksManager->UpdateBlock(IntersectionChunkPos, BlockPos);
        }
      }
    }
  }
}

/**
 * \brief Remove block function
 */
VOID player::RemoveBlock( VOID )
{
  if (ChunksManager != nullptr)
  {
    std::pair<INT32, INT32> PlayerChunkPos = GetCurrentChunk();
    glm::vec3 Pos;

    {
      std::lock_guard<std::mutex> Lock(PositionMutex);

      Pos = Position;
    }

    glm::ivec3 BlockPos(0, 0, 0);
    FLT Near = INFINITY;
    std::pair<INT32, INT32> IntersectionChunkPos;
    glm::ivec3 Normal(0, 0, 0);

    BOOL IsIntersected = ChunksManager->GetSelectedBlock(Pos, Direction, PlayerChunkPos, MaxBlockSetDistance,
      BlockPos, Near, IntersectionChunkPos, Normal);

    if (IsIntersected)
    {
      INT32 BlockInd = BlockPos.z * chunk::ChunkSizeY * chunk::ChunkSizeX +
        BlockPos.y * chunk::ChunkSizeX + BlockPos.x;
      chunk *IntersectionChunkPtr = nullptr;
      std::lock_guard<std::mutex> Lock(ChunksManager->GetActiveChunksMutex());

      if (ChunksManager->GetChunk(IntersectionChunkPos, IntersectionChunkPtr))
      {
        IntersectionChunkPtr->Blocks[BlockInd].BlockTypeId = BLOCK_TYPE::AirId;
        ChunksManager->UpdateBlock(IntersectionChunkPos, BlockPos);
      }
    }
  }
}

/**
 * \brief Get currrent player chunk
 * \return Chunk position
 */
std::pair<INT32, INT32> player::GetCurrentChunk( VOID )
{
  std::lock_guard<std::mutex> Lock(PositionMutex);

  return
    {
      static_cast<INT32>(std::floor(Position.x / chunk::ChunkSizeX)),
      static_cast<INT32>(std::floor(Position.z / chunk::ChunkSizeZ))
    };
}
