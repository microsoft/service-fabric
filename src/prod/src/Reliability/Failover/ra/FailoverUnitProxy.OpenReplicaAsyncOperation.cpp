// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

void FailoverUnitProxy::OpenReplicaAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    FABRIC_REPLICA_ID replicaId;
    Reliability::ServiceDescription serviceDesc;
    wstring runtimeId;

    {
        AcquireExclusiveLock grab(owner_.lock_);

        TraceBeforeStart(grab);

        ASSERT_IF(owner_.IsStatelessServiceFailoverUnitProxy(), "Call to open stateful service on an FUP hosting a stateless service.");

        if(owner_.IsStatefulServiceFailoverUnitProxy())
        {
            // Checks to ensure this is a valid duplicate call

            ASSERT_IFNOT(
                owner_.replicaState_ != FailoverUnitProxyStates::Closed && 
                owner_.replicaState_ != FailoverUnitProxyStates::Closing,
                "Unexpected call to stateful service close (service present but FUP marked closed/closing).");

            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            ASSERT_IFNOT(didComplete, "Failed to complete OpenReplicaAsyncOperation");
            return;
        }

        if (!owner_.serviceOperationManager_)
        {
            owner_.serviceOperationManager_ = make_unique<ServiceOperationManager>(true/*isStateful*/, owner_);
        }

        ASSERT_IF(owner_.serviceOperationManager_ == nullptr, "Service operation manager expected but not available.");

        if (!TryStartUserApi(grab, thisSPtr))
        {
            return;
        }
        
        TESTASSERT_IF(owner_.currentServiceRole_ != ReplicaRole::Unknown, "Current service role mismatch {0}", owner_);

        replicaId = (FABRIC_REPLICA_ID) owner_.replicaDescription_.ReplicaId;
        serviceDesc = owner_.serviceDescription_;
        runtimeId = owner_.runtimeId_;
    }

    AsyncOperationSPtr operation = applicationHost_.BeginGetFactoryAndCreateReplica(
        runtimeId,
        serviceDesc.Type,
        serviceDesc.CreateActivationContext(owner_.FailoverUnitId.Guid),
        ServiceModel::SystemServiceApplicationNameHelper::GetPublicServiceName(serviceDesc.Name),
        serviceDesc.InitializationData,
        owner_.FailoverUnitId.Guid,
        replicaId,
        [this](AsyncOperationSPtr const & operation) { OnGetFactoryAndCreateReplicaCompleted(operation, false); },
        thisSPtr);

    OnGetFactoryAndCreateReplicaCompleted(operation, true);
}

void FailoverUnitProxy::OpenReplicaAsyncOperation::OnGetFactoryAndCreateReplicaCompleted(Common::AsyncOperationSPtr const & createAsyncOperation, bool expectedCompletedSynchronously)
{
    if (createAsyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        return;

    ErrorCode retValue;
    Common::ComPointer<IFabricStatefulServiceReplica> comService;

    retValue = applicationHost_.EndGetFactoryAndCreateReplica(createAsyncOperation, comService);

    AsyncOperationSPtr const & thisSPtr = createAsyncOperation->Parent;

    if (!retValue.IsSuccess())
    {
        FinishWork(thisSPtr, retValue);
        return;
    }

    statefulService_ = make_unique<ComProxyStatefulService>(comService);

    ReplicaOpenMode::Enum openMode;

    {
        AcquireExclusiveLock grab(owner_.lock_);

        statefulServicePartition_ = owner_.statefulServicePartition_;
        ASSERT_IF(statefulServicePartition_.GetRawPointer() == nullptr, "Must already have been created {0}", owner_);

        owner_.replicaState_ = FailoverUnitProxyStates::Opening;
        owner_.messageStage_ = ProxyMessageStage::None;
        openMode = owner_.replicaOpenMode_;
    }

    ComPointer<IFabricStatefulServicePartition> partition;
    HRESULT hr = statefulServicePartition_->QueryInterface(IID_IFabricStatefulServicePartition, partition.VoidInitializationAddress());

    ASSERT_IFNOT(SUCCEEDED(hr), "Failed to QI for IFabricStatefulservicePartition on the comservicepartition object");

    AsyncOperationSPtr operation = statefulService_->BeginOpen(
        ReplicaOpenMode::ToPublicApi(openMode),
        partition,
        [this](AsyncOperationSPtr const & operation) { this->OpenReplicaCompletedCallback(operation); },
        thisSPtr);

    TryContinueUserApi(operation);

    if (operation->CompletedSynchronously)
    {
        FinishOpenReplica(operation);
    }
}

ProxyErrorCode FailoverUnitProxy::OpenReplicaAsyncOperation::End(Common::AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<FailoverUnitProxy::OpenReplicaAsyncOperation>(asyncOperation);
    return thisPtr->EndHelper();
}

void FailoverUnitProxy::OpenReplicaAsyncOperation::OpenReplicaCompletedCallback(AsyncOperationSPtr const & openAsyncOperation)
{
    if (!openAsyncOperation->CompletedSynchronously)
    {
        FinishOpenReplica(openAsyncOperation);
    }
}

void FailoverUnitProxy::OpenReplicaAsyncOperation::FinishOpenReplica(AsyncOperationSPtr const & openAsyncOperation)
{
    ErrorCode errorCode;
    ComProxyReplicatorUPtr replicator;

    errorCode = statefulService_->EndOpen(openAsyncOperation, replicator);
        
    AsyncOperationSPtr const & thisSPtr = openAsyncOperation->Parent;

    {
        AcquireExclusiveLock grab (owner_.lock_);

        owner_.reportedLoadStore_.OnFTOpen();

        if (errorCode.IsSuccess())
        {
            owner_.currentServiceRole_ = ReplicaRole::Unknown;

            statefulServicePartition_->OnServiceOpened();

            owner_.statefulServicePartition_ = std::move(statefulServicePartition_);
            owner_.statefulService_ = std::move(statefulService_);
            owner_.replicator_ = std::move(replicator);
    
            if (owner_.ServiceDescription.HasPersistedState)
            {
                owner_.replicaOpenMode_ = ReplicaOpenMode::Existing;
            }

            owner_.replicaState_ = FailoverUnitProxyStates::Opened;
        }
        else
        {
            owner_.replicaState_ = FailoverUnitProxyStates::Closed;
        }
    
        FinishUserApi(grab, errorCode);
    }

    bool didComplete = thisSPtr->TryComplete(thisSPtr, errorCode);
    ASSERT_IFNOT(didComplete, "Failed to complete OpenReplicaAsyncOperation");
}

void FailoverUnitProxy::OpenReplicaAsyncOperation::FinishWork(AsyncOperationSPtr const & thisSPtr, ErrorCode const & result)
{
    {
        AcquireExclusiveLock grab(owner_.lock_);
        FinishUserApi(grab, result);
    }
    
    bool didComplete = thisSPtr->TryComplete(thisSPtr, result);

    ASSERT_IFNOT(didComplete, "Failed to complete OpenReplicaAsyncOperation");
}
