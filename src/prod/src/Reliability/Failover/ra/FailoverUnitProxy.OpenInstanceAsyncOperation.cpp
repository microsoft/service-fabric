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

void FailoverUnitProxy::OpenInstanceAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    FABRIC_REPLICA_ID replicaId;
    Reliability::ServiceDescription serviceDesc;
    wstring runtimeId;

    {
        AcquireExclusiveLock grab(owner_.lock_);

        TraceBeforeStart(grab);

        ASSERT_IF(owner_.IsStatefulServiceFailoverUnitProxy(), "Call to open stateless service on an FUP hosting a stateful service.");

        if(owner_.IsStatelessServiceFailoverUnitProxy())
        {
            // Checks to ensure this is a valid duplicate call

            ASSERT_IFNOT(
               owner_.failoverUnitDescription_.FailoverUnitId == owner_.failoverUnitDescription_.FailoverUnitId &&
               owner_.replicaDescription_.InstanceId == owner_.replicaDescription_.InstanceId,
               "Duplicate open stateless service call with a different service.");
            ASSERT_IF(
                owner_.replicaState_ == FailoverUnitProxyStates::Closed ||
                owner_.replicaState_ == FailoverUnitProxyStates::Closing,
                "Unexpected call to stateless service open (service present but FUP marked closed/closing).");

            // Service location is unchanged
            serviceLocation_ = owner_.replicaDescription_.ServiceLocation;

            FinishWork(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        if (!owner_.serviceOperationManager_)
        {
            owner_.serviceOperationManager_ = make_unique<ServiceOperationManager>(false/*isStateful*/, owner_);
        }

        if (!TryStartUserApi(grab, thisSPtr))
        {
            return;
        }

        replicaId = (FABRIC_REPLICA_ID) owner_.replicaDescription_.ReplicaId;
        serviceDesc = owner_.serviceDescription_;
        runtimeId = owner_.runtimeId_;
    }

    AsyncOperationSPtr operation = applicationHost_.BeginGetFactoryAndCreateInstance(
        runtimeId,
        serviceDesc.Type,
        serviceDesc.CreateActivationContext(owner_.FailoverUnitId.Guid),
        ServiceModel::SystemServiceApplicationNameHelper::GetPublicServiceName(serviceDesc.Name),
        serviceDesc.InitializationData,
        owner_.FailoverUnitDescription.FailoverUnitId.Guid,
        replicaId,
        [this](AsyncOperationSPtr const & operation) { OnGetFactoryAndCreateInstanceCompleted(operation, false); },
        thisSPtr);

    OnGetFactoryAndCreateInstanceCompleted(operation, true);
}

void FailoverUnitProxy::OpenInstanceAsyncOperation::OnGetFactoryAndCreateInstanceCompleted(AsyncOperationSPtr const & createAsyncOperation, bool expectedCompletedSynchronously)
{
    if (createAsyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        return;

    ErrorCode retValue;
    ComPointer<IFabricStatelessServiceInstance> comStatelessService;

    retValue = applicationHost_.EndGetFactoryAndCreateInstance(createAsyncOperation, comStatelessService);

    AsyncOperationSPtr const & thisSPtr = createAsyncOperation->Parent;

    if (!retValue.IsSuccess())
    {
        FinishWork(thisSPtr, retValue);
        return;
    }

    statelessService_ = make_unique<ComProxyStatelessService>(comStatelessService);
    {
        AcquireExclusiveLock grab(owner_.lock_);
        statelessServicePartition_ = owner_.statelessServicePartition_;
        ASSERT_IF(statelessServicePartition_.GetRawPointer() == nullptr, "Partition must have been created {0}", owner_);

        owner_.replicaState_ = FailoverUnitProxyStates::Opening;
        owner_.messageStage_ = ProxyMessageStage::None;
    }

    ComPointer<IFabricStatelessServicePartition> partition(statelessServicePartition_.GetRawPointer(), IID(IID_IFabricStatelessServicePartition));

    AsyncOperationSPtr operation = statelessService_->BeginOpen(
        partition,
        [this](AsyncOperationSPtr const & operation) { this->OpenInstanceCompletedCallback(operation); },
        thisSPtr);

    TryContinueUserApi(operation);

    if (operation->CompletedSynchronously)
    {
        FinishOpenInstance(operation);
    }
}

ProxyErrorCode FailoverUnitProxy::OpenInstanceAsyncOperation::End(Common::AsyncOperationSPtr const & asyncOperation, __out std::wstring & serviceLocation)
{
    auto thisPtr = AsyncOperation::End<FailoverUnitProxy::OpenInstanceAsyncOperation>(asyncOperation);
    serviceLocation.swap(thisPtr->serviceLocation_);
    return thisPtr->EndHelper();
}

void FailoverUnitProxy::OpenInstanceAsyncOperation::OpenInstanceCompletedCallback(AsyncOperationSPtr const & openAsyncOperation)
{
    if (!openAsyncOperation->CompletedSynchronously)
    {
        FinishOpenInstance(openAsyncOperation);
    }
}

void FailoverUnitProxy::OpenInstanceAsyncOperation::FinishOpenInstance(AsyncOperationSPtr const & openAsyncOperation)
{
    ErrorCode errorCode;

    errorCode = statelessService_->EndOpen(openAsyncOperation, serviceLocation_);
        
    AsyncOperationSPtr const & thisSPtr = openAsyncOperation->Parent;

    {
        AcquireExclusiveLock grab (owner_.lock_);

        owner_.reportedLoadStore_.OnFTOpen();

        if (errorCode.IsSuccess())
        {
            owner_.statelessServicePartition_ = std::move(statelessServicePartition_);
            owner_.statelessService_ = std::move(statelessService_);
            owner_.replicaDescription_.ServiceLocation = serviceLocation_;
            
            RAPEventSource::Events->FailoverUnitProxyEndpointUpdate(
                owner_.FailoverUnitId.Guid,
                owner_.RAPId,
                interface_,
                serviceLocation_);

            owner_.replicaState_ = FailoverUnitProxyStates::Opened;
        }
        else
        {
            owner_.replicaState_ = FailoverUnitProxyStates::Closed;
        }

        FinishUserApi(grab, errorCode);
    }

    bool didComplete = thisSPtr->TryComplete(thisSPtr, errorCode);
    ASSERT_IFNOT(didComplete, "Failed to complete OpenInstanceAsyncOperation");
}

void FailoverUnitProxy::OpenInstanceAsyncOperation::FinishWork(AsyncOperationSPtr const & thisSPtr, ErrorCode const & result)
{
    {
        AcquireExclusiveLock grab(owner_.lock_);
        FinishUserApi(grab, result);
    }

    bool didComplete = thisSPtr->TryComplete(thisSPtr, result);
    ASSERT_IFNOT(didComplete, "Failed to complete OpenInstanceAsyncOperation");
}
