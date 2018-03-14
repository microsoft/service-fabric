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
                Creation = 1,
                CreationWithMove = 2,
                ConstraintCheck = 3,
                FastBalancing = 4,
                SlowBalancing = 5,
                NoActionNeeded = 6,
                Upgrade = 7,
                ClientPromoteToPrimaryApiCall = 8,
                ClientMovePrimaryApiCall = 9,
                ClientMoveSecondaryApiCall = 10,
                LastValidEnum = ClientMoveSecondaryApiCall
            };

            std::wstring ToString(PLBSchedulerActionType::Enum shedulerActionType);

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(PLBSchedulerActionType);
        }
    }
}
