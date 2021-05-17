#ifndef __event_h_
#define __event_h_

#include "ext/volk/volk.h"

#include "def.h"

/**
 * \brief Event class
 */
class event
{
public:
  /**
   * \brief Default constructor.
   */
  event( VOID ) = default;

  /**
   * \brief Event constructor
   * \param[in] Device Device identifier
   * \parram[in] IsSignaled Event signaled flag.
   */
  event( VkDevice Device );

  /**
   * \brief Event destructor
   */
  ~event( VOID );

  /**
   * \brief Get event identifier function
   * \return Event identifier
   */
  VkEvent GetEventId( VOID ) const noexcept;

  /**
   * \brief Move function
   * \param[in] Event Vulkan event
   * \return Reference to this
   */
  event & operator=( event &&Event ) noexcept;

  /**
   * \brief Move constructor
   * \param[in] Event Vulkan event
   * \return Reference to this
   */
  event( event &&Event ) noexcept;

  /**
   * \brief Set event function
   */
  VOID Set( VOID ) const;

  /**
   * \brief Reset event function
   */
  VOID Reset( VOID ) const;

private:
  /**
   * \brief Removed copy function
   * \param[in] Event Vulkan event
   * \return Reference to this
   */
  event & operator=( const event &Event ) = delete;

  /**
   * \brief Removed copy constructor
   * \param[in] Event Vulkan event
   * \return Reference to this
   */
  event( const event &Fence ) = delete;

  /** Event identifier */
  VkEvent EventId = VK_NULL_HANDLE;

  /** Device identifier */
  VkDevice DeviceId = VK_NULL_HANDLE;
};

#endif /* __event_h_ */
