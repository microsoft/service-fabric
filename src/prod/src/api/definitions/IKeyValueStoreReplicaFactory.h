// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IKeyValueStoreReplicaFactory
    {
    public:
        virtual ~IKeyValueStoreReplicaFactory() {};

        virtual Common::ErrorCode CreateKeyValueStoreReplica(
            std::wstring const & storeName,
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId,
            std::wstring const & serviceName,
            FABRIC_REPLICATOR_SETTINGS const & replicatorSettings,
            FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS const & kvsSettings,
            FABRIC_LOCAL_STORE_KIND localStoreKind,
            void * localStoreProviderSettings,
            IStoreEventHandlerPtr const & storeEventHandler,
            ISecondaryEventHandlerPtr const & secondaryEventHandler,
            IClientFactoryPtr const &,
            __out IKeyValueStoreReplicaPtr & keyValueStoreReplica) = 0;

        virtual Common::ErrorCode CreateKeyValueStoreReplica_V2(
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId,
            FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 const & kvsSettings,
            FABRIC_REPLICATOR_SETTINGS const & replicatorSettings,
            Api::IStoreEventHandlerPtr const & storeEventHandler,
            Api::ISecondaryEventHandlerPtr const & secondaryEventHandler,
            __out Api::IKeyValueStoreReplicaPtr & keyValueStoreReplica) = 0;
    };
}
