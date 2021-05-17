#ifndef __event_loop_h_
#define __event_loop_h_

#include <functional>
#include <vector>

#include "def.h"

/**
 * \brief Event loop class
 */
class event_loop
{
public:
  /**
   * \brief Event loop constructor
   * \param[in] LogicDelay Delay in logic cycle
   */
  event_loop( FLT LogicDelay = 0.05 );

  /**
   * \brief Register event which will call every loop iteration
   * \param[in] Event Event for registration
   */
  VOID RegisterLoopEvent( const std::function<VOID ( FLT Time )> &Event );

  /**
   * \brief Register event which will call with logic interval
   * \param[in] Event Event for registration
   */
  VOID RegisterLogicLoopEvent( const std::function<VOID ( FLT Time )> &Event );

  /**
   * \brief Register exit predicate
   * \param[in] Predicate Predicate for evaluation every loop iteration
   */
  VOID RegisterExitPredicate( const std::function<BOOL ( FLT Time )> &Predicate );

  /**
   * \brief Register exit predicate Run function
   */
  VOID Run( VOID ) const;

private:
  /**
   * \brief Evaluate and check all exit predicates
   * \param[in] Time Current time
   */
  BOOL IsExit( FLT Time ) const;

  /** Delay in logic cycle */
  FLT LogicDelay = 0.05;

  /** Loop events */
  std::vector<std::function<VOID ( FLT Time )>> Events;

  /** Logic events */
  std::vector<std::function<VOID ( FLT Time )>> LogicEvents;

  /** Exit predicates */
  std::vector<std::function<BOOL ( FLT Time )>> ExitPredicates;
};

#endif /* __event_loop_h_ */
