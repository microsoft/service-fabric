// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Hosting
        {
            class TerminateHostStateMachineAction : public Infrastructure::StateMachineAction
            {
                DENY_COPY(TerminateHostStateMachineAction);
            public:
                TerminateHostStateMachineAction(
                    TerminateServiceHostReason::Enum reason,
                    Hosting2::ServiceTypeRegistrationSPtr const & registration);

                void OnPerformAction(
                    std::wstring const & activityId, 
                    Infrastructure::EntityEntryBaseSPtr const & entity, 
                    ReconfigurationAgent & reconfigurationAgent);

                void OnCancelAction(ReconfigurationAgent &);

            private:
                Hosting2::ServiceTypeRegistrationSPtr registration_;
                TerminateServiceHostReason::Enum reason_;
            };
        }
    }
}


