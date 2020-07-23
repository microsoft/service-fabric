// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace V1ReplPerf;

ComPointer<IFabricReplicatorSettingsResult> LoadReplicatorSettings();

ComService::ComService(Service & service)
    : root_(service.shared_from_this()),
    service_(service),
    serviceEndpoint_(L""),
    replicationEngine_()
{
}

HRESULT ComService::BeginOpen( 
    /* [in] */ FABRIC_REPLICA_OPEN_MODE,
    /* [in] */ IFabricStatefulServicePartition *statefulServicePartition,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (statefulServicePartition == NULL || callback == NULL || context == NULL) { return E_POINTER; }

    auto settings = LoadReplicatorSettings();

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = statefulServicePartition->CreateReplicator(
        this, 
        settings->get_ReplicatorSettings(),
        replicationEngine_.InitializationAddress(), 
        stateReplicator_.InitializationAddress());

    ASSERT_IFNOT(hr == S_OK, "Failed to create the replicator");
    operation->Initialize(root_, callback);

    service_.OnOpen(statefulServicePartition, stateReplicator_);

    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(move(operation), context);
}

HRESULT ComService::EndOpen(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [out][retval] */ IFabricReplicator **replicationEngine)
{
    if (context == NULL || replicationEngine == NULL) { return E_POINTER; }

    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    if (FAILED(hr)) { return hr; }
    *replicationEngine = replicationEngine_.DetachNoRelease();
    return hr;
}

HRESULT ComService::BeginChangeRole( 
    /* [in] */ FABRIC_REPLICA_ROLE newRole,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (callback == NULL || context == NULL) { return E_POINTER; }

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = E_FAIL;
    hr = operation->Initialize(root_, callback);

    serviceEndpoint_ = service_.OnChangeRole(newRole);
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(move(operation), context);
}

HRESULT ComService::EndChangeRole( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceEndpoint)
{
    if (context == NULL || serviceEndpoint == NULL) { return E_POINTER; }

    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    if (FAILED(hr)) return hr;

    ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult,IFabricStringResult>(serviceEndpoint_);
    *serviceEndpoint = stringResult.DetachNoRelease();
    return S_OK;
}

HRESULT ComService::BeginClose( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL) { return E_POINTER; }

    service_.OnClose();

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    operation->Initialize(root_, callback);
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT ComService::EndClose( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    if (context == NULL) { return E_POINTER; }

    return ComCompletedAsyncOperationContext::End(context);
}

void STDMETHODCALLTYPE ComService::Abort()
{
    service_.OnAbort();
}

HRESULT STDMETHODCALLTYPE ComService::BeginUpdateEpoch( 
    /* [in] */ FABRIC_EPOCH const * epoch,
    /* [in] */ FABRIC_SEQUENCE_NUMBER,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (epoch == NULL || callback == NULL || context == NULL) { return E_POINTER; }
    
    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    operation->Initialize(root_, callback);
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT ComService::EndUpdateEpoch( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    if (context == NULL) { return E_POINTER; }
    return ComCompletedAsyncOperationContext::End(context);
}

HRESULT ComService::GetLastCommittedSequenceNumber( 
    /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber)
{
    if (sequenceNumber == NULL) { return E_POINTER; }
    *sequenceNumber = 1;
    return S_OK;
}

//The test does not recover from data loss
HRESULT ComService::BeginOnDataLoss( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL) { return E_POINTER; }

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    operation->Initialize(root_, callback);
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT ComService::EndOnDataLoss( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][string][out] */ BOOLEAN * isStateChanged)
{
    if (context == NULL || isStateChanged == NULL) { return E_POINTER; }
    *isStateChanged = FALSE;
    return ComCompletedAsyncOperationContext::End(context);
}

HRESULT ComService::GetCopyContext(
    /*[out, retval]*/ IFabricOperationDataStream ** operationDataAsyncEnumerator)
{
    if (operationDataAsyncEnumerator == NULL) { return E_POINTER; }
    return service_.GetCopyContext(operationDataAsyncEnumerator);
}

HRESULT ComService::GetCopyState(
    /*[in]*/ FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
    /*[in]*/ IFabricOperationDataStream * operationDataAsyncEnumerator,
    /*[out, retval]*/ IFabricOperationDataStream ** copyStateEnumerator)
{
    if (copyStateEnumerator == NULL) { return E_POINTER; }
    return service_.GetCopyState(uptoSequenceNumber, operationDataAsyncEnumerator, copyStateEnumerator);
}

ComPointer<IFabricReplicatorSettingsResult> LoadReplicatorSettings()
{
    ComPointer<IFabricReplicatorSettingsResult> replicatorSettings;
    ComPointer<IFabricCodePackageActivationContext> activationContext;

    ASSERT_IFNOT(
        ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContext.VoidInitializationAddress()) == S_OK,
        "Failed to get activation context in LoadReplicatorSettings");

    ComPointer<IFabricStringResult> configPackageName = make_com<ComStringResult, IFabricStringResult>(Constants::ReplicatorSettingsConfigPackageName);
    ComPointer<IFabricStringResult> sectionName = make_com<ComStringResult, IFabricStringResult>(Constants::ReplicatorSettingsSectionName);

    auto hr = ::FabricLoadReplicatorSettings(
        activationContext.GetRawPointer(),
        configPackageName->get_String(),
        sectionName->get_String(),
        replicatorSettings.InitializationAddress());

    ASSERT_IFNOT(hr == S_OK, "Failed to load replicator settings due to error {0}", hr);

    return replicatorSettings;
}
