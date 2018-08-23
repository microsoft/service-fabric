// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Store;
using namespace Reliability::ReplicationComponent;

KeyValueStoreReplicaFactory::KeyValueStoreReplicaFactory()
    : IKeyValueStoreReplicaFactory()
    , ComponentRoot()
{
}

KeyValueStoreReplicaFactory::~KeyValueStoreReplicaFactory()
{
}

ErrorCode KeyValueStoreReplicaFactory::CreateKeyValueStoreReplica(
    wstring const & storeName,
    Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    wstring const & serviceName,
    FABRIC_REPLICATOR_SETTINGS const & replicatorSettings,
    FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS const & kvsPublicSettings,
    FABRIC_LOCAL_STORE_KIND storeKind,
    void * publicLocalStoreSettings,
    IStoreEventHandlerPtr const & storeEventHandler,
    ISecondaryEventHandlerPtr const & secondaryEventHandler,
    IClientFactoryPtr const & clientFactory,
    __out IKeyValueStoreReplicaPtr & kvs)
{
    if (StoreConfig::GetConfig().Test_EnableTStore)
    {
        return ForceCreateKeyValueStoreReplica_V2(
            partitionId,
            replicaId,
            replicatorSettings,
            kvsPublicSettings,
            storeKind,
            publicLocalStoreSettings,
            storeEventHandler,
            secondaryEventHandler,
            kvs);
    }
    else
    {
        return CreateKeyValueStoreReplica_V1(
            storeName,
            partitionId,
            replicaId,
            serviceName,
            replicatorSettings,
            kvsPublicSettings,
            storeKind,
            publicLocalStoreSettings,
            storeEventHandler,
            secondaryEventHandler,
            clientFactory,
            kvs);
    }
}

ErrorCode KeyValueStoreReplicaFactory::CreateKeyValueStoreReplica_V1(
    wstring const & storeName,
    Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    wstring const & serviceName,
    FABRIC_REPLICATOR_SETTINGS const & replicatorSettings,
    FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS const & kvsPublicSettings,
    FABRIC_LOCAL_STORE_KIND storeKind,
    void * publicLocalStoreSettings,
    IStoreEventHandlerPtr const & storeEventHandler,
    ISecondaryEventHandlerPtr const & secondaryEventHandler,
    IClientFactoryPtr const & clientFactory,
    __out IKeyValueStoreReplicaPtr & kvs)
{
    if (storeName.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    ReplicatorSettingsUPtr internalReplicatorSettings;
    auto error = ReplicatorSettings::FromPublicApi(replicatorSettings, internalReplicatorSettings);
    if (!error.IsSuccess()) 
    { 
        return error; 
    }

    if (storeKind != FABRIC_LOCAL_STORE_KIND::FABRIC_LOCAL_STORE_KIND_ESE)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    EseLocalStoreSettings eseSettings;
    error = GetEseLocalStoreSettings(storeKind, publicLocalStoreSettings, eseSettings);
    if (!error.IsSuccess()) { return error; }

    eseSettings.StoreName = storeName;

    ReplicatedStoreSettings replicatedStoreSettings;
    error = replicatedStoreSettings.FromPublicApi(kvsPublicSettings);

    if (!error.IsSuccess()) { return error; }

    replicatedStoreSettings.EnableFlushOnDrain = StoreConfig::GetConfig().EnableUserServiceFlushOnDrain;
    replicatedStoreSettings.EnumerationPerfTraceThreshold = StoreConfig::GetConfig().UserServiceEnumerationPerfTraceThreshold;
    replicatedStoreSettings.EnsureReplicatorSettings(
        PartitionedReplicaId(partitionId, replicaId).TraceId,
        internalReplicatorSettings);

    return KeyValueStoreReplica::CreateForPublicStack_V1(
        partitionId, 
        replicaId,
        move(internalReplicatorSettings),
        replicatedStoreSettings,
        eseSettings,
        storeEventHandler,
        secondaryEventHandler,
        clientFactory,
        serviceName,
        kvs); // out
}

ErrorCode KeyValueStoreReplicaFactory::ForceCreateKeyValueStoreReplica_V2(
    Common::Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    FABRIC_REPLICATOR_SETTINGS const & replicatorSettings,
    FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS const & kvsPublicSettings,
    FABRIC_LOCAL_STORE_KIND storeKind,
    void * publicLocalStoreSettings,
    Api::IStoreEventHandlerPtr const & storeEventHandler,
    Api::ISecondaryEventHandlerPtr const & secondaryEventHandler,
    __out Api::IKeyValueStoreReplicaPtr & kvs)
{
    EseLocalStoreSettings eseSettings;
    auto error = GetEseLocalStoreSettings(storeKind, publicLocalStoreSettings, eseSettings);
    if (!error.IsSuccess()) { return error; }

    auto defaultKvsSettings = TSReplicatedStoreSettings::GetKeyValueStoreReplicaDefaultSettings(
        eseSettings.DbFolderPath,
        L"", // sharedLogDirectory
        L"", // sharedLogFilename
        Guid::Empty()); //sharedLogGuid

    ScopedHeap heap;
    FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 storeSettings = {0};

    defaultKvsSettings->ToPublicApi(heap, storeSettings);

    storeSettings.SecondaryNotificationMode = kvsPublicSettings.SecondaryNotificationMode;

    return CreateKeyValueStoreReplica_V2(
        partitionId,
        replicaId,
        storeSettings,
        replicatorSettings,
        storeEventHandler,
        secondaryEventHandler,
        kvs);
}

ErrorCode KeyValueStoreReplicaFactory::CreateKeyValueStoreReplica_V2(
    Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 const & publicStoreSettings,
    FABRIC_REPLICATOR_SETTINGS const & publicReplicatorSettings,
    IStoreEventHandlerPtr const & storeEventHandler,
    ISecondaryEventHandlerPtr const & secondaryEventHandler,
    __out IKeyValueStoreReplicaPtr & kvs)
{
    TSReplicatedStoreSettingsUPtr storeSettings;
    auto error = TSReplicatedStoreSettings::FromPublicApi(publicStoreSettings, storeSettings);
    if (!error.IsSuccess()) { return error; }

    ReplicatorSettingsUPtr replicatorSettings;
    error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, replicatorSettings);
    if (!error.IsSuccess()) { return error; }

    return KeyValueStoreReplica::CreateForPublicStack_V2(
        partitionId, 
        replicaId,
        move(replicatorSettings),
        move(storeSettings),
        storeEventHandler,
        secondaryEventHandler,
        kvs); // out
}

ErrorCode KeyValueStoreReplicaFactory::GetEseLocalStoreSettings(
    FABRIC_LOCAL_STORE_KIND storeKind,
    void * publicLocalStoreSettings,
    __out EseLocalStoreSettings & eseSettings)
{
    if (storeKind != FABRIC_LOCAL_STORE_KIND::FABRIC_LOCAL_STORE_KIND_ESE)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    if (publicLocalStoreSettings != NULL)
    {
        return eseSettings.FromPublicApi(*static_cast<FABRIC_ESE_LOCAL_STORE_SETTINGS*>(publicLocalStoreSettings));
    }
    else
    {
        return ErrorCodeValue::Success;
    }
}
