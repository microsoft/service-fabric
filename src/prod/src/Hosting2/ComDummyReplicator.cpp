//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("ComDummyReplicator");

ComDummyReplicator::ComDummyReplicator(
    wstring const & serviceName,
    Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId)
    : IFabricPrimaryReplicator()
    , ComUnknownBase()
    , traceId_(wformatString("{0}:{1}:{2}", serviceName, partitionId, replicaId))
{
    currentEpoch_ = {0};
    WriteNoise(TraceType, traceId_, "ctor");
}

ComDummyReplicator::~ComDummyReplicator()
{
    WriteNoise(TraceType, traceId_, "dtor");
}

FABRIC_EPOCH ComDummyReplicator::GetCurrentEpoch()
{
    return currentEpoch_;
}

HRESULT ComDummyReplicator::BeginOpen(
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    WriteNoise(TraceType, traceId_, "BeginOpen");

    return ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
}

HRESULT ComDummyReplicator::EndOpen(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **replicationAddress)
{
    WriteNoise(TraceType, traceId_, "EndOpen");

    auto hr = ComCompletedOperationHelper::EndCompletedComOperation(context);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr); 
    }

    return ComStringResult::ReturnStringResult(wstring(), replicationAddress);
}

HRESULT ComDummyReplicator::BeginChangeRole(
    /* [in] */ const FABRIC_EPOCH *epoch,
    /* [in] */ FABRIC_REPLICA_ROLE role,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(role);

    WriteNoise(TraceType, traceId_, "BeginChangeRole");

    currentEpoch_ = *epoch;
    return ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
}

HRESULT ComDummyReplicator::EndChangeRole(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    WriteNoise(TraceType, traceId_, "EndChangeRole");

    auto hr = ComCompletedOperationHelper::EndCompletedComOperation(context);
    return ComUtility::OnPublicApiReturn(hr);
}

HRESULT ComDummyReplicator::BeginUpdateEpoch(
    /* [in] */ const FABRIC_EPOCH *epoch,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    WriteNoise(TraceType, traceId_, "BeginUpdateEpoch");

    currentEpoch_ = *epoch;
    return ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
}

HRESULT ComDummyReplicator::EndUpdateEpoch(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    WriteNoise(TraceType, traceId_, "EndUpdateEpoch");

    auto hr = ComCompletedOperationHelper::EndCompletedComOperation(context);
    return ComUtility::OnPublicApiReturn(hr);
}

HRESULT ComDummyReplicator::BeginClose(
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    WriteNoise(TraceType, traceId_, "BeginClose");

    return ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
}

HRESULT ComDummyReplicator::EndClose(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    WriteNoise(TraceType, traceId_, "EndClose");

    auto hr = ComCompletedOperationHelper::EndCompletedComOperation(context);
    return ComUtility::OnPublicApiReturn(hr);
}

void ComDummyReplicator::Abort(void)
{
    WriteNoise(TraceType, traceId_, "Aborted");
}

HRESULT ComDummyReplicator::GetCurrentProgress(
    /* [out] */ FABRIC_SEQUENCE_NUMBER *lastSequenceNumber)
{
    WriteNoise(TraceType, traceId_, "GetCurrentProgress");

    *lastSequenceNumber = 0;

    return S_OK;
}

HRESULT ComDummyReplicator::GetCatchUpCapability(
    /* [out] */ FABRIC_SEQUENCE_NUMBER *fromSequenceNumber)
{
    WriteNoise(TraceType, traceId_, "GetCatchUpCapability");

    *fromSequenceNumber = 0;

    return S_OK;
}

HRESULT ComDummyReplicator::BeginOnDataLoss(
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    WriteNoise(TraceType, traceId_, "BeginOnDataLoss");

    return ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
}

HRESULT ComDummyReplicator::EndOnDataLoss(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ BOOLEAN *isStateChanged)
{
    WriteNoise(TraceType, traceId_, "EndOnDataLoss");

    auto hr = ComCompletedOperationHelper::EndCompletedComOperation(context);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    *isStateChanged = false;

    return S_OK;
}

HRESULT ComDummyReplicator::UpdateCatchUpReplicaSetConfiguration(
    /* [in] */ const FABRIC_REPLICA_SET_CONFIGURATION *currentConfiguration,
    /* [in] */ const FABRIC_REPLICA_SET_CONFIGURATION *previousConfiguration)
{
    UNREFERENCED_PARAMETER(currentConfiguration);
    UNREFERENCED_PARAMETER(previousConfiguration);
    
    WriteNoise(TraceType, traceId_, "UpdateCatchUpReplicaSetConfiguration");

    return S_OK;
}

HRESULT ComDummyReplicator::BeginWaitForCatchUpQuorum(
    /* [in] */ FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(catchUpMode);

    WriteNoise(TraceType, traceId_, "BeginWaitForCatchUpQuorum");

    return ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
}

HRESULT ComDummyReplicator::EndWaitForCatchUpQuorum(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    WriteNoise(TraceType, traceId_, "EndWaitForCatchUpQuorum");

    auto hr = ComCompletedOperationHelper::EndCompletedComOperation(context);
    return ComUtility::OnPublicApiReturn(hr);
}

HRESULT ComDummyReplicator::UpdateCurrentReplicaSetConfiguration(
    /* [in] */ const FABRIC_REPLICA_SET_CONFIGURATION *currentConfiguration)
{
    UNREFERENCED_PARAMETER(currentConfiguration);
    
    WriteNoise(TraceType, traceId_, "UpdateCurrentReplicaSetConfiguration");

    return S_OK;
}

HRESULT ComDummyReplicator::BeginBuildReplica(
    /* [in] */ const FABRIC_REPLICA_INFORMATION *replica,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(replica);

    WriteNoise(TraceType, traceId_, "BeginBuildReplica");

    return ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
}

HRESULT ComDummyReplicator::EndBuildReplica(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    WriteNoise(TraceType, traceId_, "EndBuildReplica");

    auto hr = ComCompletedOperationHelper::EndCompletedComOperation(context);
    return ComUtility::OnPublicApiReturn(hr);
}

HRESULT ComDummyReplicator::RemoveReplica(
    /* [in] */ FABRIC_REPLICA_ID replicaId)
{
    WriteNoise(TraceType, traceId_, "RemoveReplica: {0}", replicaId);

    return S_OK;
}
