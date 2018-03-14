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
using namespace Query;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("DeployedApplicationEntity");

DeployedApplicationEntity::DeployedApplicationEntity(
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
    , deployedServicePackages_(healthManagerReplica.PartitionedReplicaId, this->EntityIdString)
    , parentNode_(
        healthManagerReplica.PartitionedReplicaId,
        this->EntityIdString,
        [this]() { return this->EntityManager->Nodes.GetEntity(this->EntityId.NodeId); })
    , parentApplication_(
        healthManagerReplica.PartitionedReplicaId,
        this->EntityIdString,
        HealthEntityKind::Application,
        [this]() { return this->EntityManager->Applications.GetEntity(ApplicationHealthId(this->EntityId.ApplicationName)); })
{
    entityId_ = GetCastedAttributes(this->InternalAttributes).EntityId;
}

DeployedApplicationEntity::~DeployedApplicationEntity()
{
}

HEALTH_ENTITY_TEMPLATED_METHODS_DEFINITIONS( DeployedApplicationAttributesStoreData, DeployedApplicationEntity, DeployedApplicationHealthId )

void DeployedApplicationEntity::AddDeployedServicePackage(DeployedServicePackageEntitySPtr const & deployedServicePackage)
{
    deployedServicePackages_.AddChild(deployedServicePackage);
}

// Return value is only used internally, so do not set the error message
Common::ErrorCode DeployedApplicationEntity::GetUpgradeDomain(
    Common::ActivityId const & activityId,
    __inout std::wstring & upgradeDomain)
{
    parentNode_.Update(activityId);
    auto nodeAttributes = parentNode_.Attributes;
    if (!nodeAttributes)
    {
        // Parent not set
        HMEvents::Trace->NoParent(this->EntityIdString, activityId, HealthEntityKind::Node);
        return Common::ErrorCode(Common::ErrorCodeValue::HealthEntityNotFound);
    }

    auto & castedNodeAttributes = NodeEntity::GetCastedAttributes(nodeAttributes);
    if (!castedNodeAttributes.AttributeSetFlags.IsUpgradeDomainSet())
    {
        HealthManagerReplica::WriteNoise(TraceComponent, this->EntityIdString, "Parent node upgrade domain not set: {0}", castedNodeAttributes);
        return Common::ErrorCode(Common::ErrorCodeValue::HealthEntityNotFound);
    }

    upgradeDomain = castedNodeAttributes.UpgradeDomain;
    return ErrorCode::Success();
}

Common::ErrorCode DeployedApplicationEntity::HasAttributeMatchCallerHoldsLock(
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

    error = parentNode_.HasAttributeMatch(attributeName, attributeValue, /*out*/hasMatch);
    if (!error.IsError(ErrorCodeValue::InvalidArgument))
    {
        // Argument not set, not matching or matching
        return error;
    }

    error = parentApplication_.HasAttributeMatch(attributeName, attributeValue, /*out*/hasMatch);
    if (!error.IsError(ErrorCodeValue::InvalidArgument))
    {
        // Argument not set, not matching or matching
        return error;
    }

    return error;
}

bool DeployedApplicationEntity::get_HasHierarchySystemReport() const
{
    return parentNode_.HasSystemReport && parentApplication_.HasSystemReport;
}

void DeployedApplicationEntity::UpdateParents(Common::ActivityId const & activityId)
{
    parentNode_.Update(activityId);
    parentApplication_.Update(activityId);
}

bool DeployedApplicationEntity::DeleteDueToParentsCallerHoldsLock() const
{
    return parentNode_.ShouldDeleteChild<DeployedApplicationEntity>(this->InternalAttributes);
}

void DeployedApplicationEntity::OnEntityReadyToAcceptRequests(
    Common::ActivityId const & activityId)
{
    // Create the application and link this entity as its child
    this->EntityManager->Applications.ExecuteEntityWorkUnderCacheLock(
        activityId,
        ApplicationHealthId(this->EntityId.ApplicationName),
        [this](HealthEntitySPtr const & entity)
        {
            ApplicationsCache::GetCastedEntityPtr(entity)->AddDeployedApplication(this->shared_from_this());
        });
}

Common::ErrorCode DeployedApplicationEntity::EvaluateHealth(
    Common::ActivityId const & activityId,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    HealthStatisticsUPtr const & healthStats,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations)
{
    aggregatedHealthState = FABRIC_HEALTH_STATE_UNKNOWN;
    auto error = GetEventsHealthState(activityId, healthPolicy.ConsiderWarningAsError, aggregatedHealthState, unhealthyEvaluations);
    if (!error.IsSuccess())
    {
        return error;
    }

    auto deployedServicePackagesFilter = make_unique<DeployedServicePackageHealthStatesFilter>(FABRIC_HEALTH_STATE_FILTER_NONE);
    vector<DeployedServicePackageAggregatedHealthState> deployedServicePackages;
    return EvaluateDeployedServicePackages(activityId, healthPolicy, deployedServicePackagesFilter, healthStats, aggregatedHealthState, unhealthyEvaluations, deployedServicePackages);
}

Common::ErrorCode DeployedApplicationEntity::EvaluateFilteredHealth(
    Common::ActivityId const & activityId,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    ServiceModel::DeployedServicePackageHealthStateFilterList const & deployedServicePackageFilters,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::DeployedServicePackageHealthStateChunkList & deployedServicePackages)
{
    aggregatedHealthState = FABRIC_HEALTH_STATE_UNKNOWN;
    auto error = GetEventsHealthState(activityId, healthPolicy.ConsiderWarningAsError, aggregatedHealthState);
    if (!error.IsSuccess())
    {
        return error;
    }

    return EvaluateDeployedServicePackageChunks(activityId, healthPolicy, deployedServicePackageFilters, aggregatedHealthState, deployedServicePackages);
}

Common::ErrorCode DeployedApplicationEntity::GetCachedApplicationHealthPolicy(
    Common::ActivityId const & activityId,
    __out std::shared_ptr<ServiceModel::ApplicationHealthPolicy> & appHealthPolicy)
{
    parentApplication_.Update(activityId);
    auto application = parentApplication_.GetLockedParent();
    if (!application)
    {
        // Parent not set
        HMEvents::Trace->NoParent(this->EntityIdString, activityId, HealthEntityKind::Application);
        return Common::ErrorCode(
            ErrorCodeValue::HealthEntityNotFound,
            wformatString("{0} {1}", Resources::GetResources().ParentApplicationNotFound, this->EntityIdString));
    }

    return ApplicationsCache::GetCastedEntityPtr(application)->GetApplicationHealthPolicy(appHealthPolicy);
}

Common::ErrorCode DeployedApplicationEntity::UpdateContextHealthPoliciesCallerHoldsLock(
    __in QueryRequestContext & context)
{
    if (!context.ApplicationPolicy)
    {
        shared_ptr<ApplicationHealthPolicy> hierarchyAppHealthPolicy;
        auto application = parentApplication_.GetLockedParent();
        if (!application)
        {
            HMEvents::Trace->NoParent(this->EntityIdString, context.ActivityId, HealthEntityKind::Application);
            return Common::ErrorCode(
                ErrorCodeValue::HealthEntityNotFound,
                wformatString("{0} {1}", Resources::GetResources().ParentApplicationNotFound, this->EntityIdString));
        }

        auto error = ApplicationsCache::GetCastedEntityPtr(application)->GetApplicationHealthPolicy(hierarchyAppHealthPolicy);
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

Common::ErrorCode DeployedApplicationEntity::EvaluateDeployedServicePackageChunks(
    Common::ActivityId const & activityId,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    ServiceModel::DeployedServicePackageHealthStateFilterList const & deployedServicePackageFilters,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::DeployedServicePackageHealthStateChunkList & childrenHealthStates)
{
    auto deployedServicePackages = GetDeployedServicePackages();

    HealthCount deployedServicePackageHealthCount;
    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);
    ULONG totalCount = 0;

    // For each deployedServicePackage, compute the aggregated health using the deployedServicePackage entity policy
    for (auto it = deployedServicePackages.begin(); it != deployedServicePackages.end(); ++it)
    {
        auto & deployedServicePackage = *it;

        // Find the filter that applies to this deployedServicePackage
        auto itDeployedServicePackageFilter = deployedServicePackageFilters.cend();
        for (auto itFilter = deployedServicePackageFilters.cbegin(); itFilter != deployedServicePackageFilters.cend(); ++itFilter)
        {
            if (itFilter->ServicePackageActivationIdFilterSPtr != nullptr &&
                !(itFilter->ServiceManifestNameFilter.empty()))
            {
                if (itFilter->ServiceManifestNameFilter == deployedServicePackage->EntityId.ServiceManifestName &&
                    *(itFilter->ServicePackageActivationIdFilterSPtr) == deployedServicePackage->EntityId.ServicePackageActivationId)
                {
                    itDeployedServicePackageFilter = itFilter;
                    break;
                }
            }

            if(itFilter->ServicePackageActivationIdFilterSPtr != nullptr &&
               itFilter->ServiceManifestNameFilter.empty())
            {
                if (*(itFilter->ServicePackageActivationIdFilterSPtr) == deployedServicePackage->EntityId.ServicePackageActivationId)
                {
                    itDeployedServicePackageFilter = itFilter;
                    break;
                }
            }

            if (itFilter->ServicePackageActivationIdFilterSPtr == nullptr &&
                !(itFilter->ServiceManifestNameFilter.empty()))
            {
                if (itFilter->ServiceManifestNameFilter == deployedServicePackage->EntityId.ServiceManifestName)
                {
                    itDeployedServicePackageFilter = itFilter;
                    break;
                }
            }

            if (itFilter->IsDefaultFilter())
            {
                // Default filter
                itDeployedServicePackageFilter = itFilter;
            }
        }

        FABRIC_HEALTH_STATE healthState;
        std::vector<HealthEvaluation> childUnhealthyEvaluations;
        auto error = deployedServicePackage->EvaluateHealth(activityId, healthPolicy, /*out*/healthState, /*out*/childUnhealthyEvaluations);
        if (error.IsSuccess())
        {
            if (itDeployedServicePackageFilter != deployedServicePackageFilters.cend() && itDeployedServicePackageFilter->IsRespected(healthState))
            {
                totalCount++;
                childrenHealthStates.Add(DeployedServicePackageHealthStateChunk(
                    deployedServicePackage->EntityId.ServiceManifestName,
                    deployedServicePackage->EntityId.ServicePackageActivationId,
                    healthState));
            }

            if (canImpactAggregatedHealth)
            {
                deployedServicePackageHealthCount.AddResult(healthState);
            }
        }
    }

    childrenHealthStates.TotalCount = totalCount;
    UpdateHealthState(
        activityId,
        HealthEntityKind::DeployedServicePackage,
        deployedServicePackageHealthCount,
        0 /*maxPercentNodesWithUnhealthyDeployedApplication*/,
        aggregatedHealthState);

    return ErrorCode::Success();
}

Common::ErrorCode DeployedApplicationEntity::EvaluateDeployedServicePackages(
    Common::ActivityId const & activityId,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    std::unique_ptr<DeployedServicePackageHealthStatesFilter> const & filter,
    HealthStatisticsUPtr const & healthStats,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout std::vector<HealthEvaluation> & unhealthyEvaluations,
    __inout vector<DeployedServicePackageAggregatedHealthState> & childrenHealthStates)
{
    auto deployedServicePackages = GetDeployedServicePackages();

    HealthCount deployedServicePackageHealthCount;
    HealthCount totalDeployedServicePackageCount;

    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);
    std::vector<HealthEvaluation> childUnhealthyEvaluationsList;

    // For each deployedServicePackage, compute the aggregated health using the deployedServicePackage entity policy
    for (auto it = deployedServicePackages.begin(); it != deployedServicePackages.end(); ++it)
    {
        auto & deployedServicePackage = *it;

        FABRIC_HEALTH_STATE healthState;
        std::vector<HealthEvaluation> childUnhealthyEvaluations;
        auto error = deployedServicePackage->EvaluateHealth(activityId, healthPolicy, /*out*/healthState, /*out*/childUnhealthyEvaluations);
        if (error.IsSuccess())
        {
            totalDeployedServicePackageCount.AddResult(healthState);
            auto attributes = deployedServicePackage->GetAttributesCopy();
            auto & castedAttributes = DeployedServicePackageEntity::GetCastedAttributes(attributes);

            if (!filter || filter->IsRespected(healthState))
            {
                childrenHealthStates.push_back(DeployedServicePackageAggregatedHealthState(
                    deployedServicePackage->EntityId.ApplicationName,
                    deployedServicePackage->EntityId.ServiceManifestName,
                    deployedServicePackage->EntityId.ServicePackageActivationId,
                    castedAttributes.NodeName,
                    healthState));
            }

            if (canImpactAggregatedHealth)
            {
                deployedServicePackageHealthCount.AddResult(healthState);

                if (!childUnhealthyEvaluations.empty())
                {
                    childUnhealthyEvaluationsList.push_back(HealthEvaluation(make_shared<DeployedServicePackageHealthEvaluation>(
                        deployedServicePackage->EntityId.ApplicationName,
                        deployedServicePackage->EntityId.ServiceManifestName,
                        deployedServicePackage->EntityId.ServicePackageActivationId,
                        castedAttributes.NodeName,
                        healthState,
                        move(childUnhealthyEvaluations))));
                }
            }
        }
    }

    if (UpdateHealthState(
        activityId,
        HealthEntityKind::DeployedServicePackage,
        deployedServicePackageHealthCount,
        0 /*maxPercentNodesWithUnhealthyDeployedApplication*/,
        aggregatedHealthState))
    {
        unhealthyEvaluations.clear();
        unhealthyEvaluations.push_back(HealthEvaluation(make_shared<DeployedServicePackagesHealthEvaluation>(
            aggregatedHealthState,
            HealthCount::FilterUnhealthy(childUnhealthyEvaluationsList, aggregatedHealthState),
            deployedServicePackageHealthCount.TotalCount)));
    }

    if (healthStats)
    {
        healthStats->Add(EntityKind::DeployedServicePackage, totalDeployedServicePackageCount.GetHealthStateCount());
    }

    return ErrorCode::Success();
}

Common::ErrorCode DeployedApplicationEntity::GetDeployedServicePackagesAggregatedHealthStates(
    __in QueryRequestContext & context)
{
    ASSERT_IFNOT(context.ApplicationPolicy, "{0}: health policy not set", context);

    auto unsortedDeployedServicePackages = GetDeployedServicePackages();

    wstring const & lastContinuationToken = context.ContinuationToken;
    bool checkContinuationToken = !lastContinuationToken.empty();

    //
    // Sort by the ContinuationToken which is:
    // <ServiceManifestName> if ServicePackageActivationId is empty (for backward compatibility)
    // <ServiceManifestName>_<ServicePackageActivationId> otherwise.
    //
    map<wstring /* ContinuationToken */, DeployedServicePackageEntitySPtr> deployedServicePackages;

    for (auto it = unsortedDeployedServicePackages.begin(); it != unsortedDeployedServicePackages.end(); ++it)
    {
        auto currentContinuationToken = DeployedServicePackageAggregatedHealthState::CreateContinuationString(
            (*it)->EntityId.ServiceManifestName,
            (*it)->EntityId.ServicePackageActivationId);

        if (!checkContinuationToken || currentContinuationToken > lastContinuationToken)
        {
            deployedServicePackages.insert(make_pair(move(currentContinuationToken), move(*it)));
        }
    }

    ListPager<DeployedServicePackageAggregatedHealthState> pager;

    // For each deployedServicePackage, compute the aggregated health using the deployedServicePackage entity policy
    for (auto it = deployedServicePackages.begin(); it != deployedServicePackages.end(); ++it)
    {
        auto & deployedServicePackage = it->second;
        if (!deployedServicePackage->Match(context.Filters))
        {
            continue;
        }

        FABRIC_HEALTH_STATE deployedServicePackageHealthState;
        vector<HealthEvaluation> unhealthyEvaluations;
        auto error = deployedServicePackage->EvaluateHealth(context.ActivityId, *context.ApplicationPolicy, /*out*/deployedServicePackageHealthState, unhealthyEvaluations);
        if (error.IsSuccess())
        {
            auto attributes = deployedServicePackage->GetAttributesCopy();
            auto & castedAttributes = DeployedServicePackageEntity::GetCastedAttributes(attributes);
            error = pager.TryAdd(
                DeployedServicePackageAggregatedHealthState(
                    deployedServicePackage->EntityId.ApplicationName,
                    deployedServicePackage->EntityId.ServiceManifestName,
                    deployedServicePackage->EntityId.ServicePackageActivationId,
                    castedAttributes.NodeName,
                    deployedServicePackageHealthState));
            if (error.IsError(ErrorCodeValue::EntryTooLarge))
            {
                HMEvents::Trace->Query_MaxMessageSizeReached(
                    this->EntityIdString,
                    context.ActivityId,
                    deployedServicePackage->EntityIdString,
                    error,
                    error.Message);
                break;
            }
        }
    }

    context.AddQueryListResults<DeployedServicePackageAggregatedHealthState>(move(pager));
    return ErrorCode::Success();
}

Common::ErrorCode DeployedApplicationEntity::SetDetailQueryResult(
    __in QueryRequestContext & context,
    FABRIC_HEALTH_STATE entityEventsState,
    std::vector<ServiceModel::HealthEvent> && queryEvents,
    std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations)
{
    ASSERT_IFNOT(context.ApplicationPolicy, "{0}: application health policy not set", context);
    ASSERT_IF(context.ContextKind != RequestContextKind::QueryEntityDetail, "{0}: context kind not supported", context);

    std::unique_ptr<DeployedServicePackageHealthStatesFilter> filter;
    auto error = context.GetDeployedServicePackagesFilter(filter);
    if (!error.IsSuccess())
    {
        return error;
    }

    DeployedApplicationHealthStatisticsFilterUPtr healthStatsFilter;
    error = context.GetDeployedApplicationHealthStatsFilter(healthStatsFilter);
    if (!error.IsSuccess())
    {
        return error;
    }

    HealthStatisticsUPtr healthStats;
    if (!healthStatsFilter || !healthStatsFilter->ExcludeHealthStatistics)
    {
        // Include healthStats
        healthStats = make_unique<HealthStatistics>();
    }

    auto aggregatedHealthState = entityEventsState;
    vector<DeployedServicePackageAggregatedHealthState> childrenHealthStates;
    error = EvaluateDeployedServicePackages(
        context.ActivityId,
        *context.ApplicationPolicy,
        filter,
        healthStats,
        aggregatedHealthState,
        unhealthyEvaluations,
        childrenHealthStates);
    if (!error.IsSuccess())
    {
        return error;
    }

    auto attributes = this->GetAttributesCopy();
    auto & castedAttributes = DeployedApplicationEntity::GetCastedAttributes(attributes);
    auto health = make_unique<DeployedApplicationHealth>(
        this->EntityId.ApplicationName,
        castedAttributes.NodeName,
        move(queryEvents),
        aggregatedHealthState,
        move(unhealthyEvaluations),
        move(childrenHealthStates),
        move(healthStats));
    
    error = health->UpdateUnhealthyEvalautionsPerMaxAllowedSize(QueryRequestContext::GetMaxAllowedSize());
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

Common::ErrorCode DeployedApplicationEntity::SetChildrenAggregatedHealthQueryResult(
    __in QueryRequestContext & context)
{
    ASSERT_IF(
        context.ChildrenKind != HealthEntityKind::DeployedServicePackage,
        "{0}: {1}: SetChildrenAggregatedHealthQueryResult: can't return children of type {2}",
        this->PartitionedReplicaId.TraceId,
        this->EntityIdString,
        context.ChildrenKind);

    return GetDeployedServicePackagesAggregatedHealthStates(context);
}

std::set<DeployedServicePackageEntitySPtr> DeployedApplicationEntity::GetDeployedServicePackages()
{
    return deployedServicePackages_.GetChildren();
}

bool DeployedApplicationEntity::CleanupChildren()
{
    return deployedServicePackages_.CleanupChildren();
}
