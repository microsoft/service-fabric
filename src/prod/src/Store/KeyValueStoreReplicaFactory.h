// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class KeyValueStoreReplicaFactory : 
        public Api::IKeyValueStoreReplicaFactory,
        public Common::ComponentRoot
    {
        DENY_COPY(KeyValueStoreReplicaFactory);

    public:
        KeyValueStoreReplicaFactory();
        virtual ~KeyValueStoreReplicaFactory();

        //
        // IKeyValueStoreReplicaFactory methods
        // 
        Common::ErrorCode CreateKeyValueStoreReplica(
            std::wstring const & storeName,
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId,
            std::wstring const & serviceName,
            FABRIC_REPLICATOR_SETTINGS const & replicatorSettings,
            FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS const & kvsSettings,
            FABRIC_LOCAL_STORE_KIND localStoreKind,
            void * localStoreSettings,
            Api::IStoreEventHandlerPtr const & storeEventHandler,
            Api::ISecondaryEventHandlerPtr const & secondaryEventHandler,
            Api::IClientFactoryPtr const & clientFactory,
            __out Api::IKeyValueStoreReplicaPtr & keyValueStoreReplica) override;

        Common::ErrorCode CreateKeyValueStoreReplica_V2(
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId,
            FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 const & kvsSettings,
            FABRIC_REPLICATOR_SETTINGS const & replicatorSettings,
            Api::IStoreEventHandlerPtr const & storeEventHandler,
            Api::ISecondaryEventHandlerPtr const & secondaryEventHandler,
            __out Api::IKeyValueStoreReplicaPtr & keyValueStoreReplica) override;

    private:

        Common::ErrorCode CreateKeyValueStoreReplica_V1(
            std::wstring const & storeName,
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId,
            std::wstring const & serviceName,
            FABRIC_REPLICATOR_SETTINGS const & replicatorSettings,
            FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS const & kvsSettings,
            FABRIC_LOCAL_STORE_KIND localStoreKind,
            void * localStoreSettings,
            Api::IStoreEventHandlerPtr const & storeEventHandler,
            Api::ISecondaryEventHandlerPtr const & secondaryEventHandler,
            Api::IClientFactoryPtr const & clientFactory,
            __out Api::IKeyValueStoreReplicaPtr & keyValueStoreReplica);

        Common::ErrorCode ForceCreateKeyValueStoreReplica_V2(
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId,
            FABRIC_REPLICATOR_SETTINGS const & replicatorSettings,
            FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS const & kvsPublicSettings,
            FABRIC_LOCAL_STORE_KIND storeKind,
            void * publicLocalStoreSettings,
            Api::IStoreEventHandlerPtr const & storeEventHandler,
            Api::ISecondaryEventHandlerPtr const & secondaryEventHandler,
            __out Api::IKeyValueStoreReplicaPtr & keyValueStoreReplica);

        Common::ErrorCode GetEseLocalStoreSettings(
            FABRIC_LOCAL_STORE_KIND storeKind,
            void * publicLocalStoreSettings,
            __out EseLocalStoreSettings &);
    };
}
