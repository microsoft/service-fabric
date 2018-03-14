// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComKeyValueStoreReplicaFactory
    {
    public:
        ComKeyValueStoreReplicaFactory(IKeyValueStoreReplicaFactoryPtr const & impl);
        virtual ~ComKeyValueStoreReplicaFactory();

        /* [entry] */ HRESULT FabricCreateKeyValueStoreReplica( 
            /* [in] */ __RPC__in REFIID riid,
            /* [in] */ __RPC__in LPCWSTR storeName,
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_REPLICA_ID replicaId,
            /* [in] */ __RPC__in FABRIC_URI serviceName,
            /* [in] */ __RPC__in const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
            /* [in] */ __RPC__in const FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS *kvsSettings,
            /* [in] */ FABRIC_LOCAL_STORE_KIND localStoreKind,
            /* [in] */ __RPC__in void *localStoreSettings,
            /* [in] */ __RPC__in_opt IFabricStoreEventHandler *storeEventHandler,
            /* [in] */ __RPC__in_opt IFabricSecondaryEventHandler *secondaryEventHandler,
            /* [in] */ IClientFactoryPtr const & clientFactory,
            /* [retval][out] */ __RPC__deref_out_opt void **keyValueStore);
        
        /* [entry] */ HRESULT FabricCreateKeyValueStoreReplica_V2( 
            /* [in] */ __RPC__in REFIID riid,
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_REPLICA_ID replicaId,
            /* [in] */ __RPC__in const FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 *kvsSettings,
            /* [in] */ __RPC__in const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
            /* [in] */ __RPC__in_opt IFabricStoreEventHandler *storeEventHandler,
            /* [in] */ __RPC__in_opt IFabricSecondaryEventHandler *secondaryEventHandler,
            /* [retval][out] */ __RPC__deref_out_opt void **keyValueStore);
        
    private:
        HRESULT GetValidatedSettings(const FABRIC_REPLICATOR_SETTINGS *replicatorSettings, __out FABRIC_REPLICATOR_SETTINGS &);

        IKeyValueStoreReplicaFactoryPtr impl_;
    };
}
