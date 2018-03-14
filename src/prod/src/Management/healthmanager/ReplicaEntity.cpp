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

StringLiteral const TraceComponent("ReplicaEntity");

ReplicaEntity::ReplicaEntity(
    AttributesStoreDataSPtr && attributes,
    __in HealthManagerReplica & healthManagerReplica,
    HealthEntityState::Enum entityState)
    : HealthEntity(
        move(attributes), 
        healthManagerReplica, 
        false /*considerSystemErrorUnhealthy*/,
        true /*expectSystemReports*/, 
        entityState)
    , entityId_()
    , parentPartition_(
        healthManagerReplica.PartitionedReplicaId, 
        this->EntityIdString, 
        HealthEntityKind::Partition,
        [this]() { return this->EntityManager->Partitions.GetEntity(this->EntityId.PartitionId); })
    , parentNode_(
        healthManagerReplica.PartitionedReplicaId, 
        this->EntityIdString, 
        [this]() { return this->GetParent(); })
{
    entityId_ = GetCastedAttributes(this->InternalAttributes).EntityId;
}

ReplicaEntity::~ReplicaEntity()
{
}

HEALTH_ENTITY_TEMPLATED_METHODS_DEFINITIONS(ReplicaAttributesStoreData, ReplicaEntity, ReplicaHealthId)

HealthEntitySPtr ReplicaEntity::GetParent()
{
    auto attributes = this->GetAttributesCopy();
    auto const & attrib = GetCastedAttributes(attributes);
    if (attrib.AttributeSetFlags.IsNodeIdSet())
    {
        return this->EntityManager->Nodes.GetEntity(attrib.NodeId);
    }

    return nullptr;
}

bool ReplicaEntity::get_HasHierarchySystemReport() const
{
    return parentNode_.HasSystemReport && parentPartition_.HasSystemReport;
}

Common::ErrorCode ReplicaEntity::HasAttributeMatchCallerHoldsLock(
    std::wstring const & attributeName,
    std::wstring const & attributeValue,
    __out bool & hasMatch) const
{
    auto error = this->InternalAttributes->HasAttributeMatch(attributeName, attributeValue, /*out*/hasMatch);
    if (!error.IsError(ErrorCodeValue::InvalidArgument))
    {
        // Argument not set, not matching or matching
        return error;
    }
    
    if (parentNode_.HasSameNodeInstance<ReplicaEntity>(this->InternalAttributes))
    {
        error = parentNode_.HasAttributeMatch(attributeName, attributeValue, /*out*/hasMatch);
        if (!error.IsError(ErrorCodeValue::InvalidArgument))
        {
            // Argument not set, not matching or matching
            return error;
        }
    }

    error = parentPartition_.HasAttributeMatch(attributeName, attributeValue, /*out*/hasMatch);
    if (!error.IsError(ErrorCodeValue::InvalidArgument))
    {
        // Argument not set, not matching or matching
        return error;
    }
    
    // TODO: check service and application attributes as well
    
    return error;
}

Common::ErrorCode ReplicaEntity::EvaluateHealth(
    Common::ActivityId const & activityId,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations)
{
    aggregatedHealthState = FABRIC_HEALTH_STATE_UNKNOWN;
    auto error = GetEventsHealthState(activityId, healthPolicy.ConsiderWarningAsError, aggregatedHealthState, unhealthyEvaluations);
    // Do not generate evaluation description, this must be done by the caller
    return error;
}

Common::ErrorCode ReplicaEntity::UpdateContextHealthPoliciesCallerHoldsLock(
    __in QueryRequestContext & context)
{
    if (!context.ApplicationPolicy)
    {
        // Parent has been updated 
        auto partition = parentPartition_.GetLockedParent();
        if (!partition)
        {
            HMEvents::Trace->NoParent(this->EntityIdString, context.ActivityId, HealthEntityKind::Partition);
            return ErrorCode(
                ErrorCodeValue::HealthEntityNotFound,
                wformatString("{0} {1}", Resources::GetResources().ParentPartitionNotFound, this->EntityIdString));
        }
        
        shared_ptr<ApplicationHealthPolicy> hierarchyAppHealthPolicy;
        auto error = PartitionsCache::GetCastedEntityPtr(partition)->GetCachedApplicationHealthPolicy(context.ActivityId, hierarchyAppHealthPolicy);
        if (!error.IsSuccess())
        {
            return error;
        }
         
        // Save the application health policy taken from application attributes into the context,
        // to be used for processing
        context.ApplicationPolicy = move(hierarchyAppHealthPolicy);
    }
    
    return ErrorCode(ErrorCodeValue::Success);
}

void ReplicaEntity::UpdateParents(Common::ActivityId const & activityId)
{
    parentPartition_.Update(activityId);
    parentNode_.Update(activityId);
}

bool ReplicaEntity::DeleteDueToParentsCallerHoldsLock() const
{
    return parentNode_.ShouldDeleteChild<ReplicaEntity>(this->InternalAttributes);
}

void ReplicaEntity::OnEntityReadyToAcceptRequests(
    Common::ActivityId const & activityId)
{
    // Update the relationship between partition and replica:
    // If partition exists, set the parentPartition weak_ptr for replica
    // and add the replica as child of the parent.
    // Otherwise, create a partition that is not persisted to store to remember the hierarchy
    this->EntityManager->Partitions.ExecuteEntityWorkUnderCacheLock(
        activityId,
        this->EntityId.PartitionId,
        [this](HealthEntitySPtr const & entity)
        {
            PartitionsCache::GetCastedEntityPtr(entity)->AddReplica(this->shared_from_this());
        });
}

Common::ErrorCode ReplicaEntity::SetDetailQueryResult(
    __in QueryRequestContext & context,
    FABRIC_HEALTH_STATE entityEventsState,
    std::vector<ServiceModel::HealthEvent> && queryEvents,
    std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations)
{
    auto attributes = this->GetAttributesCopy();
    FABRIC_SERVICE_KIND kind = FABRIC_SERVICE_KIND_STATEFUL;
    if (!attributes->UseInstance)
    {
        kind = FABRIC_SERVICE_KIND_STATELESS;
    }

    auto health = make_unique<ReplicaHealth>(
        kind,
        this->EntityId.PartitionId,
        this->EntityId.ReplicaId,
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

    context.SetQueryResult(QueryResult(std::move(health)));
    return ErrorCode::Success();
}
