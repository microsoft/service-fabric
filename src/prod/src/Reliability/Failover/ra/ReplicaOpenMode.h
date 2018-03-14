// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicaOpenMode
    {
        enum Enum
        {
            Invalid = 0,
            New = 1,
            Existing = 2,

            LastValidEnum = Existing
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

        ::FABRIC_REPLICA_OPEN_MODE ToPublicApi(ReplicaOpenMode::Enum toConvert);

        DECLARE_ENUM_STRUCTURED_TRACE(ReplicaOpenMode);
    }
}
