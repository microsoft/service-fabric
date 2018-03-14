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

StringLiteral const TraceComponent("PartitionEntity");

PartitionEntity::PartitionEntity(
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
    , replicas_(healthManagerReplica.PartitionedReplicaId, this->EntityIdString)
    , replicasLock_()
    , hasUpdatedParentService_(false)
    , parentService_(
        healthManagerReplica.PartitionedReplicaId, 
        this->EntityIdString, 
        HealthEntityKind::Service,
        [this]() { return this->GetParent(); })
{
    entityId_ = GetCastedAttributes(this->InternalAttributes).EntityId;
}

PartitionEntity::~PartitionEntity()
{
}

HEALTH_ENTITY_TEMPLATED_METHODS_DEFINITIONS( PartitionAttributesStoreData, PartitionEntity, PartitionHealthId )

HealthEntitySPtr PartitionEntity::GetParent()
{
    auto attributes = this->GetAttributesCopy();
    auto const & attrib = GetCastedAttributes(attributes);
    if (attrib.AttributeSetFlags.IsServiceNameSet())
    {
        return this->EntityManager->Services.GetEntity(ServiceHealthId(attrib.ServiceName));
    }
    
    return nullptr;
}

void PartitionEntity::AddReplica(ReplicaEntitySPtr const & replica)
{
    AcquireWriteLock lock(replicasLock_);
    replicas_.AddChild(replica);
}

std::set<ReplicaEntitySPtr> PartitionEntity::GetReplicas()
{
    AcquireWriteLock lock(replicasLock_);
    return replicas_.GetChildren();
}

bool PartitionEntity::get_HasHierarchySystemReport() const
{
    return parentService_.HasSystemReport;
}

Common::ErrorCode PartitionEntity::HasAttributeMatchCallerHoldsLock(
    std::wstring const & attributeName,
    std::wstring const & attributeValue,
    __inout bool & hasMatch) const
{
    auto error = this->InternalAttributes->HasAttributeMatch(attributeName, attributeValue, /*out*/hasMatch);
    if (!error.IsError(ErrorCodeValue::InvalidArgument))
    {
        // Argument not set, not matching or matching
        return error;
    }
    
    // Service checks the parent application attributes as well.
    error = parentService_.HasAttributeMatch(attributeName, attributeValue, /*out*/hasMatch);
    if (!error.IsError(ErrorCodeValue::InvalidArgument))
    {
        // Argument not set, not matching or matching
        return error;
    }
    
    // TODO: check application attributes

    return error;
}

void PartitionEntity::OnEntityReadyToAcceptRequests(Common::ActivityId const &)
{
    // Do nothing, the parent is created when all attributes are set
    // (called by CreateOrUpdateNonPersistentParents)
}

void PartitionEntity::CreateOrUpdateNonPersistentParents(Common::ActivityId const & activityId) 
{
    if (!hasUpdatedParentService_.load())
    {
        auto attributes = this->GetAttributesCopy();
        auto & castedAttributes = GetCastedAttributes(attributes);
        if (castedAttributes.AttributeSetFlags.IsServiceNameSet())
        {
            bool expected = false;
            if (hasUpdatedParentService_.compare_exchange_weak(expected, true))
            {
                this->EntityManager->Services.ExecuteEntityWorkUnderCacheLock(
                    activityId,
                    ServiceHealthId(castedAttributes.ServiceName),
                    [this](HealthEntitySPtr const & entity)
                    {
                        ServicesCache::GetCastedEntityPtr(entity)->AddPartition(this->shared_from_this());
                    });
            }
        }
    }
}

void PartitionEntity::UpdateParents(Common::ActivityId const & activityId)
{
    parentService_.Update(activityId);
}

Common::ErrorCode PartitionEntity::EvaluateHealth(
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

    wstring serviceTypeName;
    error = this->GetServiceTypeName(activityId, serviceTypeName);
    if (!error.IsSuccess())
    {
        return error;
    }

    auto replicasFilter = make_unique<ReplicaHealthStatesFilter>(FABRIC_HEALTH_STATE_FILTER_NONE);
    vector<ReplicaAggregatedHealthState> replicas;
    return EvaluateReplicas(
        activityId,
        serviceTypeName,
        healthPolicy,
        replicasFilter,
        healthStats,
        aggregatedHealthState,
        unhealthyEvaluations,
        replicas);
}

Common::ErrorCode PartitionEntity::EvaluateFilteredHealth(
    Common::ActivityId const & activityId,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    ServiceModel::ReplicaHealthStateFilterList const & replicaFilters,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::ReplicaHealthStateChunkList & replicas)
{
    aggregatedHealthState = FABRIC_HEALTH_STATE_UNKNOWN;
    auto error = GetEventsHealthState(activityId, healthPolicy.ConsiderWarningAsError, aggregatedHealthState);
    if (!error.IsSuccess())
    {
        return error;
    }

    wstring serviceTypeName;
    error = this->GetServiceTypeName(activityId, serviceTypeName);
    if (!error.IsSuccess())
    {
        return error;
    }

    return EvaluateReplicaChunks(
        activityId,
        serviceTypeName,
        healthPolicy,
        replicaFilters,
        aggregatedHealthState,
        replicas);
}

Common::ErrorCode PartitionEntity::GetCachedApplicationHealthPolicy(
    Common::ActivityId const & activityId,
    __inout std::shared_ptr<ServiceModel::ApplicationHealthPolicy> & appHealthPolicy)
{
    parentService_.Update(activityId);
    auto service = parentService_.GetLockedParent();
    if (!service)
    {
        HMEvents::Trace->NoParent(this->EntityIdString, activityId, HealthEntityKind::Service);
        return ErrorCode(
            ErrorCodeValue::HealthEntityNotFound,
            wformatString("{0} {1}", Resources::GetResources().ParentServiceNotFound, this->EntityIdString));
    }

    return ServicesCache::GetCastedEntityPtr(service)->GetCachedApplicationHealthPolicy(activityId, appHealthPolicy);
}

Common::ErrorCode PartitionEntity::UpdateContextHealthPoliciesCallerHoldsLock(
    __in QueryRequestContext & context)
{
    if (!context.ApplicationPolicy)
    {
        // Parent has been updated, so it can't be stale 
        auto service = parentService_.GetLockedParent();
        if (!service)
        {
            HMEvents::Trace->NoParent(this->EntityIdString, context.ActivityId, HealthEntityKind::Service);
            return ErrorCode(
                ErrorCodeValue::HealthEntityNotFound,
                wformatString("{0} {1}", Resources::GetResources().ParentServiceNotFound, this->EntityIdString));
        }

        shared_ptr<ApplicationHealthPolicy> hierarchyAppHealthPolicy;
        auto error = ServicesCache::GetCastedEntityPtr(service)->GetCachedApplicationHealthPolicy(context.ActivityId, hierarchyAppHealthPolicy);
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

Common::ErrorCode PartitionEntity::GetReplicasAggregatedHealthStates(
    __in QueryRequestContext & context)
{
    FABRIC_REPLICA_ID lastReplicaId = FABRIC_INVALID_REPLICA_ID;
    bool checkContinuationToken = false;
    if (!context.ContinuationToken.empty())
    {
        auto error = PagingStatus::GetContinuationTokenData<FABRIC_REPLICA_ID>(context.ContinuationToken, lastReplicaId);
        if (!error.IsSuccess())
        {
            HealthManagerReplica::WriteInfo(
                TraceComponent,
                "{0}: {1}: error interpreting the continuation token {2} as FABRIC_REPLICA_ID: {3} {4}",
                this->PartitionedReplicaId.TraceId,
                context.ActivityId,
                context.ContinuationToken,
                error,
                error.Message);
            return error;
        }

        checkContinuationToken = true;
    }

    std::set<ReplicaEntitySPtr> unsortedReplicas = GetReplicas();
    // Sort by replica id
    map<FABRIC_REPLICA_ID, ReplicaEntitySPtr> replicas;
    for (auto it = unsortedReplicas.begin(); it != unsortedReplicas.end(); ++it)
    {
        if (!checkContinuationToken || (*it)->EntityId.ReplicaId > lastReplicaId)
        {
            if ((*it)->Match(context.Filters))
            {
                replicas.insert(make_pair((*it)->EntityId.ReplicaId, move(*it)));
            }
        }
    }

    // For each replica, compute the aggregated health using the replica entity policy
    ListPager<ReplicaAggregatedHealthState> pager;
    for (auto it = replicas.begin(); it != replicas.end(); ++it)
    {
        auto & replica = it->second;

        FABRIC_HEALTH_STATE replicaHealthState; 
        vector<HealthEvaluation> unhealthyEvaluations;
        auto error = replica->EvaluateHealth(context.ActivityId, *context.ApplicationPolicy, /*out*/replicaHealthState, unhealthyEvaluations);
        if (error.IsSuccess())
        {
            auto attributes = replica->GetAttributesCopy();
            FABRIC_SERVICE_KIND kind = FABRIC_SERVICE_KIND_STATEFUL;
            if (!attributes->UseInstance)
            {
                kind = FABRIC_SERVICE_KIND_STATELESS;
            }

            error = pager.TryAdd(
                ReplicaAggregatedHealthState(
                    kind,
                    replica->EntityId.PartitionId,
                    replica->EntityId.ReplicaId,
                    replicaHealthState));
            if (error.IsError(ErrorCodeValue::EntryTooLarge))
            {
                HMEvents::Trace->Query_MaxMessageSizeReached(
                    this->EntityIdString,
                    context.ActivityId,
                    replica->EntityIdString,
                    error,
                    error.Message);
                break;
            }
        }
    }

    context.AddQueryListResults<ReplicaAggregatedHealthState>(move(pager));

    return ErrorCode::Success();
}

Common::ErrorCode PartitionEntity::EvaluateReplicaChunks(
    Common::ActivityId const & activityId,
    std::wstring const & serviceTypeName,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    ServiceModel::ReplicaHealthStateFilterList const & replicaFilters,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::ReplicaHealthStateChunkList & childrenHealthStates)
{
    std::set<ReplicaEntitySPtr> replicas = GetReplicas();

    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);
    std::vector<HealthEvaluation> childUnhealthyEvaluationsList;
    HealthCount replicaHealthCount;

    ULONG totalCount = 0;
    for (auto it = replicas.begin(); it != replicas.end(); ++it)
    {
        auto & replica = *it;

        // Find the filter that applies to this replica
        auto itReplicaFilter = replicaFilters.cend();
        for (auto itFilter = replicaFilters.cbegin(); itFilter != replicaFilters.cend(); ++itFilter)
        {
            if (itFilter->ReplicaOrInstanceIdFilter == replica->EntityId.ReplicaId)
            {
                // Found the most specific filter, use it
                itReplicaFilter = itFilter;
                break;
            }

            if (itFilter->ReplicaOrInstanceIdFilter == FABRIC_INVALID_REPLICA_ID)
            {
                // Default filter
                itReplicaFilter = itFilter;
            }
        }

        FABRIC_HEALTH_STATE healthState;
        std::vector<HealthEvaluation> childUnhealthyEvaluations;
        auto error = replica->EvaluateHealth(activityId, healthPolicy, /*out*/healthState, /*out*/childUnhealthyEvaluations);
        if (error.IsSuccess())
        {
            if (itReplicaFilter != replicaFilters.cend() && itReplicaFilter->IsRespected(healthState))
            {
                ++totalCount;
                childrenHealthStates.Add(ReplicaHealthStateChunk(replica->EntityId.ReplicaId, healthState));
            }

            if (canImpactAggregatedHealth)
            {
                replicaHealthCount.AddResult(healthState);
            }
        }
    }

    childrenHealthStates.TotalCount = totalCount;

    if (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
    {
        auto serviceTypeHealthPolicy = healthPolicy.GetServiceTypeHealthPolicy(serviceTypeName);
        this->UpdateHealthState(
            activityId,
            HealthEntityKind::Replica,
            replicaHealthCount,
            serviceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition,
            aggregatedHealthState);
    }

    return ErrorCode::Success();
}

Common::ErrorCode PartitionEntity::EvaluateReplicas(
    Common::ActivityId const & activityId,
    std::wstring const & serviceTypeName,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    std::unique_ptr<ServiceModel::ReplicaHealthStatesFilter> const & replicasFilter,
    HealthStatisticsUPtr const & healthStats,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
    __inout std::vector<ServiceModel::ReplicaAggregatedHealthState> & childrenHealthStates)
{
    std::set<ReplicaEntitySPtr> replicas = GetReplicas();

    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);
    std::vector<HealthEvaluation> childUnhealthyEvaluationsList;
    HealthCount replicaHealthCount;
    HealthCount totalReplicaCount;
    
    for (auto it = replicas.begin(); it != replicas.end(); ++it)
    {
        auto & replica = *it;
        
        FABRIC_HEALTH_STATE healthState; 
        std::vector<HealthEvaluation> childUnhealthyEvaluations;
        auto error = replica->EvaluateHealth(activityId, healthPolicy, /*out*/healthState, /*out*/childUnhealthyEvaluations);
        if (error.IsSuccess())
        {
            totalReplicaCount.AddResult(healthState);
            if (!replicasFilter || replicasFilter->IsRespected(healthState))
            {
                auto attributes = replica->GetAttributesCopy();
                FABRIC_SERVICE_KIND kind = FABRIC_SERVICE_KIND_STATEFUL;
                if (!attributes->UseInstance)
                {
                    kind = FABRIC_SERVICE_KIND_STATELESS;
                }

                childrenHealthStates.push_back(ReplicaAggregatedHealthState(
                    kind,
                    replica->EntityId.PartitionId,
                    replica->EntityId.ReplicaId,
                    healthState));
            }

            if (canImpactAggregatedHealth)
            {
                replicaHealthCount.AddResult(healthState);
                if (!childUnhealthyEvaluations.empty())
                {
                    childUnhealthyEvaluationsList.push_back(HealthEvaluation(make_shared<ReplicaHealthEvaluation>(
                        replica->EntityId.PartitionId,
                        replica->EntityId.ReplicaId,
                        healthState,
                        move(childUnhealthyEvaluations))));
                }
            }
        }
    }

    if (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
    {
        auto serviceTypeHealthPolicy = healthPolicy.GetServiceTypeHealthPolicy(serviceTypeName);
        if (UpdateHealthState(
            activityId,
            HealthEntityKind::Replica,
            replicaHealthCount,
            serviceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition,
            aggregatedHealthState))
        {
            unhealthyEvaluations.clear();
            unhealthyEvaluations.push_back(
                HealthEvaluation(make_shared<ReplicasHealthEvaluation>(
                aggregatedHealthState,
                HealthCount::FilterUnhealthy(childUnhealthyEvaluationsList, aggregatedHealthState),
                replicaHealthCount.TotalCount,
                serviceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition)));
        }
    }

    if (healthStats)
    {
        healthStats->Add(EntityKind::Replica, totalReplicaCount.GetHealthStateCount());
    }

    return ErrorCode::Success();
}

Common::ErrorCode PartitionEntity::GetServiceTypeName(
    Common::ActivityId const & activityId,
    __inout std::wstring & serviceTypeName)
{
    // Get service type specific health policy
    parentService_.Update(activityId);
    auto serviceAttributes = parentService_.Attributes;
    if (!serviceAttributes)
    {
        HMEvents::Trace->NoParent(this->EntityIdString, activityId, HealthEntityKind::Service);
        return ErrorCode(
            ErrorCodeValue::HealthEntityNotFound,
            wformatString("{0} {1}", Resources::GetResources().ParentServiceNotFound, this->EntityIdString));
    }

    auto & castedServiceAttributes = ServiceEntity::GetCastedAttributes(serviceAttributes);
    if (!castedServiceAttributes.AttributeSetFlags.IsServiceTypeNameSet())
    {
        return ErrorCode(
            ErrorCodeValue::HealthEntityNotFound,
            wformatString("{0} {1}", Resources::GetResources().ServiceTypeNotSet, castedServiceAttributes.EntityId.ServiceName));
    }

    serviceTypeName = castedServiceAttributes.ServiceTypeName;

    return ErrorCode::Success();
}

Common::ErrorCode PartitionEntity::SetDetailQueryResult(
    __in QueryRequestContext & context,
    FABRIC_HEALTH_STATE entityEventsState,
    std::vector<ServiceModel::HealthEvent> && queryEvents,
    std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations)
{
    ASSERT_IFNOT(context.ApplicationPolicy, "{0}: application health policy not set", context);
    ASSERT_IF(context.ContextKind != RequestContextKind::QueryEntityDetail, "{0}: context kind not supported", context);

    wstring serviceTypeName;
    auto error = GetServiceTypeName(context.ActivityId, serviceTypeName);
    if (!error.IsSuccess())
    {
        return error;
    }

    auto aggregatedHealthState = entityEventsState;
    std::unique_ptr<ReplicaHealthStatesFilter> replicasFilter;
    error = context.GetReplicasFilter(replicasFilter);
    if (!error.IsSuccess())
    {
        return error;
    }

    PartitionHealthStatisticsFilterUPtr healthStatsFilter;
    error = context.GetPartitionHealthStatsFilter(healthStatsFilter);
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

    vector<ReplicaAggregatedHealthState> childrenHealthStates;
    error = EvaluateReplicas(
        context.ActivityId,
        serviceTypeName,
        *context.ApplicationPolicy,
        replicasFilter,
        healthStats,
        aggregatedHealthState,
        unhealthyEvaluations,
        childrenHealthStates);
    if (!error.IsSuccess())
    {
        return error;
    }

    auto health = make_unique<PartitionHealth>(
        this->EntityId,
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

Common::ErrorCode PartitionEntity::SetChildrenAggregatedHealthQueryResult(
    __in QueryRequestContext & context)
{
    ASSERT_IF(
        context.ChildrenKind != HealthEntityKind::Replica, 
        "{0}: {1}: SetChildrenAggregatedHealthQueryResult: can't return children of type {2}", 
        this->PartitionedReplicaId.TraceId, 
        this->EntityIdString, 
        context.ChildrenKind);
    ASSERT_IFNOT(context.ApplicationPolicy, "{0}: application health policy not set", context);

    return GetReplicasAggregatedHealthStates(context);
}

Common::ErrorCode PartitionEntity::SetEntityHealthState(
    __in QueryRequestContext & context,
    FABRIC_HEALTH_STATE entityEventsState)
{
    ASSERT_IFNOT(context.ApplicationPolicy, "{0}: application health policy not set", context);
    ASSERT_IF(context.ContextKind != RequestContextKind::QueryEntityHealthState, "SetEntityHealthState {0}: context kind not supported", context);

    ListPager<PartitionAggregatedHealthState> pager;
    if (!context.ContinuationToken.empty())
    {
        Guid lastPartitionId = Guid::Empty();
        auto error = PagingStatus::GetContinuationTokenData<Guid>(context.ContinuationToken, lastPartitionId);
        if (!error.IsSuccess())
        {
            HealthManagerReplica::WriteInfo(
                TraceComponent,
                "{0}: {1}: error interpreting the continuation token {2} as guid: {3} {4}",
                this->PartitionedReplicaId.TraceId,
                context.ActivityId,
                context.ContinuationToken,
                error,
                error.Message);
            return error;
        }

        if (this->EntityId <= lastPartitionId)
        {
            // Doesn't respect continuation token, do not return
            HealthManagerReplica::WriteInfo(
                TraceComponent,
                this->EntityIdString,
                "{0}: {1}: continuation token {2} not respected",
                this->PartitionedReplicaId.TraceId,
                context.ActivityId,
                context.ContinuationToken);
            context.AddQueryListResults<PartitionAggregatedHealthState>(move(pager));
            return ErrorCode::Success();
        }
    }

    if (!this->Match(context.Filters))
    {
        // Can't include the result, since it doesn't match other filters
        context.AddQueryListResults<PartitionAggregatedHealthState>(move(pager));
        return ErrorCode::Success();
    }
    
    wstring serviceTypeName;
    auto error = GetServiceTypeName(context.ActivityId, serviceTypeName);
    if (!error.IsSuccess()) { return error; }
    
    auto aggregatedHealthState = entityEventsState;

    auto replicasFilter = make_unique<ReplicaHealthStatesFilter>(FABRIC_HEALTH_STATE_FILTER_NONE);
    vector<ReplicaAggregatedHealthState> replicas;
    vector<HealthEvaluation> unhealthyEvaluations;
    // Evaluate replicas and do not include the health stats
    error = EvaluateReplicas(
        context.ActivityId,
        serviceTypeName,
        *context.ApplicationPolicy,
        replicasFilter,
        nullptr,
        aggregatedHealthState,
        unhealthyEvaluations,
        replicas);
    if (!error.IsSuccess()) { return error; }
    ASSERT_IFNOT(replicas.empty(), "{0}: no replicas should be returned", context);

    error = pager.TryAdd(PartitionAggregatedHealthState(this->EntityId, aggregatedHealthState));
    if (!error.IsSuccess()) { return error; }

    context.AddQueryListResults<PartitionAggregatedHealthState>(move(pager));
    return ErrorCode::Success();
}

bool PartitionEntity::CleanupChildren()
{
    AcquireWriteLock lock(replicasLock_);
    return replicas_.CleanupChildren();
}
