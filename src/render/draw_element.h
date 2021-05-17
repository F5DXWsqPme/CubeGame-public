#ifndef __draw_element_h_
#define __draw_element_h_

#include "ext/volk/volk.h"

#include "def.h"

/**
 * \brief Element for drawing
 */
class draw_element
{
public:
  /**
   * \brief Get secondary command buffer for drawing
   * \return Command buffer
   */
  virtual VkCommandBuffer GetCommandBuffer( VOID ) = 0;

  ///**
  // * \brief Update WVP function
  // */
  //virtual VOID UpdateWVP( VOID ) = 0;

  /**
   * \brief Element destructor
   */
  virtual ~draw_element();
};

#endif /* __draw_element_h_ */
