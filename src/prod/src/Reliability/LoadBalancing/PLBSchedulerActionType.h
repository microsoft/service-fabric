// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        namespace PLBSchedulerActionType
        {
            enum Enum
            {
                None = 0,
                NewReplicaPlacement = 1,
                NewReplicaPlacementWithMove = 2,
                ConstraintCheck = 3,
                QuickLoadBalancing = 4,
                LoadBalancing = 5,
                NoActionNeeded = 6,
                Upgrade = 7,
                ClientApiPromoteToPrimary = 8,
                ClientApiMovePrimary = 9,
                ClientApiMoveSecondary = 10,
                LastValidEnum = ClientApiMoveSecondary
            };

            std::wstring ToString(PLBSchedulerActionType::Enum shedulerActionType);

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(PLBSchedulerActionType);
        }
    }
}
