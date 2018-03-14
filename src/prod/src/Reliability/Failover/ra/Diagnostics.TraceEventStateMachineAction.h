// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Common/macro.h"
#include "Reliability/Failover/FailoverPointers.h"
#include "Infrastructure.StateMachineAction.h"
#include "Diagnostics.IEventWriter.h"
#include "ReconfigurationAgent.h"


namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Diagnostics
        {
            template <typename T>
            class TraceEventStateMachineAction : public Infrastructure::StateMachineAction
            {
                DENY_COPY(TraceEventStateMachineAction);
            public:
                
                TraceEventStateMachineAction(T && eventData) :
                    eventData_(move(eventData))
                {
                }

                void OnPerformAction(
                    std::wstring const & activityId,
                    Infrastructure::EntityEntryBaseSPtr const & entity,
                    ReconfigurationAgent & reconfigurationAgent)
                {
                    UNREFERENCED_PARAMETER(activityId);
                    UNREFERENCED_PARAMETER(entity);
                    reconfigurationAgent.EventWriter.Trace(move(eventData_));
                }

                void OnCancelAction(ReconfigurationAgent&)
                {
                }

            private:
                T eventData_;
            };
        }
    }
}


