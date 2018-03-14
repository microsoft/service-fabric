// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        namespace FailoverUnitHealthState
        {
            enum Enum
            {
                Invalid = 0,
                Healthy = 1,
                Placement = 2,
                PlacementStuck = 3,
                Build = 4,
                BuildStuck = 5,
                Reconfiguration = 6,
                ReconfigurationStuck = 7,
                QuorumLost = 8,
                PlacementStuckBelowMinReplicaCount = 9,
                BuildStuckBelowMinReplicaCount = 10,
                DeletionInProgress = 11,
                PlacementStuckAtZeroReplicaCount = 12,
            };

            void WriteToTextWriter(Common::TextWriter & w, FailoverUnitHealthState::Enum const & val);
        }
    }
}

