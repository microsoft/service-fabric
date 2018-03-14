// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;


// ********************************************************************************************************************
// ComKeyValueStoreReplicaFactory::ComKeyValueStoreReplicaFactory Implementation
//

ComKeyValueStoreReplicaFactory::ComKeyValueStoreReplicaFactory(IKeyValueStoreReplicaFactoryPtr const & impl)
    : impl_(impl)
{
}

ComKeyValueStoreReplicaFactory::~ComKeyValueStoreReplicaFactory()
{
}

/* [entry] */ HRESULT ComKeyValueStoreReplicaFactory::FabricCreateKeyValueStoreReplica( 
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in LPCWSTR storeName,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId,
    /* [in] */ __RPC__in FABRIC_URI serviceName,
    /* [in] */ __RPC__in const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
    /* [in] */ __RPC__in const FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS *kvsPublicSettings,
    /* [in] */ FABRIC_LOCAL_STORE_KIND localStoreKind,
    /* [in] */ __RPC__in void *localStoreSettings,
    /* [in] */ __RPC__in_opt IFabricStoreEventHandler *storeEventHandler,
    /* [in] */ __RPC__in_opt IFabricSecondaryEventHandler *secondaryEventHandler,
    /* [in] */ IClientFactoryPtr const & clientFactory,
    /* [retval][out] */ __RPC__deref_out_opt void **keyValueStore)
{
    UNREFERENCED_PARAMETER(riid);

    if (storeName == NULL || keyValueStore == NULL) 
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    FABRIC_REPLICATOR_SETTINGS validatedReplicatorSettings = { 0 };
    auto hr = GetValidatedSettings(replicatorSettings, validatedReplicatorSettings);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS kvsInternalSettings = { 0 };
    if (kvsPublicSettings != NULL)
    {
        kvsInternalSettings = *kvsPublicSettings;
    }

    TRY_COM_PARSE_PUBLIC_STRING( storeName );
    TRY_COM_PARSE_PUBLIC_STRING_ALLOW_NULL( serviceName );

    // Assumption is that only Service Fabric implements IStoreEventHandler.
    if (storeEventHandler == NULL)  { return ComUtility::OnPublicApiReturn(E_POINTER); }

    ComPointer<IFabricStoreEventHandler2> fabricStoreEventHandler2CPtr;
    hr = storeEventHandler->QueryInterface(IID_IFabricStoreEventHandler2, fabricStoreEventHandler2CPtr.VoidInitializationAddress());
    if (!SUCCEEDED(hr) && hr != E_NOINTERFACE) { return ComUtility::OnPublicApiReturn(hr); }

    IKeyValueStoreReplicaPtr keyValueStoreReplicaImpl;
    auto error = impl_->CreateKeyValueStoreReplica(
        parsed_storeName,
        Guid(partitionId),
        replicaId,
        parsed_serviceName,
        validatedReplicatorSettings,
        kvsInternalSettings,
        localStoreKind,
        localStoreSettings,
        fabricStoreEventHandler2CPtr.GetRawPointer() != NULL 
            ? WrapperFactory::create_com_proxy_wrapper(fabricStoreEventHandler2CPtr.GetRawPointer())
            : WrapperFactory::create_com_proxy_wrapper(storeEventHandler),
        WrapperFactory::create_rooted_com_proxy(secondaryEventHandler),
        clientFactory,
        keyValueStoreReplicaImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    ComPointer<IFabricKeyValueStoreReplica> keyValueStoreReplicaCPtr = 
        WrapperFactory::create_com_wrapper(keyValueStoreReplicaImpl);
    *keyValueStore = keyValueStoreReplicaCPtr.DetachNoRelease();

    return S_OK;
}

/* [entry] */ HRESULT ComKeyValueStoreReplicaFactory::FabricCreateKeyValueStoreReplica_V2( 
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId,
    /* [in] */ __RPC__in const FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 *storeSettings,
    /* [in] */ __RPC__in const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
    /* [in] */ __RPC__in_opt IFabricStoreEventHandler *storeEventHandler,
    /* [in] */ __RPC__in_opt IFabricSecondaryEventHandler *secondaryEventHandler,
    /* [retval][out] */ __RPC__deref_out_opt void **keyValueStore)
{
    UNREFERENCED_PARAMETER(riid);

    if (keyValueStore == NULL) 
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 validatedStoreSettings = { 0 };
    if (storeSettings != NULL)
    {
        validatedStoreSettings = *storeSettings;
    }

    FABRIC_REPLICATOR_SETTINGS validatedReplicatorSettings = { 0 };
    auto hr = GetValidatedSettings(replicatorSettings, validatedReplicatorSettings);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    // Assumption is that only Service Fabric implements IStoreEventHandler.
    if (storeEventHandler == NULL)  { return ComUtility::OnPublicApiReturn(E_POINTER); }

    ComPointer<IFabricStoreEventHandler2> fabricStoreEventHandler2CPtr;
    hr = storeEventHandler->QueryInterface(IID_IFabricStoreEventHandler2, fabricStoreEventHandler2CPtr.VoidInitializationAddress());
    if (!SUCCEEDED(hr) && hr != E_NOINTERFACE) { return ComUtility::OnPublicApiReturn(hr); }

    IKeyValueStoreReplicaPtr keyValueStoreReplicaImpl;
    auto error = impl_->CreateKeyValueStoreReplica_V2(
        Guid(partitionId),
        replicaId,
        validatedStoreSettings,
        validatedReplicatorSettings,
        fabricStoreEventHandler2CPtr.GetRawPointer() != NULL 
            ? WrapperFactory::create_com_proxy_wrapper(fabricStoreEventHandler2CPtr.GetRawPointer())
            : WrapperFactory::create_com_proxy_wrapper(storeEventHandler),
        WrapperFactory::create_rooted_com_proxy(secondaryEventHandler),
        keyValueStoreReplicaImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    ComPointer<IFabricKeyValueStoreReplica> keyValueStoreReplicaCPtr = WrapperFactory::create_com_wrapper(keyValueStoreReplicaImpl);
    *keyValueStore = keyValueStoreReplicaCPtr.DetachNoRelease();

    return S_OK;
}

HRESULT ComKeyValueStoreReplicaFactory::GetValidatedSettings(
    const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
    __out FABRIC_REPLICATOR_SETTINGS & result)
{
    if (replicatorSettings != NULL)
    {
        FABRIC_REPLICATOR_SETTINGS_EX1 * replicatorSettingsEx1 = (FABRIC_REPLICATOR_SETTINGS_EX1 *) replicatorSettings->Reserved;

        if (replicatorSettingsEx1 != NULL)
        {
            FABRIC_REPLICATOR_SETTINGS_EX2 * replicatorSettingsEx2 = (FABRIC_REPLICATOR_SETTINGS_EX2 *)replicatorSettingsEx1->Reserved;

            if (replicatorSettingsEx2 != NULL)
            {
                FABRIC_REPLICATOR_SETTINGS_EX3 * replicatorSettingsEx3 = (FABRIC_REPLICATOR_SETTINGS_EX3 *)replicatorSettingsEx2->Reserved;

                if (replicatorSettingsEx3 != NULL)
                {
                    FABRIC_REPLICATOR_SETTINGS_EX4 * replicatorSettingsEx4 = (FABRIC_REPLICATOR_SETTINGS_EX4 *)replicatorSettingsEx3->Reserved;
                    FAIL_IF_OPTIONAL_PARAM_RESERVED_FIELD_NOT_NULL(replicatorSettingsEx4);
                }
            }
        }

        result = *replicatorSettings;
    }

    return S_OK;
}
