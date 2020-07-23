// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    struct OrderLineStoreUtil
    {
        static void CreateStore(
            __in const Data::StateManager::FactoryArguments& factoryArg,
            __in FABRIC_REPLICA_ID replicaId,
            __in KAllocator& allocator,
            __out TxnReplicator::IStateProvider2::SPtr& stateProvider);
    };
}
