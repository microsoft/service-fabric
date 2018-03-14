// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

ComServicePartitionBase::ComServicePartitionBase(std::weak_ptr<FailoverUnitProxy> && failoverUnitProxyWPtr)
: lock_(),
isValidPartition_(true),
failoverUnitProxyWPtr_(std::move(failoverUnitProxyWPtr))
{
}

HRESULT ComServicePartitionBase::ReportLoad(ULONG metricCount, FABRIC_LOAD_METRIC const *metrics)
{
    if (!metrics)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    FAIL_IF_RESERVED_FIELD_NOT_NULL(metrics);

    FailoverUnitProxySPtr failoverUnitProxySPtr;
    auto error = CheckIfOpenAndLockFailoverUnitProxy(failoverUnitProxySPtr);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error);
    }

    vector<LoadBalancingComponent::LoadMetric> loadMetrics;
    for (ULONG i = 0; i < metricCount; i++)
    {
        if (metrics[i].Name == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        if (metrics[i].Value >= INT_MAX)
        {
            RAPEventSource::Events->LoadReportingInvalidLoad(failoverUnitProxySPtr->RAPId, failoverUnitProxySPtr->FailoverUnitId.Guid, failoverUnitProxySPtr->CurrentReplicatorRole);
            return ComUtility::OnPublicApiReturn(FABRIC_E_VALUE_TOO_LARGE);
        }

        if (PLBLoadUtility::IsMetricDefined(failoverUnitProxySPtr->ServiceDescription, metrics[i].Name))
        {
            loadMetrics.push_back(LoadBalancingComponent::LoadMetric(wstring(metrics[i].Name), metrics[i].Value));
        }
        else
        {
            RAPEventSource::Events->LoadReportingUndefinedMetric(metrics[i].Name, failoverUnitProxySPtr->FailoverUnitId.Guid);
        }
    }

    error = failoverUnitProxySPtr->ReportLoad(move(loadMetrics));
    return ComUtility::OnPublicApiReturn(error);
}

ErrorCode ComServicePartitionBase::CheckIfOpen(Common::AcquireReadLock &)
{
    if (!isValidPartition_)
    {
        return ErrorCode(ErrorCodeValue::ObjectClosed);
    }

    return ErrorCode::Success();
}

ErrorCode ComServicePartitionBase::CheckIfOpen()
{
    AcquireReadLock grab(lock_);
    return CheckIfOpen(grab);
}

ErrorCode ComServicePartitionBase::CheckIfOpenAndLockFailoverUnitProxy(FailoverUnitProxySPtr & failoverUnitProxy)
{
    auto error = CheckIfOpen();
    if (!error.IsSuccess())
    {
        return error;
    }

    return LockFailoverUnitProxy(failoverUnitProxy);
}

ErrorCode ComServicePartitionBase::LockFailoverUnitProxy(FailoverUnitProxySPtr & failoverUnitProxy)
{
    failoverUnitProxy = failoverUnitProxyWPtr_.lock();
    if (!failoverUnitProxy)
    {
        return ErrorCode(ErrorCodeValue::ObjectClosed);
    }

    return ErrorCode::Success();
}
