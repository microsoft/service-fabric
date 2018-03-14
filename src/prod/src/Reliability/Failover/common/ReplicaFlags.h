// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicaFlags
    {
        enum Enum
        {
            None = 0x00,
            ToBeDroppedByFM = 0x01,
            ToBeDroppedByPLB = 0x02,
            ToBePromoted = 0x04,
            PendingRemove = 0x08,
            Deleted = 0x10,
            PreferredPrimaryLocation = 0x20,
            EndpointAvailable = 0x40,
            PreferredReplicaLocation = 0x80,
            PrimaryToBeSwappedOut = 0x100,
            PrimaryToBePlaced = 0x200,
            ReplicaToBePlaced = 0x400,
            MoveInProgress = 0x800,
            ToBeDroppedForNodeDeactivation = 0x1000
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
    }
}
