// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IKeyValueStoreReplicaSettings_V2Result
    {
    public:
        virtual ~IKeyValueStoreReplicaSettings_V2Result() {};

        virtual void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 &) const = 0;
    };
}

