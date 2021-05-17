#ifndef __render_synchronization_h_
#define __render_synchronization_h_

#include <mutex>

#include "def.h"

/**
 * \brief Class with render synchronization objects
 */
class render_synchronization
{
public:
  /** Mutex for main render objects */
  std::mutex RenderMutex;
};

#endif /* __render_synchronization_h_ */