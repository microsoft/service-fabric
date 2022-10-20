// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    namespace TargetReplicaSelector
    {
        enum Enum
        {
            Default = 0,
            RandomInstance = 0,
            PrimaryReplica = 0,
            RandomReplica = 1,
            RandomSecondaryReplica = 2
        };

        //
        // Enumvalue to string doesnt work for all strings if the same enum value maps to multiple
        // strings. For this particular enum, this is just used to map string to enum in application
        // gateway, so it is ok.
        //
        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Default)
            ADD_ENUM_VALUE(RandomInstance)
            ADD_ENUM_VALUE(PrimaryReplica)
            ADD_ENUM_VALUE(RandomReplica)
            ADD_ENUM_VALUE(RandomSecondaryReplica)
        END_DECLARE_ENUM_SERIALIZER()
    }
}
