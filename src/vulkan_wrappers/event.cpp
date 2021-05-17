#include "event.h"
#include "vulkan_validation.h"

/**
 * \brief Event constructor
 * \param[in] Device Device identifier
 * \parram[in] IsSignaled Event signaled flag.
 */
event::event( VkDevice Device ) : DeviceId(Device)
{
  VkEventCreateInfo CreateInfo = {};

  CreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
  CreateInfo.pNext = nullptr;
  CreateInfo.flags = 0;

  vulkan_validation::Check(
    vkCreateEvent(DeviceId, &CreateInfo, nullptr, &EventId),
    "Event creation failed");
}

/**
 * \brief Event destructor
 */
event::~event( VOID )
{
  if (EventId != VK_NULL_HANDLE)
    vkDestroyEvent(DeviceId, EventId, nullptr);
}

/**
 * \brief Get event identifier function
 * \return Event identifier
 */
VkEvent event::GetEventId( VOID ) const noexcept
{
  return EventId;
}

/**
 * \brief Move function
 * \param[in] Event Vulkan event
 * \return Reference to this
 */
event & event::operator=( event &&Event ) noexcept
{
  std::swap(EventId, Event.EventId);
  std::swap(DeviceId, Event.DeviceId);

  return *this;
}

/**
 * \brief Move constructor
 * \param[in] Event Vulkan event
 * \return Reference to this
 */
event::event( event &&Event ) noexcept
{
  std::swap(EventId, Event.EventId);
  std::swap(DeviceId, Event.DeviceId);
}

/**
 * \brief Set event function
 */
VOID event::Set( VOID ) const
{
  vulkan_validation::Check(
    vkSetEvent(DeviceId, EventId),
    "Event set failed");
}

/**
 * \brief Reset event function
 */
VOID event::Reset( VOID ) const
{
  vulkan_validation::Check(
    vkResetEvent(DeviceId, EventId),
    "Event set failed");
}
