// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;

void ComProxyReplicator::Abort()
{
    replicator_->Abort();
}

AsyncOperationSPtr ComProxyReplicator::BeginOpen(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, callback, parent);
}

ErrorCode ComProxyReplicator::EndOpen(AsyncOperationSPtr const & asyncOperation,  __out wstring & replicationEndpoint)
{
    ASSERT_IFNOT(asyncOperation, "asyncOperation cannot be null.");
    return OpenAsyncOperation::End(asyncOperation, replicationEndpoint);
}
        
AsyncOperationSPtr ComProxyReplicator::BeginChangeRole(
    ::FABRIC_EPOCH const & epoch,
    ::FABRIC_REPLICA_ROLE newRole,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ChangeRoleAsyncOperation>(epoch, newRole, *this, callback, parent);
}

ErrorCode ComProxyReplicator::EndChangeRole(AsyncOperationSPtr const & asyncOperation)
{
    ASSERT_IFNOT(asyncOperation, "asyncOperation cannot be null.");
    return ChangeRoleAsyncOperation::End(asyncOperation);
}
               
AsyncOperationSPtr ComProxyReplicator::BeginUpdateEpoch( 
    ::FABRIC_EPOCH const & epoch,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UpdateEpochAsyncOperation>(*this, epoch, callback, parent);
}

ErrorCode ComProxyReplicator::EndUpdateEpoch(AsyncOperationSPtr const & asyncOperation)
{
    ASSERT_IFNOT(asyncOperation, "asyncOperation cannot be null.");
    return UpdateEpochAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr ComProxyReplicator::BeginClose(
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, callback, parent);
}

ErrorCode ComProxyReplicator::EndClose(AsyncOperationSPtr const & asyncOperation)
{
    ASSERT_IFNOT(asyncOperation, "asyncOperation cannot be null.");
    return CloseAsyncOperation::End(asyncOperation);
}

ErrorCode ComProxyReplicator::GetStatus(__out FABRIC_SEQUENCE_NUMBER & firstLsn, __out FABRIC_SEQUENCE_NUMBER & lastLsn)
{
    HRESULT hr = replicator_->GetCurrentProgress(&lastLsn);

    if(FAILED(hr))
    {
        return ErrorCode::FromHResult(hr);
    }

    hr = replicator_->GetCatchUpCapability(&firstLsn);
    
    return ErrorCode::FromHResult(hr);
}

// IFabricPrimaryReplicator

ErrorCode ComProxyReplicator::UpdateCatchUpReplicaSetConfiguration(
    FABRIC_REPLICA_SET_CONFIGURATION const * currentConfiguration,
    FABRIC_REPLICA_SET_CONFIGURATION const * previousConfiguration)
{
    HRESULT hr = primary_->UpdateCatchUpReplicaSetConfiguration(currentConfiguration, previousConfiguration);

    return ErrorCode::FromHResult(hr);
}

AsyncOperationSPtr ComProxyReplicator::BeginWaitForCatchUpQuorum(
    ::FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CatchupReplicaSetAsyncOperation>(
        catchUpMode, *this, callback, parent);
}

ErrorCode ComProxyReplicator::EndWaitForCatchUpQuorum(AsyncOperationSPtr const & asyncOperation)
{
    ASSERT_IFNOT(asyncOperation, "asyncOperation cannot be null.");
    return CatchupReplicaSetAsyncOperation::End(asyncOperation);
}

ErrorCode ComProxyReplicator::UpdateCurrentReplicaSetConfiguration(
    FABRIC_REPLICA_SET_CONFIGURATION const * currentConfiguration)
{
    HRESULT hr = primary_->UpdateCurrentReplicaSetConfiguration(currentConfiguration);

    return ErrorCode::FromHResult(hr);
}

AsyncOperationSPtr ComProxyReplicator::BeginBuildIdleReplica( 
    ReplicaDescription const & idleReplicaDescription,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<BuildIdleReplicaAsyncOperation>(
        idleReplicaDescription, *this, callback, parent);
}

ErrorCode ComProxyReplicator::EndBuildIdleReplica(AsyncOperationSPtr const & asyncOperation)
{
    ASSERT_IFNOT(asyncOperation, "asyncOperation cannot be null.");
    return BuildIdleReplicaAsyncOperation::End(asyncOperation);
}
        
ErrorCode ComProxyReplicator::RemoveIdleReplica(FABRIC_REPLICA_ID replicaId)
{
    HRESULT hr = primary_->RemoveReplica(replicaId);
    
    return ErrorCode::FromHResult(hr);
}


AsyncOperationSPtr ComProxyReplicator::BeginOnDataLoss( 
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OnDataLossAsyncOperation>(*this, callback, parent);
}

ErrorCode ComProxyReplicator::EndOnDataLoss(AsyncOperationSPtr const & asyncOperation, __out bool & isStateChanged)
{
    ASSERT_IFNOT(asyncOperation, "asyncOperation cannot be null.");
    return OnDataLossAsyncOperation::End(asyncOperation, isStateChanged);
}

// IFabricInternalReplicator

ErrorCode ComProxyReplicator::GetReplicatorQueryResult(__out ServiceModel::ReplicatorStatusQueryResultSPtr & result)
{
    HRESULT hr = E_NOTIMPL;
    
    if (internalReplicator_.GetRawPointer() != NULL)
    {
        ComPointer<IFabricGetReplicatorStatusResult> replicatorStatusResult;
        hr = internalReplicator_->GetReplicatorStatus(replicatorStatusResult.InitializationAddress());

        if (hr == S_OK)
        {
            FABRIC_REPLICA_ROLE role = replicatorStatusResult->get_ReplicatorStatus()->Role;

            TESTASSERT_IF(
                role != FABRIC_REPLICA_ROLE_PRIMARY && role != FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY && role != FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
                "Invalid Role in replicator result");
            
            result = ServiceModel::ReplicatorStatusQueryResult::CreateSPtr(role);
            result->FromPublicApi(*replicatorStatusResult->get_ReplicatorStatus());
        }
    }

    return ErrorCode::FromHResult(hr);
}
