// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace RepairPolicyEngine;
using namespace Common;

const std::wstring RepairPolicyEngineServiceReplica::KeyValueStoreName = L"RepairPolicyEngineServiceKeyValueStore";

RepairPolicyEngineServiceReplica::RepairPolicyEngineServiceReplica()
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica::RepairPolicyEngineServiceReplica()");
}

RepairPolicyEngineServiceReplica::~RepairPolicyEngineServiceReplica()
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica::~RepairPolicyEngineServiceReplica()");
}

HRESULT RepairPolicyEngineServiceReplica::Initialize(
    IFabricCodePackageActivationContext *activationContext,
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    ManualResetEvent* exitServiceHostEventPtr)
{
    activationContextCPtr_.SetAndAddRef(activationContext);
    exitServiceHostEventPtr_ = exitServiceHostEventPtr;

    Common::Guid partitionGuid(partitionId);
    StringWriter partitionIdWriter(partitionIdString_);
    partitionIdWriter.Write("{0}", partitionGuid);

    StringWriter replicaIdWriter(replicaIdString_);
    replicaIdWriter.Write("{0}", replicaId);

    ReplicaRole = FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_UNKNOWN;

    Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica::Initialize(partitionId=={0}, replicaId=={1}, replicaRole=={2})", partitionIdString_, replicaIdString_, replicaRoleString_);

    if (!KeyValueStoreHelper::GetReplicatorAddress(activationContextCPtr_, replicatorAddress_))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineServiceReplica::Initialize() failed as GetReplicatorAddress failed.");
        return E_FAIL;
    }
    
    FABRIC_REPLICATOR_SETTINGS replicatorSettings = { 0 };
    replicatorSettings.ReplicatorAddress = replicatorAddress_.c_str();
    replicatorSettings.Flags = FABRIC_REPLICATOR_ADDRESS;
    replicatorSettings.Reserved = NULL;

    HRESULT resultCreateStoreReplica = FabricCreateKeyValueStoreReplica(
        IID_IFabricKeyValueStoreReplica2,
        KeyValueStoreName.c_str(),
        partitionId,
        replicaId,
        &replicatorSettings,
        FABRIC_LOCAL_STORE_KIND_ESE,
        NULL,
        this,
        (void **)keyValueStorePtr_.InitializationAddress());
    if (FAILED(resultCreateStoreReplica))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineServiceReplica::Initialize() failed as FabricCreateKeyValueStoreReplica() failed with error {0}.", resultCreateStoreReplica);
        return resultCreateStoreReplica;
    }

    HRESULT resultServiceInitialize = repairPolicyEngineService_.Initialize(keyValueStorePtr_, activationContext, exitServiceHostEventPtr_, partitionId, replicaId);

    if (FAILED(resultServiceInitialize))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineServiceReplica::Initialize() failed as RepairPolicyEngineService.Initialize() failed with error {0}.", resultServiceInitialize);
        return resultServiceInitialize;
    }

    return S_OK;
}

STDMETHODIMP RepairPolicyEngineServiceReplica::BeginOpen(
    __in FABRIC_REPLICA_OPEN_MODE openMode,
    __in IFabricStatefulServicePartition *partition,
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context) 
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica::BeginOpen() partition={0}, replica={1}", partitionIdString_, replicaIdString_);

    UNREFERENCED_PARAMETER(openMode);

    if (partition == nullptr || callback == nullptr || context == nullptr)
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineServiceReplica::BeginOpen(): Failed. One of arguments is null.");
        return E_POINTER;
    }

    return keyValueStorePtr_->BeginOpen(openMode, partition, callback, context);
}

STDMETHODIMP RepairPolicyEngineServiceReplica::EndOpen(
    __in IFabricAsyncOperationContext *context,
    __out IFabricReplicator **replicator) 
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica::EndOpen() partition={0}, replica={1}", partitionIdString_, replicaIdString_);

    if (context == nullptr || replicator == nullptr)
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineServiceReplica::EndOpen() one of arguments is NULL.");
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    return  keyValueStorePtr_->EndOpen(context,replicator);
}

STDMETHODIMP RepairPolicyEngineServiceReplica::BeginChangeRole( 
    __in FABRIC_REPLICA_ROLE newRole,
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    std::wstring replicaRoleString;
    StringWriter replicaRoleWriter(replicaRoleString);
    replicaRoleWriter << newRole;

    Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica::BeginChangeRole() partition={0}, replica={1}, newRole={2}", partitionIdString_, replicaIdString_, replicaRoleString);

    if (callback == nullptr || context == nullptr)
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineServiceReplica::BeginChangeRole() one of arguments is NULL.");
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    HRESULT resultChangeRole = keyValueStorePtr_->BeginChangeRole(newRole, callback, context);
    if (FAILED(resultChangeRole))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineServiceReplica::BeginChangeRole(): KVS BeginChangeRole() failed with error '{0}'.", ErrorCode::FromHResult(resultChangeRole));
        return ComUtility::OnPublicApiReturn(resultChangeRole); 
    }

    if (newRole == FABRIC_REPLICA_ROLE_PRIMARY)
    {
        Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica.BeginChangeRole() is Primary. Stopping RepairPolicyEngineService if running ...");
        repairPolicyEngineService_.Stop();

        Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica.BeginChangeRole() is Primary. Starting RepairPolicyEngineService in primary mode...");
        repairPolicyEngineService_.IsPrimaryReplica = true;
        ReplicaRole = newRole;
        repairPolicyEngineService_.RunAsync();
    }
    else if (newRole == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
    {
        Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica.BeginChangeRole() is Active Secondary. Stopping RepairPolicyEngineService ...");
        repairPolicyEngineService_.Stop();
        Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica.BeginChangeRole() is Active Secondary. Starting RepairPolicyEngineService in secondary mode ...");
        repairPolicyEngineService_.IsPrimaryReplica = false;
        ReplicaRole = newRole;
        repairPolicyEngineService_.RunAsync();
    }
    else
    {
        Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica.BeginChangeRole() is not primary or active secondary. Stopping RepairPolicyEngineService if running.");
        repairPolicyEngineService_.Stop();
        repairPolicyEngineService_.IsPrimaryReplica = false;
        ReplicaRole = newRole;
    }

    return ComUtility::OnPublicApiReturn(S_OK);
}

STDMETHODIMP RepairPolicyEngineServiceReplica::EndChangeRole( 
    __in IFabricAsyncOperationContext *context,
    __out IFabricStringResult **serviceAddress) 
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica::EndChangeRole() partition={0}, replica={1}", partitionIdString_, replicaIdString_);

    if (context == nullptr || serviceAddress == nullptr)
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineServiceReplica::EndChangeRole() one of arguments is NULL.");
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    return keyValueStorePtr_->EndChangeRole(context, serviceAddress);
}

STDMETHODIMP RepairPolicyEngineServiceReplica::BeginClose( 
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context) 
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica::BeginClose() partition={0}, replica={1}", partitionIdString_, replicaIdString_);

    if (callback == nullptr || context == nullptr)
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineServiceReplica::BeginClose() one of arguments is NULL.");
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    HRESULT resultClose = keyValueStorePtr_->BeginClose(callback, context);
    if (FAILED(resultClose))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineServiceReplica::BeginClose(): KVS BeginClose() failed with error '{0}'.", ErrorCode::FromHResult(resultClose));
        return ComUtility::OnPublicApiReturn(resultClose); 
    }

    Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica.BeginClose(). Stopping RepairPolicyEngineService ...");
    repairPolicyEngineService_.Stop();

    return ComUtility::OnPublicApiReturn(S_OK);
}

STDMETHODIMP RepairPolicyEngineServiceReplica::EndClose( 
    __in IFabricAsyncOperationContext *context) 
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica::EndClose() partition={0}, replica={1}", partitionIdString_, replicaIdString_);

    if (context == nullptr)
    {
        Trace.WriteError(ComponentName, "VoidFabricReplicator::EndClose() argument is NULL.");
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    return keyValueStorePtr_->EndClose(context);
}

STDMETHODIMP_(void) RepairPolicyEngineServiceReplica::Abort(void)
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica::Abort() partition={0}, replica={1}", partitionIdString_, replicaIdString_);
    keyValueStorePtr_->Abort();

    Trace.WriteInfo(ComponentName, "RepairPolicyEngineServiceReplica::Abort(). Stopping RepairPolicyEngineService ...");
    repairPolicyEngineService_.Stop();
}

STDMETHODIMP_(void) RepairPolicyEngineServiceReplica::OnDataLoss(void)
{
    Trace.WriteError(ComponentName, "RepairPolicyEngineServiceReplica::OnDataLoss() received");
}
