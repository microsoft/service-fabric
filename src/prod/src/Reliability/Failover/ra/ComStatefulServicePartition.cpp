// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ReplicationComponent;
using namespace ServiceModel;

namespace
{
    StringLiteral const ReadStatusTag("read");
    StringLiteral const WriteStatusTag("write");
}

ComStatefulServicePartition::ComStatefulServicePartition(
    Common::Guid const & partitionId, 
    FABRIC_REPLICA_ID replicaId, 
    ConsistencyUnitDescription const & consistencyUnitDescription, 
    bool hasPersistedState,
    std::weak_ptr<FailoverUnitProxy> && failoverUnitProxyWPtr)
    : ComServicePartitionBase(std::move(failoverUnitProxyWPtr)),
    replicaId_(replicaId),
    isServiceOpened_(false),
    createReplicatorAlreadyCalled_(false),
    partitionInfo_(partitionId, consistencyUnitDescription),
    hasPersistedState_(hasPersistedState),
    partitionId_(partitionId)
{
}

void ComStatefulServicePartition::ClosePartition()
{
    AcquireWriteLock grab(lock_);

    if(!isValidPartition_)
    {
        Assert::CodingError("ComStatefulServicePartition duplicate close call.");
    }

    isValidPartition_ = false;
}

void ComStatefulServicePartition::AssertIsValid(Common::Guid const & partitionId, FABRIC_REPLICA_ID replicaId, FailoverUnitProxy const & owner) const
{
    AcquireReadLock grab(lock_);
    ASSERT_IF(partitionId_ != partitionId, "PartitionId is invalid {0}", owner);
    ASSERT_IF(replicaId_ != replicaId, "ReplicaId is invalid {0}", owner);
    ASSERT_IF(!isValidPartition_, "Partition is closed {0}", owner);
}

HRESULT ComStatefulServicePartition::GetPartitionInfo(FABRIC_SERVICE_PARTITION_INFORMATION const **bufferedValue)
{
    if (!bufferedValue)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto error = CheckIfOpen();
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error);
    }

    *bufferedValue = partitionInfo_.Value;

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT ComStatefulServicePartition::GetStatus(
    AccessStatusType::Enum type,
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS * valueOut)
{
    if (!valueOut)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto error = ErrorCode::Success();
    AccessStatus::Enum accessStatus = AccessStatus::NotPrimary;

    {
        AcquireReadLock grab(lock_);
        error = CheckIfOpen(grab);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(error);
        }

        // Read the value from the partition under the lock
        error = readWriteStatus_.TryGet(type, accessStatus);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(error);
        }
    }
    
    if (accessStatus != AccessStatus::NotPrimary)
    {
        FailoverUnitProxySPtr failoverUnitProxy;
        error = LockFailoverUnitProxy(failoverUnitProxy);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(error);
        }

        if (failoverUnitProxy->ApplicationHostObj.IsLeaseExpired())
        {
            /*
                This returns whether the lease has expired and not whether the lease has failed

                If the lease has expired then the node goes into arbitration. At this time the read/write status must be TRY_AGAIN (unless already not primary).

                If the node wins arbitration then the lease is no longer expired and the value will be whatever failover determines

                If the node loses arbitration it will go down. In the future we can add an optimization where during the time the node is going down we return NOT_PRIMARY
            */
            auto original = accessStatus;
            accessStatus = AccessStatus::TryAgain;

            RAPEventSource::Events->SFPartitionLeaseExpiredStatus(type == AccessStatusType::Read ? ReadStatusTag : WriteStatusTag, accessStatus, original);
        }
    }

    *valueOut = AccessStatus::ConvertToPublicAccessStatus(accessStatus);

    return ComUtility::OnPublicApiReturn(error);
}

HRESULT ComStatefulServicePartition::GetReadStatus(::FABRIC_SERVICE_PARTITION_ACCESS_STATUS *readStatus)
{
    auto hr = GetStatus(AccessStatusType::Read, readStatus);

    ASSERT_IF(SUCCEEDED(hr) && *readStatus == FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID, "Cannot return invalid read status");
    return hr;
}

HRESULT ComStatefulServicePartition::GetWriteStatus(::FABRIC_SERVICE_PARTITION_ACCESS_STATUS *writeStatus)
{
    auto hr = GetStatus(AccessStatusType::Write, writeStatus);

    ASSERT_IF(SUCCEEDED(hr) && *writeStatus == FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID, "Cannot return invalid write status");
    return hr;
}

ErrorCode ComStatefulServicePartition::Test_IsLeaseExpired(bool & isLeaseExpired)
{
    FailoverUnitProxySPtr failoverUnitProxy;
    auto error = LockFailoverUnitProxy(failoverUnitProxy);
    if (!error.IsSuccess())
    {
        return error;
    }

    isLeaseExpired = failoverUnitProxy->ApplicationHostObj.IsLeaseExpired();
    return error;
}

HRESULT ComStatefulServicePartition::CreateReplicator(
    __in ::IFabricStateProvider *stateProvider,
    __in_opt ::FABRIC_REPLICATOR_SETTINGS const *replicatorSettings,
    __out ::IFabricReplicator **replicator,
    __out ::IFabricStateReplicator **stateReplicator)
{  
    FailoverUnitProxySPtr failoverUnitProxySPtr;
    HRESULT hr = PrepareToCreateReplicator(failoverUnitProxySPtr);

    if (!SUCCEEDED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    ASSERT_IFNOT(failoverUnitProxySPtr, "FailoverUnit proxy should be valid");
    FailoverUnitProxySPtr forReplicator = failoverUnitProxySPtr;
    Common::ComPointer<::IFabricStateReplicator> localReplicator;
    
    hr = failoverUnitProxySPtr->ReplicatorFactory.CreateReplicator(
        replicaId_, 
        this, 
        stateProvider, 
        replicatorSettings,
        hasPersistedState_,
        move(forReplicator),
        localReplicator.InitializationAddress());

    if(FAILED(hr))
    {
        RAPEventSource::Events->SFPartitionFailedToCreateReplicator(
            static_cast<_int64>(replicaId_),
            hr);
        return ComUtility::OnPublicApiReturn(hr);
    }

    hr = localReplicator->QueryInterface(::IID_IFabricReplicator, (LPVOID*) replicator);
    ASSERT_IFNOT(SUCCEEDED(hr), "Built-in replicator failed a QI for IFabricReplicator.");
    
    hr = localReplicator->QueryInterface(::IID_IFabricStateReplicator, (LPVOID*) stateReplicator);;
    ASSERT_IFNOT(SUCCEEDED(hr), "Built-in replicator failed a QI for IFabricStateReplicator.");

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT ComStatefulServicePartition::ReportLoad(ULONG metricCount, FABRIC_LOAD_METRIC const *metrics)
{
    return ComServicePartitionBase::ReportLoad(metricCount, metrics);
}

HRESULT ComStatefulServicePartition::ReportFault(FABRIC_FAULT_TYPE faultType)
{
    if (
        faultType != FABRIC_FAULT_TYPE_PERMANENT &&
        faultType != FABRIC_FAULT_TYPE_TRANSIENT)
    {
        return ComUtility::OnPublicApiReturn(ErrorCode(ErrorCodeValue::InvalidArgument));
    }

    FailoverUnitProxySPtr failoverUnitProxySPtr;
    auto error = CheckIfOpenAndLockFailoverUnitProxy(failoverUnitProxySPtr);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error);
    }

    error = failoverUnitProxySPtr->ReportFault(FaultType::FromPublicAPI(faultType));
    return ComUtility::OnPublicApiReturn(error);
}

HRESULT ComStatefulServicePartition::ReportReplicaHealth(FABRIC_HEALTH_INFORMATION const *healthInformation)
{
    return ReportReplicaHealth2(healthInformation, nullptr);
}

HRESULT ComStatefulServicePartition::ReportReplicaHealth2(
    FABRIC_HEALTH_INFORMATION const *healthInformation,
    FABRIC_HEALTH_REPORT_SEND_OPTIONS const *sendOptions)
{
    if (!healthInformation)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    HealthInformation healthInfoObj;
    auto error = healthInfoObj.FromCommonPublicApi(*healthInformation);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(move(error));
    }

    HealthReportSendOptionsUPtr sendOptionsObj;
    if (sendOptions != nullptr)
    {
        sendOptionsObj = make_unique<HealthReportSendOptions>();
        error = sendOptionsObj->FromPublicApi(*sendOptions);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(error));
        }
    }

    {
        AcquireReadLock grab(lock_);
        if (!isValidPartition_)
        {
            return ComUtility::OnPublicApiReturn(ErrorCode(ErrorCodeValue::ObjectClosed));
        }
    }

    auto healthReport = ServiceModel::HealthReport::GenerateReplicaHealthReport(move(healthInfoObj), partitionId_, replicaId_);
    
    FailoverUnitProxySPtr failoverUnitProxySPtr = failoverUnitProxyWPtr_.lock();
    if (failoverUnitProxySPtr)
    {
        error = failoverUnitProxySPtr->ReportHealth(move(healthReport), move(sendOptionsObj));
        return ComUtility::OnPublicApiReturn(move(error));
    }

    return ComUtility::OnPublicApiReturn(FABRIC_E_COMMUNICATION_ERROR);
}

HRESULT ComStatefulServicePartition::ReportPartitionHealth(FABRIC_HEALTH_INFORMATION const *healthInformation)
{
    return ReportPartitionHealth2(healthInformation, nullptr);
}

HRESULT ComStatefulServicePartition::ReportPartitionHealth2(
    FABRIC_HEALTH_INFORMATION const *healthInformation,
    FABRIC_HEALTH_REPORT_SEND_OPTIONS const *sendOptions)
{
    if (!healthInformation)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    HealthInformation healthInfoObj;
    auto error = healthInfoObj.FromCommonPublicApi(*healthInformation);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(move(error));
    }

    HealthReportSendOptionsUPtr sendOptionsObj;
    if (sendOptions != nullptr)
    {
        sendOptionsObj = make_unique<HealthReportSendOptions>();
        error = sendOptionsObj->FromPublicApi(*sendOptions);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(error));
        }
    }

    {
        AcquireReadLock grab(lock_);
        if (!isValidPartition_)
        {
            return ComUtility::OnPublicApiReturn(ErrorCode(ErrorCodeValue::ObjectClosed));
        }
    }

    auto healthReport = ServiceModel::HealthReport::GeneratePartitionHealthReport(move(healthInfoObj), partitionId_);
    
    FailoverUnitProxySPtr failoverUnitProxySPtr = failoverUnitProxyWPtr_.lock();
    if (failoverUnitProxySPtr)
    {
        error = failoverUnitProxySPtr->ReportHealth(move(healthReport), move(sendOptionsObj));
        return ComUtility::OnPublicApiReturn(move(error));
    }

    return ComUtility::OnPublicApiReturn(FABRIC_E_COMMUNICATION_ERROR);
}

HRESULT ComStatefulServicePartition::ReportMoveCost(FABRIC_MOVE_COST moveCost)
{
    FailoverUnitProxySPtr failoverUnitProxySPtr;
    auto error = CheckIfOpenAndLockFailoverUnitProxy(failoverUnitProxySPtr);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error);
    }
    
    vector<LoadBalancingComponent::LoadMetric> loadMetrics;
    std::wstring moveCostMetricName(*LoadBalancingComponent::Constants::MoveCostMetricName);
    loadMetrics.push_back(LoadBalancingComponent::LoadMetric(move(moveCostMetricName), moveCost));

    error = failoverUnitProxySPtr->ReportLoad(move(loadMetrics));
    return ComUtility::OnPublicApiReturn(error);
}

HRESULT ComStatefulServicePartition::CreateTransactionalReplicator(
    __in ::IFabricStateProvider2Factory * factory,
    __in ::IFabricDataLossHandler * dataLossHandler,
    __in_opt ::FABRIC_REPLICATOR_SETTINGS const *replicatorSettings,
    __in_opt ::TRANSACTIONAL_REPLICATOR_SETTINGS const * transactionalReplicatorSettings,
    __in_opt KTLLOGGER_SHARED_LOG_SETTINGS const * ktlloggerSharedSettings,
    __out ::IFabricPrimaryReplicator ** primaryReplicator,
    __out void ** transactionalReplicator)
{
    FailoverUnitProxySPtr failoverUnitProxySPtr;
    Common::ComPointer<IFabricCodePackageActivationContext> activationContext;

    HRESULT hr = PrepareToCreateReplicator(failoverUnitProxySPtr);

    if (!SUCCEEDED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    ASSERT_IFNOT(failoverUnitProxySPtr, "FailoverUnit proxy should be valid");

    ErrorCode error = failoverUnitProxySPtr->ApplicationHostObj.GetCodePackageActivationContext(
        failoverUnitProxySPtr->RuntimeId,
        activationContext);

    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error);
    }

    FailoverUnitProxySPtr forReplicator = failoverUnitProxySPtr;

    hr = failoverUnitProxySPtr->TransactionalReplicatorFactory.CreateReplicator(
        replicaId_, 
        failoverUnitProxySPtr->ReplicatorFactory,
        this, 
        replicatorSettings,
        transactionalReplicatorSettings,
        ktlloggerSharedSettings,
        *activationContext.GetRawPointer(),
        hasPersistedState_,
        move(forReplicator),
        factory,
        dataLossHandler,
        primaryReplicator,
        (PHANDLE)transactionalReplicator);

    if (FAILED(hr))
    {
        RAPEventSource::Events->SFPartitionFailedToCreateReplicator(
            static_cast<_int64>(replicaId_),
            hr);
    }

    return ComUtility::OnPublicApiReturn(hr);
}

HRESULT ComStatefulServicePartition::CreateTransactionalReplicatorInternal(
    __in ::IFabricTransactionalReplicatorRuntimeConfigurations * runtimeConfigurations,
    __in ::IFabricStateProvider2Factory * factory,
    __in ::IFabricDataLossHandler * dataLossHandler,
    __in_opt ::FABRIC_REPLICATOR_SETTINGS const *replicatorSettings,
    __in_opt ::TRANSACTIONAL_REPLICATOR_SETTINGS const * transactionalReplicatorSettings,
    __in_opt KTLLOGGER_SHARED_LOG_SETTINGS const * ktlloggerSharedSettings, 
    __out ::IFabricPrimaryReplicator ** primaryReplicator,
    __out void ** transactionalReplicator)
{
    if (!runtimeConfigurations)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    FailoverUnitProxySPtr failoverUnitProxySPtr;
    HRESULT hr = PrepareToCreateReplicator(failoverUnitProxySPtr);

    if (!SUCCEEDED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    FailoverUnitProxySPtr forReplicator = failoverUnitProxySPtr;

    hr = failoverUnitProxySPtr->TransactionalReplicatorFactory.CreateReplicator(
        replicaId_, 
        failoverUnitProxySPtr->ReplicatorFactory,
        this, 
        replicatorSettings,
        transactionalReplicatorSettings,
        ktlloggerSharedSettings,
        runtimeConfigurations,
        hasPersistedState_,
        move(forReplicator),
        factory,
        dataLossHandler,
        primaryReplicator,
        (PHANDLE)transactionalReplicator);

    if (FAILED(hr))
    {
        RAPEventSource::Events->SFPartitionFailedToCreateReplicator(
            static_cast<_int64>(replicaId_),
            hr);
    }

    return ComUtility::OnPublicApiReturn(hr);
}

HRESULT ComStatefulServicePartition::GetKtlSystem(
    __out void** ktlSystem)
{
    FailoverUnitProxySPtr failoverUnitProxySPtr = failoverUnitProxyWPtr_.lock();
    if (!failoverUnitProxySPtr)
    {
        RAPEventSource::Events->SFPartitionCouldNotGetFUP(static_cast<_int64>(replicaId_));
        return ComUtility::OnPublicApiReturn(ErrorCode(ErrorCodeValue::ObjectClosed));
    }

    KtlSystem * ktlSystemPtr = nullptr;
    ErrorCode error = failoverUnitProxySPtr->ApplicationHostObj.GetKtlSystem(&ktlSystemPtr);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error);
    }
   
    *ktlSystem = ktlSystemPtr;
    return ComUtility::OnPublicApiReturn(error);
}

void ComStatefulServicePartition::SetReadWriteStatus(ReadWriteStatusValue && value)
{
    AcquireWriteLock grab(lock_);
    readWriteStatus_ = move(value);
}

void ComStatefulServicePartition::OnServiceOpened()
{
    AcquireWriteLock grab(lock_);
    
    isServiceOpened_ = true;
}

FABRIC_SERVICE_PARTITION_ACCESS_STATUS ComStatefulServicePartition::GetReadStatusForQuery() 
{
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS rv = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
    HRESULT hr = GetReadStatus(&rv);
    if (FAILED(hr))
    {
        return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
    }

    return rv;
}

FABRIC_SERVICE_PARTITION_ACCESS_STATUS ComStatefulServicePartition::GetWriteStatusForQuery() 
{
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS rv = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
    HRESULT hr = GetWriteStatus(&rv);
    if (FAILED(hr))
    {
        return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
    }

    return rv;
}

HRESULT ComStatefulServicePartition::PrepareToCreateReplicator(__out FailoverUnitProxySPtr & failoverUnitProxySPtr)
{
    RAPEventSource::Events->SFPartitionCreatingReplicator(static_cast<_int64>(replicaId_));
    AcquireWriteLock grab(lock_);

    if (!isValidPartition_)
    {
        RAPEventSource::Events->SFPartitionInvalidReplica(static_cast<_int64>(replicaId_));
        return ErrorCode(ErrorCodeValue::ObjectClosed).ToHResult();
    }

    if (createReplicatorAlreadyCalled_ || isServiceOpened_)
    {
        RAPEventSource::Events->SFPartitionReplicaAlreadyOpen(
            static_cast<_int64>(replicaId_),
            isServiceOpened_,
            createReplicatorAlreadyCalled_);
        return ErrorCode(ErrorCodeValue::InvalidState).ToHResult();
    }

    createReplicatorAlreadyCalled_ = true;

    failoverUnitProxySPtr = failoverUnitProxyWPtr_.lock();

    if (!failoverUnitProxySPtr)
    {
        RAPEventSource::Events->SFPartitionCouldNotGetFUP(static_cast<_int64>(replicaId_));
        return ErrorCode(ErrorCodeValue::ObjectClosed).ToHResult();
    }

    return S_OK;
}
