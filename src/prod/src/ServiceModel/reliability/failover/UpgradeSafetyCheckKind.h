// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace UpgradeSafetyCheckKind
    {
        enum Enum
        {
            Invalid = 0,
            EnsureSeedNodeQuorum = 1,
            EnsurePartitionQuorum = 2,
            WaitForPrimaryPlacement = 3,
            WaitForPrimarySwap = 4,
            WaitForReconfiguration = 5,
            WaitForInBuildReplica = 6,
            EnsureAvailability = 7,
            WaitForResourceAvailability = 8
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const& value);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(EnsureSeedNodeQuorum)
            ADD_ENUM_VALUE(EnsurePartitionQuorum)
            ADD_ENUM_VALUE(WaitForPrimaryPlacement)
            ADD_ENUM_VALUE(WaitForPrimarySwap)
            ADD_ENUM_VALUE(WaitForReconfiguration)
            ADD_ENUM_VALUE(WaitForInBuildReplica)
            ADD_ENUM_VALUE(EnsureAvailability)
            ADD_ENUM_VALUE(WaitForResourceAvailability)
        END_DECLARE_ENUM_SERIALIZER()
    }
}
