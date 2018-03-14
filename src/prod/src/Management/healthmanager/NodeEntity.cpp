// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace ServiceModel;
using namespace Store;
using namespace Management::HealthManager;

NodeEntity::NodeEntity(
    AttributesStoreDataSPtr && attributes,
    __in HealthManagerReplica & healthManagerReplica,
    HealthEntityState::Enum entityState)
    : HealthEntity(
        move(attributes), 
        healthManagerReplica, 
        true /*considerSystemErrorUnhealthy*/, 
        true /*expectSystemReports*/, 
        entityState)
    , entityId_()
{
    entityId_ = GetCastedAttributes(this->InternalAttributes).EntityId;
}

NodeEntity::~NodeEntity()
{
}

HEALTH_ENTITY_TEMPLATED_METHODS_DEFINITIONS( NodeAttributesStoreData, NodeEntity, NodeHealthId )

Common::ErrorCode NodeEntity::EvaluateHealth(
    Common::ActivityId const & activityId,
    ServiceModel::ClusterHealthPolicy const & healthPolicy,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations)
{
    aggregatedHealthState = FABRIC_HEALTH_STATE_UNKNOWN;

    auto error = GetEventsHealthState(
        activityId,
        healthPolicy.ConsiderWarningAsError,
        true, // setUnhealthyEvaluations
        aggregatedHealthState,
        unhealthyEvaluations);

    // Do not generate evaluation description, this must be done by the caller
    return error;
}

Common::ErrorCode NodeEntity::UpdateContextHealthPoliciesCallerHoldsLock(
    __in QueryRequestContext & context)
{
    if (!context.ClusterPolicy)
    {
        shared_ptr<ClusterHealthPolicy> policy;
        auto error = this->EntityManager->Cluster.GetClusterHealthPolicy(policy);

        if (!error.IsSuccess())
        {
            return error;
        }
        
        context.ClusterPolicy = move(policy);
    }
    
    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode NodeEntity::SetDetailQueryResult(
    __in QueryRequestContext & context,
    FABRIC_HEALTH_STATE entityEventsState,
    std::vector<ServiceModel::HealthEvent> && queryEvents,
    std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations)
{
    auto attributes = this->GetAttributesCopy();
    auto & castedAttributes = GetCastedAttributes(attributes);

    // Create node health without passing the unhealthy evaluations.
    auto health = make_unique<NodeHealth>(
        castedAttributes.NodeName,
        move(queryEvents),
        entityEventsState,
        move(unhealthyEvaluations));

    auto error = health->UpdateUnhealthyEvalautionsPerMaxAllowedSize(QueryRequestContext::GetMaxAllowedSize());
    if (!error.IsSuccess())
    {
        return error;
    }

    HMEvents::Trace->Entity_QueryCompleted(
        EntityIdString,
        context,
        *health,
        context.GetFilterDescription());
     
    context.SetQueryResult(ServiceModel::QueryResult(std::move(health)));
    return ErrorCode::Success();
}
