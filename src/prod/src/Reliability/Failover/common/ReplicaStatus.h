// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicaStatus
    {
        // Value represents status of a replica
        enum Enum
        {
            Invalid = 0,
            Down = 1,
            Up = 2,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

        ::FABRIC_REPLICA_STATUS ConvertToPublicReplicaStatus(ReplicaStatus::Enum toConvert);
    }
}
