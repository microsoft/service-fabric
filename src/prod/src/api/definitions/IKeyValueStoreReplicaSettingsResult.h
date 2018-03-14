// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IKeyValueStoreReplicaSettingsResult
    {
    public:
        virtual ~IKeyValueStoreReplicaSettingsResult() {};

        virtual void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS &) const = 0;
    };
}
