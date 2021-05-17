#include <chrono>

#include "event_loop.h"

/**
 * \brief Event loop constructor
 * \param[in] LogicDelay Delay in logic cycle
 */
event_loop::event_loop( FLT LogicDelay ) : LogicDelay(LogicDelay)
{
}

/**
 * \brief Register event which will call every loop iteration
 * \param[in] Event Event for registration
 */
VOID event_loop::RegisterLoopEvent( const std::function<VOID ( FLT Time )> &Event )
{
  Events.push_back(Event);
}

/**
 * \brief Register event which will call with logic interval
 * \param[in] Event Event for registration
 */
VOID event_loop::RegisterLogicLoopEvent( const std::function<VOID ( FLT Time )> &Event )
{
  LogicEvents.push_back(Event);
}

/**
 * \brief Register exit predicate
 * \param[in] Predicate Predicate for evaluation every loop iteration
 */
VOID event_loop::RegisterExitPredicate( const std::function<BOOL ( FLT Time )> &Predicate )
{
  ExitPredicates.push_back(Predicate);
}

/**
 * \brief Register exit predicate Run function
 */
VOID event_loop::Run( VOID ) const
{
  std::chrono::time_point StartTime = std::chrono::steady_clock::now();
  std::chrono::duration<FLT> CurTimeDuration = std::chrono::steady_clock::now() - StartTime;
  FLT CurTime = CurTimeDuration.count(); //(std::chrono::steady_clock::now() - StartTime) / std::chrono::seconds(1);
  FLT OldLogicTime = CurTime;

  while (!IsExit(CurTime))
  {
    for (const std::function<VOID ( FLT Time )> &Event : Events)
      Event(CurTime);

    while (CurTime - OldLogicTime > LogicDelay)
    {
      for (const std::function<VOID ( FLT Time )> &Event : LogicEvents)
        Event(CurTime);

      OldLogicTime += LogicDelay;
    }

    CurTimeDuration = std::chrono::steady_clock::now() - StartTime;
    CurTime = CurTimeDuration.count();
  }
}

/**
 * \brief Evaluate and check all exit predicates
 * \param[in] Time Current time
 */
BOOL event_loop::IsExit( FLT Time ) const
{
  for (const std::function<BOOL ( FLT Time )> &Predicate : ExitPredicates)
    if (Predicate(Time))
      return TRUE;

  return FALSE;
}
