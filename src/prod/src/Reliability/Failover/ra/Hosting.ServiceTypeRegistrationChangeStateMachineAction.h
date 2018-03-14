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
            class ServiceTypeRegistrationChangeStateMachineAction : public Infrastructure::StateMachineAction
            {
                DENY_COPY(ServiceTypeRegistrationChangeStateMachineAction);

            public:
                ServiceTypeRegistrationChangeStateMachineAction(
                    Hosting2::ServiceTypeRegistrationSPtr const & registration,
                    bool isAdd);

                void OnPerformAction(
                    std::wstring const & activityId,
                    Infrastructure::EntityEntryBaseSPtr const & entity,
                    ReconfigurationAgent & reconfigurationAgent);

                void OnCancelAction(ReconfigurationAgent & ra);

            private:
                bool isAdd_;
                Hosting2::ServiceTypeRegistrationSPtr registration_;
            };
        }
    }
}


