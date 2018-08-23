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
            class HostingEventHandler
            {
                DENY_COPY(HostingEventHandler);
            public:
                HostingEventHandler(ReconfigurationAgent & ra);

                void ProcessServiceTypeRegistered(Hosting2::ServiceTypeRegistrationSPtr const & registration);

                void ProcessRuntimeClosed(std::wstring const & hostId, std::wstring const & runtimeId);

                void ProcessAppHostClosed(std::wstring const & hostId, Common::ActivityDescription const & activityDescription);

            private:
                bool CanProcessEvent(std::wstring const & activityId, bool doesEventCauseReplicaOpen);                

                std::wstring CreateActivityId() const;

                ReconfigurationAgent & ra_;
            };
        }
    }
}


