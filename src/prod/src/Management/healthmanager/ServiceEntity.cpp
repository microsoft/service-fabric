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

StringLiteral const TraceComponent("ServiceEntity");

ServiceEntity::ServiceEntity(
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
    , partitions_(healthManagerReplica.PartitionedReplicaId, this->EntityIdString)
    , hasUpdatedParent_(false)
    , parentApplication_(
        healthManagerReplica.PartitionedReplicaId, 
        this->EntityIdString, 
        HealthEntityKind::Application,
        [this]() { return this->GetParent(); })
{
    entityId_ = GetCastedAttributes(this->InternalAttributes).EntityId;
}

ServiceEntity::~ServiceEntity()
{
}

HEALTH_ENTITY_TEMPLATED_METHODS_DEFINITIONS( ServiceAttributesStoreData, ServiceEntity, ServiceHealthId )

HealthEntitySPtr ServiceEntity::GetParent()
{
    auto attributes = this->GetAttributesCopy();
    auto const & attrib = GetCastedAttributes(attributes);
    if (attrib.AttributeSetFlags.IsApplicationNameSet())
    {
        return this->EntityManager->Applications.GetEntity(ApplicationHealthId(attrib.ApplicationName));
    }
     
    HealthManagerReplica::WriteNoise(TraceComponent, "{0}: GetParent: Application name not set", attrib);
    return nullptr;
}

// Called on load from store successful, report with attributes changed
void ServiceEntity::CreateOrUpdateNonPersistentParents(Common::ActivityId const & activityId) 
{
    if (!hasUpdatedParent_.load())
    {
        auto attributes = this->GetAttributesCopy();
        auto & castedAttributes = GetCastedAttributes(attributes);
        if (castedAttributes.AttributeSetFlags.IsApplicationNameSet())
        {
            bool expected = false;
            if (hasUpdatedParent_.compare_exchange_weak(expected, true))
            {
                this->EntityManager->Applications.ExecuteEntityWorkUnderCacheLock(
                    activityId,
                    ApplicationHealthId(castedAttributes.ApplicationName),
                    [this](HealthEntitySPtr const & entity)
                    {
                        ApplicationsCache::GetCastedEntityPtr(entity)->AddService(this->shared_from_this());
                    });
            }
        }
    }
}

void ServiceEntity::AddPartition(PartitionEntitySPtr const & partition)
{
    partitions_.AddChild(partition);
}

std::set<PartitionEntitySPtr> ServiceEntity::GetPartitions()
{
    return partitions_.GetChildren();
}

bool ServiceEntity::get_HasHierarchySystemReport() const
{
    return parentApplication_.HasSystemReport;
}

Common::ErrorCode ServiceEntity::HasAttributeMatchCallerHoldsLock(
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
    
    error = parentApplication_.HasAttributeMatch(attributeName, attributeValue, /*out*/hasMatch);
    if (!error.IsError(ErrorCodeValue::InvalidArgument))
    {
        // Argument not set, not matching or matching
        return error;
    }
    
    return error;
}

void ServiceEntity::UpdateParents(Common::ActivityId const & activityId)
{
    parentApplication_.Update(activityId);
}

Common::ErrorCode ServiceEntity::EvaluateHealth(
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

    auto partitionsFilter = make_unique<PartitionHealthStatesFilter>(FABRIC_HEALTH_STATE_FILTER_NONE);
    vector<PartitionAggregatedHealthState> partitions;
    return EvaluatePartitions(
        activityId,
        serviceTypeName,
        healthPolicy,
        partitionsFilter,
        healthStats,
        aggregatedHealthState,
        unhealthyEvaluations,
        partitions);
}

Common::ErrorCode ServiceEntity::EvaluateFilteredHealth(
    Common::ActivityId const & activityId,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    ServiceModel::PartitionHealthStateFilterList const & partitionFilters,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::PartitionHealthStateChunkList & partitions)
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

    return EvaluatePartitionChunks(
        activityId,
        serviceTypeName,
        healthPolicy,
        partitionFilters,
        aggregatedHealthState,
        partitions);
}

Common::ErrorCode ServiceEntity::GetCachedApplicationHealthPolicy(
    Common::ActivityId const & activityId,
    __inout std::shared_ptr<ServiceModel::ApplicationHealthPolicy> & appHealthPolicy)
{
    // ServiceTypeName must be set in order to get the service specific health policy
    auto attrib = this->GetAttributesCopy();
    auto & castedAttributes = GetCastedAttributes(attrib);
    if (!castedAttributes.AttributeSetFlags.IsServiceTypeNameSet())
    {
        HealthManagerReplica::WriteNoise(TraceComponent, "{0}: {1}: Service type name not set", castedAttributes, activityId);
        return ErrorCode(
            ErrorCodeValue::HealthEntityNotFound,
            wformatString("{0} {1}", Resources::GetResources().ServiceTypeNotSet, this->EntityIdString));
    }

    parentApplication_.Update(activityId);
    auto application = parentApplication_.GetLockedParent();
    if (!application)
    {
        HMEvents::Trace->NoParent(this->EntityIdString, activityId, HealthEntityKind::Application);
        return ErrorCode(
            ErrorCodeValue::HealthEntityNotFound,
            wformatString("{0} {1}", Resources::GetResources().ParentApplicationNotFound, this->EntityIdString));
    }

    return ApplicationsCache::GetCastedEntityPtr(application)->GetApplicationHealthPolicy(appHealthPolicy);
}

Common::ErrorCode ServiceEntity::UpdateContextHealthPoliciesCallerHoldsLock(
    __in QueryRequestContext & context)
{
    // ServiceTypeName must be set in order to get the service specific health policy
    auto & castedAttributes = GetCastedAttributes(this->InternalAttributes);
    if (!castedAttributes.AttributeSetFlags.IsServiceTypeNameSet())
    {
        return ErrorCode(
            ErrorCodeValue::HealthEntityNotFound,
            wformatString("{0} {1}", Resources::GetResources().ServiceTypeNotSet, this->EntityIdString));
    }

    if (!context.ApplicationPolicy)
    {
        auto application = parentApplication_.GetLockedParent();
        if (!application)
        {
            HMEvents::Trace->NoParent(this->EntityIdString, context.ActivityId, HealthEntityKind::Application);
            return ErrorCode(
                ErrorCodeValue::HealthEntityNotFound,
                wformatString("{0} {1}", Resources::GetResources().ParentApplicationNotFound, this->EntityIdString));
        }

        shared_ptr<ApplicationHealthPolicy> hierarchyAppHealthPolicy;
        auto error = ApplicationsCache::GetCastedEntityPtr(application)->GetApplicationHealthPolicy(hierarchyAppHealthPolicy);
        if (!error.IsSuccess())
        {
            return error;
        }
        
        // Save the application health policy taken from application attributes into the context,
        // to be used for processing
        context.ApplicationPolicy = move(hierarchyAppHealthPolicy);
        ASSERT_IFNOT(context.ApplicationPolicy, "{0}: application health policy not set", context);
    }
    
    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode ServiceEntity::GetPartitionsAggregatedHealthStates(
    __in QueryRequestContext & context)
{
    ASSERT_IFNOT(context.ApplicationPolicy, "{0}: health policy not set", context);

    Guid lastPartitionId = Guid::Empty();
    bool checkContinuationToken = false;
    if (!context.ContinuationToken.empty())
    {
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

        checkContinuationToken = true;
    }

    std::set<PartitionEntitySPtr> unsortedPartitions = GetPartitions();
    // Sort by partition id
    map<Guid, PartitionEntitySPtr> partitions;
    for (auto it = unsortedPartitions.begin(); it != unsortedPartitions.end(); ++it)
    {
        if (!checkContinuationToken || (*it)->EntityId > lastPartitionId)
        {
            if ((*it)->Match(context.Filters))
            {
                partitions.insert(make_pair((*it)->EntityId, move(*it)));
            }
        }
    }

    ListPager<PartitionAggregatedHealthState> pager;
    for (auto it = partitions.begin(); it != partitions.end(); ++it)
    {
        auto & partition = it->second;

        FABRIC_HEALTH_STATE partitionHealthState; 
        vector<HealthEvaluation> unhealthyEvaluations;
        auto error = partition->EvaluateHealth(
            context.ActivityId,
            *context.ApplicationPolicy,
            nullptr,
            /*out*/partitionHealthState,
            unhealthyEvaluations);
        if (error.IsSuccess())
        {
            error = pager.TryAdd(PartitionAggregatedHealthState(partition->EntityId, partitionHealthState));
            if (error.IsError(ErrorCodeValue::EntryTooLarge))
            {
                HMEvents::Trace->Query_MaxMessageSizeReached(
                    this->EntityIdString,
                    context.ActivityId,
                    partition->EntityIdString,
                    error,
                    error.Message);
                break;
            }
        }
    }

    context.AddQueryListResults<PartitionAggregatedHealthState>(move(pager));
    return ErrorCode::Success();
}

Common::ErrorCode ServiceEntity::EvaluatePartitionChunks(
    Common::ActivityId const & activityId,
    std::wstring const & serviceTypeName,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    ServiceModel::PartitionHealthStateFilterList const & partitionFilters,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::PartitionHealthStateChunkList & childrenHealthStates)
{
    std::set<PartitionEntitySPtr> partitions = GetPartitions();

    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);
    std::vector<HealthEvaluation> childUnhealthyEvaluationsList;

    HealthCount partitionHealthCount;
    ULONG totalCount = 0;
    for (auto & partition : partitions)
    {
        ErrorCode error(ErrorCodeValue::Success);
    
        // Find the filter that applies to this partition
        auto itPartitionFilter = partitionFilters.cend();
        for (auto itFilter = partitionFilters.cbegin(); itFilter != partitionFilters.cend(); ++itFilter)
        {
            if (itFilter->PartitionIdFilter == partition->EntityId)
            {
                // Found the most specific filter, use it
                itPartitionFilter = itFilter;
                break;
            }
           
            if (itFilter->PartitionIdFilter == Guid::Empty())
            {
                itPartitionFilter = itFilter;
            }
        }
        
        FABRIC_HEALTH_STATE healthState;
        ReplicaHealthStateChunkList replicas;

        if (itPartitionFilter != partitionFilters.cend())
        {
            error = partition->EvaluateFilteredHealth(activityId, healthPolicy, itPartitionFilter->ReplicaFilters, /*out*/healthState, /*out*/replicas);
        }
        else
        {
            ReplicaHealthStateFilterList replicaFilters;
            error = partition->EvaluateFilteredHealth(activityId, healthPolicy, replicaFilters, /*out*/healthState, /*out*/replicas);
        }

        if (error.IsSuccess())
        {
            if (itPartitionFilter != partitionFilters.cend() && itPartitionFilter->IsRespected(healthState))
            {
                // Add child
                ++totalCount;
                childrenHealthStates.Add(PartitionHealthStateChunk(partition->EntityId, healthState, move(replicas)));
            }

            if (canImpactAggregatedHealth)
            {
                partitionHealthCount.AddResult(healthState);
            }
        }
    }

    childrenHealthStates.TotalCount = totalCount;

    if (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
    {
        auto serviceTypeHealthPolicy = healthPolicy.GetServiceTypeHealthPolicy(serviceTypeName);
        UpdateHealthState(
            activityId,
            HealthEntityKind::Partition,
            partitionHealthCount,
            serviceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService,
            aggregatedHealthState);
    }

    return ErrorCode::Success();
}

Common::ErrorCode ServiceEntity::EvaluatePartitions(
    Common::ActivityId const & activityId,
    std::wstring const & serviceTypeName,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    std::unique_ptr<ServiceModel::PartitionHealthStatesFilter> const & partitionsFilter,
    HealthStatisticsUPtr const & healthStats,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
    __inout std::vector<ServiceModel::PartitionAggregatedHealthState> & childrenHealthStates)
{
    std::set<PartitionEntitySPtr> partitions = GetPartitions();

    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);
    std::vector<HealthEvaluation> childUnhealthyEvaluationsList;
    HealthCount partitionHealthCount;
    HealthCount totalPartitionCount;
       
    for (auto it = partitions.begin(); it != partitions.end(); ++it)
    {
        auto & partition = *it;
        
        FABRIC_HEALTH_STATE healthState; 
        std::vector<HealthEvaluation> childUnhealthyEvaluations;
        auto error = partition->EvaluateHealth(
            activityId,
            healthPolicy,
            healthStats,
            /*out*/healthState,
            /*out*/childUnhealthyEvaluations);
        if (error.IsSuccess())
        {
            totalPartitionCount.AddResult(healthState);
            if (!partitionsFilter || partitionsFilter->IsRespected(healthState))
            {
                childrenHealthStates.push_back(PartitionAggregatedHealthState(partition->EntityId, healthState));
            }

            if (canImpactAggregatedHealth)
            {
                partitionHealthCount.AddResult(healthState);
                if (!childUnhealthyEvaluations.empty())
                {
                    childUnhealthyEvaluationsList.push_back(HealthEvaluation(make_shared<PartitionHealthEvaluation>(
                        partition->EntityId,
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
            HealthEntityKind::Partition,
            partitionHealthCount,
            serviceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService,
            aggregatedHealthState))
        {
            unhealthyEvaluations.clear();
            unhealthyEvaluations.push_back(
                HealthEvaluation(make_shared<PartitionsHealthEvaluation>(
                aggregatedHealthState,
                HealthCount::FilterUnhealthy(childUnhealthyEvaluationsList, aggregatedHealthState),
                partitionHealthCount.TotalCount,
                serviceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService)));
        }
    }

    if (healthStats)
    {
        healthStats->Add(EntityKind::Partition, totalPartitionCount.GetHealthStateCount());
        if (totalPartitionCount.TotalCount == 0)
        {
            // No partition children, so no replicas, add 0
            healthStats->Add(EntityKind::Replica);
        }
    }

    return ErrorCode::Success();
}

Common::ErrorCode ServiceEntity::GetServiceTypeName(
    Common::ActivityId const &,
    __inout std::wstring & serviceTypeName)
{
    auto attributes = this->GetAttributesCopy();
    auto & castedAttributes = GetCastedAttributes(attributes);
    if (!castedAttributes.AttributeSetFlags.IsServiceTypeNameSet())
    {
        return ErrorCode(
            ErrorCodeValue::HealthEntityNotFound,
            wformatString("{0} {1}", Resources::GetResources().ServiceTypeNotSet, this->EntityIdString));
    }

    serviceTypeName = castedAttributes.ServiceTypeName;
    return ErrorCode::Success();
}

Common::ErrorCode ServiceEntity::SetDetailQueryResult(
    __in QueryRequestContext & context,
    FABRIC_HEALTH_STATE entityEventsState,
    std::vector<ServiceModel::HealthEvent> && queryEvents,
    std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations)
{
    // The context has the health policy, either the one passed in query or the application saved one
    ASSERT_IFNOT(context.ApplicationPolicy, "{0}: application health policy not set", context);
    ASSERT_IF(context.ContextKind != RequestContextKind::QueryEntityDetail, "{0}: context kind not supported", context);

    std::unique_ptr<PartitionHealthStatesFilter> partitionsFilter;
    auto error = context.GetPartitionsFilter(partitionsFilter);
    if (!error.IsSuccess())
    {
        return error;
    }

    wstring serviceTypeName;
    error = GetServiceTypeName(context.ActivityId, serviceTypeName);
    if (!error.IsSuccess())
    {
        return error;
    }

    ServiceHealthStatisticsFilterUPtr healthStatsFilter;
    error = context.GetServiceHealthStatsFilter(healthStatsFilter);
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
    vector<PartitionAggregatedHealthState> childrenHealthStates;
    error = EvaluatePartitions(
        context.ActivityId,
        serviceTypeName,
        *context.ApplicationPolicy,
        partitionsFilter,
        healthStats,
        aggregatedHealthState,
        unhealthyEvaluations,
        childrenHealthStates);
    if (!error.IsSuccess())
    {
        return error;
    }

    auto health = make_unique<ServiceHealth>(
        this->EntityId.ServiceName,
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

Common::ErrorCode ServiceEntity::SetChildrenAggregatedHealthQueryResult(
    __in QueryRequestContext & context)
{
    ASSERT_IFNOT(context.ApplicationPolicy, "{0}: application health policy not set", context);
    
    switch (context.ChildrenKind)
    {
    case HealthEntityKind::Partition:
        return GetPartitionsAggregatedHealthStates(context);
    default:
        Assert::CodingError(
            "{0}: {1}: SetChildrenAggregatedHealthQueryResult: can't return children of type {2}", 
            this->PartitionedReplicaId.TraceId, 
            this->EntityIdString, 
            context.ChildrenKind);
    }
}

bool ServiceEntity::CleanupChildren()
{
    return partitions_.CleanupChildren();
}
