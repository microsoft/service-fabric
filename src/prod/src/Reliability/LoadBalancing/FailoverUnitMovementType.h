// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        namespace FailoverUnitMovementType
        {
            enum Enum
            {
                SwapPrimarySecondary = 0,
                MoveSecondary = 1,
                MovePrimary = 2,
                AddPrimary = 3,
                AddSecondary = 4,
                PromoteSecondary = 5,
                Void = 6,
                Drop = 7,
                LastValidEnum = Drop
            };

            std::wstring ToString(FailoverUnitMovementType::Enum failoverUnitMovementType);

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(FailoverUnitMovementType);
        }
    }
}
