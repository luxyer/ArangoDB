////////////////////////////////////////////////////////////////////////////////
/// @brief tasks used to handle timer events
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2010-2011 triagens GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
/// @author Achim Brandt
/// @author Copyright 2008-2011, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIAGENS_FYN_REST_TIMER_TASK_H
#define TRIAGENS_FYN_REST_TIMER_TASK_H 1

#include "Scheduler/Task.h"

namespace triagens {
  namespace rest {

    ////////////////////////////////////////////////////////////////////////////////
    /// @ingroup Scheduler
    /// @brief task used to handle timer events
    ////////////////////////////////////////////////////////////////////////////////

    class  TimerTask : virtual public Task {
      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief constructs a new task for a timer event
        ////////////////////////////////////////////////////////////////////////////////

        explicit
        TimerTask (double seconds);

      protected:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief called when the timer is reached
        ////////////////////////////////////////////////////////////////////////////////

        virtual bool handleTimeout () = 0;

      protected:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief destructor
        ////////////////////////////////////////////////////////////////////////////////

        ~TimerTask ();

      protected:

        ////////////////////////////////////////////////////////////////////////////////
        /// {@inheritDoc}
        ////////////////////////////////////////////////////////////////////////////////

        void setup (Scheduler*, EventLoop);

        ////////////////////////////////////////////////////////////////////////////////
        /// {@inheritDoc}
        ////////////////////////////////////////////////////////////////////////////////

        void cleanup ();

        ////////////////////////////////////////////////////////////////////////////////
        /// {@inheritDoc}
        ////////////////////////////////////////////////////////////////////////////////

        bool handleEvent (EventToken token, EventType event);

      protected:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief timer event
        ////////////////////////////////////////////////////////////////////////////////

        EventToken watcher;

      protected:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief timeout
        ////////////////////////////////////////////////////////////////////////////////

        double seconds;
    };
  }
}

#endif
