// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ServiceModel;

 ComStatelessServicePartition::ComStatelessServicePartition(
    Guid const & partitionId,
    ConsistencyUnitDescription const & consistencyUnitDescription,
    FABRIC_REPLICA_ID replicaId,
    std::weak_ptr<FailoverUnitProxy> && failoverUnitProxyWPtr)
	: ComServicePartitionBase(std::move(failoverUnitProxyWPtr)),
    partitionInfo_(partitionId, consistencyUnitDescription),
    partitionId_(partitionId),
    replicaId_(replicaId)
 {
 }

 void ComStatelessServicePartition::AssertIsValid(Common::Guid const & partitionId, FABRIC_REPLICA_ID replicaId, FailoverUnitProxy const & owner) const
 {
     AcquireReadLock grab(lock_);
     ASSERT_IF(partitionId_ != partitionId, "PartitionId is invalid {0}", owner);
     ASSERT_IF(replicaId_ != replicaId, "ReplicaId is invalid {0}", owner);
     ASSERT_IF(!isValidPartition_, "Partition is closed {0}", owner);
 }


HRESULT ComStatelessServicePartition::GetPartitionInfo(FABRIC_SERVICE_PARTITION_INFORMATION const **bufferedValue)
{
    if (!bufferedValue)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    AcquireReadLock grab(lock_);

    if(!isValidPartition_)
    {
        return ComUtility::OnPublicApiReturn(ErrorCode(ErrorCodeValue::ObjectClosed));
    }

    *bufferedValue = partitionInfo_.Value;

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT ComStatelessServicePartition::ReportLoad(ULONG metricCount, FABRIC_LOAD_METRIC const *metrics)
{
	return ComServicePartitionBase::ReportLoad(metricCount, metrics);
}

HRESULT ComStatelessServicePartition::ReportFault(FABRIC_FAULT_TYPE faultType)
{
    if (
        faultType != FABRIC_FAULT_TYPE_PERMANENT &&
        faultType != FABRIC_FAULT_TYPE_TRANSIENT)
    {
        return ComUtility::OnPublicApiReturn(ErrorCode(ErrorCodeValue::InvalidArgument));
    }

    {
        AcquireReadLock grab(lock_);
        if(!isValidPartition_)
        {
            return ComUtility::OnPublicApiReturn(ErrorCode(ErrorCodeValue::ObjectClosed));
        }
    }

    FailoverUnitProxySPtr failoverUnitProxySPtr = failoverUnitProxyWPtr_.lock();
    if(failoverUnitProxySPtr)
    {
        ErrorCode error = failoverUnitProxySPtr->ReportFault(FaultType::FromPublicAPI(faultType));
        return ComUtility::OnPublicApiReturn(move(error));
    }

    return ComUtility::OnPublicApiReturn(FABRIC_E_COMMUNICATION_ERROR);
}


HRESULT ComStatelessServicePartition::ReportInstanceHealth(FABRIC_HEALTH_INFORMATION const *healthInformation)
{
    return ReportInstanceHealth2(healthInformation, nullptr);
}

HRESULT ComStatelessServicePartition::ReportInstanceHealth2(
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

    auto healthReport = ServiceModel::HealthReport::GenerateInstanceHealthReport(move(healthInfoObj), partitionId_, replicaId_);
    
    FailoverUnitProxySPtr failoverUnitProxySPtr = failoverUnitProxyWPtr_.lock();
    if (failoverUnitProxySPtr)
    {
        error = failoverUnitProxySPtr->ReportHealth(move(healthReport), move(sendOptionsObj));
        return ComUtility::OnPublicApiReturn(move(error));
    }

    return ComUtility::OnPublicApiReturn(FABRIC_E_COMMUNICATION_ERROR);
}

HRESULT ComStatelessServicePartition::ReportPartitionHealth(FABRIC_HEALTH_INFORMATION const *healthInformation)
{
    return ReportPartitionHealth2(healthInformation, nullptr);
}

HRESULT ComStatelessServicePartition::ReportPartitionHealth2(
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

HRESULT ComStatelessServicePartition::ReportMoveCost(FABRIC_MOVE_COST moveCost)
{
    {
        AcquireReadLock grab(lock_);
        if (!isValidPartition_)
        {
            return ComUtility::OnPublicApiReturn(ErrorCode(ErrorCodeValue::ObjectClosed));
        }
    }

    FailoverUnitProxySPtr failoverUnitProxySPtr = failoverUnitProxyWPtr_.lock();
    if (failoverUnitProxySPtr)
    {
        vector<LoadBalancingComponent::LoadMetric> loadMetrics;
        std::wstring moveCostMetricName(*LoadBalancingComponent::Constants::MoveCostMetricName);
        loadMetrics.push_back(LoadBalancingComponent::LoadMetric(move(moveCostMetricName), moveCost));

        ErrorCode error = failoverUnitProxySPtr->ReportLoad(move(loadMetrics));
        return ComUtility::OnPublicApiReturn(move(error));
    }

    return ComUtility::OnPublicApiReturn(FABRIC_E_COMMUNICATION_ERROR);
}

void ComStatelessServicePartition::ClosePartition()
{
    AcquireWriteLock grab(lock_);
    ASSERT_IFNOT(isValidPartition_, "ComStatelessServicePartition duplicate close call.");
    isValidPartition_ = false;
}
