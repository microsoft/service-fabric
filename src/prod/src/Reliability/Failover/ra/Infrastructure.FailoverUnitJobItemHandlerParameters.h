// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            class HandlerParameters : public HandlerParametersT<FailoverUnit>
            {
            public:
                HandlerParameters(
                    std::string const & traceId,
                    LockedFailoverUnitPtr & ft,
                    ReconfigurationAgent & ra,
                    StateMachineActionQueue & actionQueue,
                    MultipleEntityWork const * work,
                    std::wstring const & activityId)
                    : HandlerParametersT(traceId, ft, ra, actionQueue, work, activityId)
                {
                }

                __declspec(property(get = get_FT)) LockedFailoverUnitPtr & FailoverUnit;
                LockedFailoverUnitPtr & get_FT() const { return *entity_; }  

                void AssertFTIsOpen() const
                {
                    ASSERT_IF(entity_->get_Current()->State == FailoverUnitStates::Closed, "FT must be open for this job item. JobItemChecks is incorrect {0}", *entity_->get_Current());
                }

                void AssertFTExists() const
                {
                    ASSERT_IF(entity_->get_Current() == nullptr, "FT must exist for this job item. JobItemCheck is incorrect {0}", *entity_->get_Current());
                }
            };
        }
    }
}

