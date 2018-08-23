// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "PLBSchedulerActionType.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        struct PLBSchedulerAction
        {
        public:

            PLBSchedulerAction();
            PLBSchedulerAction(const PLBSchedulerAction & other);

            __declspec (property(get=get_Action, put=set_Action)) PLBSchedulerActionType::Enum Action;
            PLBSchedulerActionType::Enum get_Action() const { return action_; }
            void set_Action(PLBSchedulerActionType::Enum action) { action_ = action; }

            __declspec (property(get=get_IsSkip, put=set_IsSkip)) bool IsSkip;
            bool get_IsSkip() const { return isSkip_; }
            void set_IsSkip(bool value) { isSkip_ = value; }

            __declspec (property(get=get_IsConstraintCheckLight)) bool IsConstraintCheckLight;
            bool get_IsConstraintCheckLight() const { return isConstraintCheckLight_; }

            void SetAction(PLBSchedulerActionType::Enum value);
            void SetConstraintCheckLightness(bool lightness);
            void Reset();
            void End();

            void IncreaseIteration();
            bool CanRetry() const;

            bool IsConstraintCheck() const { return action_ == PLBSchedulerActionType::ConstraintCheck; }
            bool IsBalancing() const { return action_ == PLBSchedulerActionType::QuickLoadBalancing || action_ == PLBSchedulerActionType::LoadBalancing; }
            bool IsCreationWithMove() const { return action_ == PLBSchedulerActionType::NewReplicaPlacementWithMove; }
            bool IsCreation() const { return action_ == PLBSchedulerActionType::NewReplicaPlacement; }

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;

            std::wstring ToQueryString() const;

        private:

            PLBSchedulerActionType::Enum action_;
            int iteration_;
            bool isSkip_;
            bool isConstraintCheckLight_;
        };
    }
}
