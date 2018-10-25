// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("ChaosParameters");

ChaosParameters::ChaosParameters()
: maxClusterStabilizationTimeoutInSeconds_(60)
, maxConcurrentFaults_(1)
, waitTimeBetweenIterationsInSeconds_(30)
, waitTimeBetweenFaultsInSeconds_(20)
, timeToRunInSeconds_(4294967295)
, enableMoveReplicaFaults_(true)
, healthPolicy_()
, eventContextMap_()
, chaosTargetFilter_()
{
}

Common::ErrorCode ChaosParameters::FromPublicApi(
    FABRIC_CHAOS_PARAMETERS const & publicParameters)
{

    maxClusterStabilizationTimeoutInSeconds_ = publicParameters.MaxClusterStabilizationTimeoutInSeconds;
    maxConcurrentFaults_ = publicParameters.MaxConcurrentFaults;
    waitTimeBetweenIterationsInSeconds_ = publicParameters.WaitTimeBetweenIterationsInSeconds;
    waitTimeBetweenFaultsInSeconds_ = publicParameters.WaitTimeBetweenFaultsInSeconds;

    // Managed allows only UInt32.MaxValue number of seconds
    if (publicParameters.TimeToRunInSeconds > 4294967295)
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0}", FASResource::GetResources().ChaosTimeToRunTooBig));
    }

    timeToRunInSeconds_ = publicParameters.TimeToRunInSeconds;
    enableMoveReplicaFaults_ = (publicParameters.EnableMoveReplicaFaults == TRUE) ? true : false;

    // EventContextMap
    //
    if (publicParameters.Context != NULL)
    {
        auto error = eventContextMap_.FromPublicApi(*publicParameters.Context);
        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "Failed to populate event context map from public map.");
            return error;
        }
    }

    // Extensions
    //
    if (publicParameters.Reserved != NULL)
    {
        auto parametersEx1 = reinterpret_cast<FABRIC_CHAOS_PARAMETERS_EX1*>(publicParameters.Reserved);

        if (parametersEx1->ClusterHealthPolicy != NULL)
        {
            healthPolicy_ = make_shared<ClusterHealthPolicy>();
            healthPolicy_->FromPublicApi(*const_cast<FABRIC_CLUSTER_HEALTH_POLICY*>(parametersEx1->ClusterHealthPolicy));
        }

        if (parametersEx1->Reserved != NULL)
        {
            auto parametersEx2 = reinterpret_cast<FABRIC_CHAOS_PARAMETERS_EX2*>(parametersEx1->Reserved);

            if (parametersEx2->ChaosTargetFilter != NULL)
            {
                chaosTargetFilter_ = make_shared<ChaosTargetFilter>();
                chaosTargetFilter_->FromPublicApi(*const_cast<FABRIC_CHAOS_TARGET_FILTER*>(parametersEx2->ChaosTargetFilter));
            }
        }
    }

    return ErrorCodeValue::Success;
}

void ChaosParameters::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_PARAMETERS & result) const
{
    result.MaxClusterStabilizationTimeoutInSeconds = maxClusterStabilizationTimeoutInSeconds_;
    result.WaitTimeBetweenIterationsInSeconds = waitTimeBetweenIterationsInSeconds_;
    result.WaitTimeBetweenFaultsInSeconds = waitTimeBetweenFaultsInSeconds_;

    result.MaxConcurrentFaults = maxConcurrentFaults_;
    result.TimeToRunInSeconds = timeToRunInSeconds_;

    result.EnableMoveReplicaFaults = enableMoveReplicaFaults_ ? TRUE : FALSE;

    auto publicContext = heap.AddItem<FABRIC_EVENT_CONTEXT_MAP>();
    eventContextMap_.ToPublicApi(heap, *publicContext);

    result.Context = publicContext.GetRawPointer();

    // Extensions
    //
    auto parametersEx1 = heap.AddItem<FABRIC_CHAOS_PARAMETERS_EX1>();

    if (healthPolicy_)
    {
        auto healthPolicy = heap.AddItem<FABRIC_CLUSTER_HEALTH_POLICY>();
        parametersEx1->ClusterHealthPolicy = healthPolicy.GetRawPointer();
        healthPolicy_->ToPublicApi(heap, *healthPolicy);
    }
    else
    {
        parametersEx1->ClusterHealthPolicy = NULL;
    }

    auto parametersEx2 = heap.AddItem<FABRIC_CHAOS_PARAMETERS_EX2>();

    if (chaosTargetFilter_)
    {
        auto chaosTargetFilter = heap.AddItem<FABRIC_CHAOS_TARGET_FILTER>();
        parametersEx2->ChaosTargetFilter = chaosTargetFilter.GetRawPointer();
        chaosTargetFilter_->ToPublicApi(heap, *chaosTargetFilter);
    }
    else
    {
        parametersEx2->ChaosTargetFilter = NULL;
    }

    parametersEx1->Reserved = parametersEx2.GetRawPointer();

    result.Reserved = parametersEx1.GetRawPointer();
}

Common::ErrorCode ChaosParameters::Validate() const
{
    if (chaosTargetFilter_)
    {
        return chaosTargetFilter_->Validate();
    }

    return ErrorCode::Success();
}
