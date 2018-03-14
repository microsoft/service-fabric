// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Communication
        {
            class RequestFMMessageRetryAction : public Infrastructure::StateMachineAction
            {
            public:
                RequestFMMessageRetryAction(Reliability::FailoverManagerId const & target);

            private:
                void OnPerformAction(
                    std::wstring const & activityId,
                    Infrastructure::EntityEntryBaseSPtr const & entity,
                    ReconfigurationAgent & reconfigurationAgent) override;

                void OnCancelAction(
                    ReconfigurationAgent & ra) override;

                Reliability::FailoverManagerId target_;
            };
        }
    }
}


