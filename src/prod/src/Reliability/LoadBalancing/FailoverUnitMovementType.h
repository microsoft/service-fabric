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
                MoveInstance = 3,
                AddPrimary = 4,
                AddSecondary = 5,
                AddInstance = 6,
                PromoteSecondary = 7,
                RequestedPlacementNotPossible = 8,
                DropPrimary = 9,
                DropSecondary = 10,
                DropInstance = 11,
                LastValidEnum = DropInstance
            };

            std::wstring ToString(FailoverUnitMovementType::Enum failoverUnitMovementType);

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(FailoverUnitMovementType);
        }
    }
}
