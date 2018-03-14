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

StringLiteral const TraceComponent("DeployedServicePackageEntity");

DeployedServicePackageEntity::DeployedServicePackageEntity(
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
    , parentDeployedApplication_(
        healthManagerReplica.PartitionedReplicaId, 
        this->EntityIdString, 
        HealthEntityKind::DeployedServicePackage,
        [this]() { return this->EntityManager->DeployedApplications.GetEntity(DeployedApplicationHealthId(this->EntityId.ApplicationName, this->EntityId.NodeId)); })
    , parentNode_(
        healthManagerReplica.PartitionedReplicaId, 
        this->EntityIdString, 
        [this]() { return this->EntityManager->Nodes.GetEntity(this->EntityId.NodeId); })
{
    entityId_ = GetCastedAttributes(this->InternalAttributes).EntityId;
}

DeployedServicePackageEntity::~DeployedServicePackageEntity()
{
}

HEALTH_ENTITY_TEMPLATED_METHODS_DEFINITIONS(DeployedServicePackageAttributesStoreData, DeployedServicePackageEntity, DeployedServicePackageHealthId)

bool DeployedServicePackageEntity::get_HasHierarchySystemReport() const
{
    return parentDeployedApplication_.HasSystemReport && parentNode_.HasSystemReport;
}

Common::ErrorCode DeployedServicePackageEntity::HasAttributeMatchCallerHoldsLock(
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
    
    if (parentNode_.HasSameNodeInstance<DeployedServicePackageEntity>(this->InternalAttributes))
    {
        error = parentNode_.HasAttributeMatch(attributeName, attributeValue, /*out*/hasMatch);
        if (!error.IsError(ErrorCodeValue::InvalidArgument))
        {
            // Argument not set, not matching or matching
            return error;
        }
    }

    error = parentDeployedApplication_.HasAttributeMatch(attributeName, attributeValue, /*out*/hasMatch);
    if (!error.IsError(ErrorCodeValue::InvalidArgument))
    {
        // Argument not set, not matching or matching
        return error;
    }

    // TODO: check application attributes 
    
    return error;
}

void DeployedServicePackageEntity::UpdateParents(Common::ActivityId const & activityId)
{
    parentDeployedApplication_.Update(activityId);
    parentNode_.Update(activityId);
}

bool DeployedServicePackageEntity::DeleteDueToParentsCallerHoldsLock() const
{
    return parentNode_.ShouldDeleteChild<DeployedServicePackageEntity>(this->InternalAttributes);
}

void DeployedServicePackageEntity::OnEntityReadyToAcceptRequests(
    Common::ActivityId const & activityId)
{
    // Create the parent deployed application if it doesn't exist
    // and update the children list to point to this entity
    this->EntityManager->DeployedApplications.ExecuteEntityWorkUnderCacheLock(
        activityId,
        DeployedApplicationHealthId(this->EntityId.ApplicationName, this->EntityId.NodeId),
        [this](HealthEntitySPtr const & entity)
        {
            DeployedApplicationsCache::GetCastedEntityPtr(entity)->AddDeployedServicePackage(this->shared_from_this());
        });
}

Common::ErrorCode DeployedServicePackageEntity::EvaluateHealth(
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

Common::ErrorCode DeployedServicePackageEntity::UpdateContextHealthPoliciesCallerHoldsLock(
    __in QueryRequestContext & context)
{
    if (!context.ApplicationPolicy)
    {
        // Parent has been updated, so it can't be stale 
        auto deployedApplication = parentDeployedApplication_.GetLockedParent();
        if (!deployedApplication)
        {
            HMEvents::Trace->NoParent(this->EntityIdString, context.ActivityId, HealthEntityKind::DeployedApplication);
            return ErrorCode(
                ErrorCodeValue::HealthEntityNotFound,
                wformatString("{0} {1}", Resources::GetResources().ParentDeployedApplicationNotFound, this->EntityIdString));
        }

        shared_ptr<ApplicationHealthPolicy> hierarchyAppHealthPolicy;
        auto error = DeployedApplicationsCache::GetCastedEntityPtr(deployedApplication)->GetCachedApplicationHealthPolicy(context.ActivityId, hierarchyAppHealthPolicy);
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

Common::ErrorCode DeployedServicePackageEntity::SetDetailQueryResult(
    __in QueryRequestContext & context,
    FABRIC_HEALTH_STATE entityEventsState,
    std::vector<ServiceModel::HealthEvent> && queryEvents,
    std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations)
{
    auto attributes = this->GetAttributesCopy();
    auto health = make_unique<DeployedServicePackageHealth>(
        this->EntityId.ApplicationName,
        this->EntityId.ServiceManifestName,
        this->EntityId.ServicePackageActivationId,
        GetCastedAttributes(attributes).NodeName,
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
