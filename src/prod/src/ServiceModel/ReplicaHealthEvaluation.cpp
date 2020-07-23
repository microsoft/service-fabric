// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ReplicaEvaluation");

INITIALIZE_SIZE_ESTIMATION(ReplicaHealthEvaluation)

ReplicaHealthEvaluation::ReplicaHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_REPLICA)
    , partitionId_(Guid::Empty())
    , replicaId_(FABRIC_INVALID_REPLICA_ID)
{
}

ReplicaHealthEvaluation::ReplicaHealthEvaluation(
    Common::Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    HealthEvaluationList && unhealthyEvaluations)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_REPLICA, aggregatedHealthState, move(unhealthyEvaluations))
    , partitionId_(partitionId)
    , replicaId_(replicaId)
{
}

ReplicaHealthEvaluation::~ReplicaHealthEvaluation()
{
}

ErrorCode ReplicaHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicReplicaHealthEvaluation = heap.AddItem<FABRIC_REPLICA_HEALTH_EVALUATION>();

    publicReplicaHealthEvaluation->Description = heap.AddString(description_);
    publicReplicaHealthEvaluation->PartitionId = partitionId_.AsGUID();
    publicReplicaHealthEvaluation->ReplicaOrInstanceId = replicaId_;
    publicReplicaHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationList);
    if (!error.IsSuccess()) { return error; }
    publicReplicaHealthEvaluation->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();
    
    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_REPLICA;
    publicHealthEvaluation.Value = publicReplicaHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode ReplicaHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicReplicaHealthEvaluation = reinterpret_cast<FABRIC_REPLICA_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    partitionId_ = Guid(publicReplicaHealthEvaluation->PartitionId);
    replicaId_= publicReplicaHealthEvaluation->ReplicaOrInstanceId;

    auto hr = StringUtility::LpcwstrToWstring(publicReplicaHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    aggregatedHealthState_ = publicReplicaHealthEvaluation->AggregatedHealthState;
    
    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicReplicaHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void ReplicaHealthEvaluation::SetDescription()
{
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyReplica,
        replicaId_,
        aggregatedHealthState_);
}
