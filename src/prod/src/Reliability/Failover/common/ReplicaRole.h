// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicaRole
    {
        // Value represents role of a replica
        enum Enum
        {
            Unknown = 0,
            None = 1,
            Idle = 2,
            Secondary = 3,
            Primary = 4,
            LastValidEnum = Primary
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

        ::FABRIC_REPLICA_ROLE ConvertToPublicReplicaRole(ReplicaRole::Enum toConvert);
        
        DECLARE_ENUM_STRUCTURED_TRACE(ReplicaRole);
    }
}
