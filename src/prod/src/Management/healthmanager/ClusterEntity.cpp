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

StringLiteral const TraceComponent("ClusterEntity");
StringLiteral const HealthStatsTimerTag("HealthStats");

// ************************************
// Class that updates the health policy
// Caller must ensure that there is just one operation started.
// ************************************
class ClusterEntity::UpdateClusterHealthPolicyAsyncOperation : public AsyncOperation
{
    DENY_COPY(UpdateClusterHealthPolicyAsyncOperation)
public:
     UpdateClusterHealthPolicyAsyncOperation(
        __in ClusterEntity & owner,
        Common::ActivityId const & activityId,
        ServiceModel::ClusterHealthPolicySPtr const & healthPolicy,
        ServiceModel::ClusterUpgradeHealthPolicySPtr const & upgradeHealthPolicy,
        ServiceModel::ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & root)
        : AsyncOperation(callback, root)
        , owner_(owner)
        , clusterStoreData_(
            *Constants::StoreType_ClusterTypeString,
            healthPolicy,
            upgradeHealthPolicy,
            applicationHealthPolicies,
            ReplicaActivityId(owner.PartitionedReplicaId, activityId))
        , tx_()
        , timeout_(timeout)
    {
    }

    virtual ~UpdateClusterHealthPolicyAsyncOperation() {}

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<UpdateClusterHealthPolicyAsyncOperation>(operation);
        return casted->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        auto state = owner_.State;
        if (!state.CanAcceptQuery() || !owner_.HealthManagerReplicaObj.EntityManager->IsReady())
        {
            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::NotReady));
            return;
        }

        auto error = owner_.HealthManagerReplicaObj.StoreWrapper.CreateSimpleTransaction(clusterStoreData_.ActivityId, tx_);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        if (state.IsInStore)
        {
            error = owner_.HealthManagerReplicaObj.StoreWrapper.Update(tx_, clusterStoreData_);
        }
        else
        {
            error = owner_.HealthManagerReplicaObj.StoreWrapper.Insert(tx_, clusterStoreData_);
        }

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        auto operation = tx_->BeginCommit(
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
            thisSPtr);
        this->OnCommitComplete(operation, true);
    }

private:
    void OnCommitComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != operation->CompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;
        auto error = tx_->EndCommit(operation);
        tx_.reset();

        if (error.IsSuccess())
        {
            // Update in memory attributes
            owner_.ReplaceInMemoryAttributes(make_shared<ClusterAttributesStoreData>(move(clusterStoreData_)));
        }
        else
        {
            HMEvents::Trace->CommitFailed(clusterStoreData_.ReplicaActivityId.TraceId, error, error.Message);
        }

        this->TryComplete(thisSPtr, move(error));
    }

    ClusterEntity & owner_;
    ClusterAttributesStoreData clusterStoreData_;
    IStoreBase::TransactionSPtr tx_;
    TimeSpan timeout_;
};

ClusterEntity::ClusterEntity(
    AttributesStoreDataSPtr && attributes,
    __in HealthManagerReplica & healthManagerReplica,
    HealthEntityState::Enum entityState)
    : HealthEntity(
        move(attributes),
        healthManagerReplica,
        false /*considerSystemErrorUnhealthy*/,
        false /*expectSystemReports*/,
        entityState)
    , useCachedStats_(false)
    , statsTimer_()
    , cachedClusterHealth_()
    , statsLock_()
{
}

ClusterEntity::~ClusterEntity()
{
}

HEALTH_ENTITY_TEMPLATED_METHODS_DEFINITIONS( ClusterAttributesStoreData, ClusterEntity, ClusterHealthId )

Common::AsyncOperationSPtr ClusterEntity::BeginUpdateClusterHealthPolicy(
    Common::ActivityId const & activityId,
    ServiceModel::ClusterHealthPolicySPtr const & healthPolicy,
    ServiceModel::ClusterUpgradeHealthPolicySPtr const & upgradeHealthPolicy,
    ServiceModel::ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UpdateClusterHealthPolicyAsyncOperation>(
        *this,
        activityId,
        healthPolicy,
        upgradeHealthPolicy,
        applicationHealthPolicies,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ClusterEntity::EndUpdateClusterHealthPolicy(
    Common::AsyncOperationSPtr const & operation)
{
    return UpdateClusterHealthPolicyAsyncOperation::End(operation);
}

Common::ErrorCode ClusterEntity::GetClusterHealthPolicy(
    __inout ServiceModel::ClusterHealthPolicySPtr & healthPolicy)
{
    auto attributes = this->GetAttributesCopy();
    auto & castedAttributes = GetCastedAttributes(attributes);
    if (!castedAttributes.AttributeSetFlags.IsHealthPolicySet())
    {
        // If entity can accept queries, return config value
        // otherwise, return error
        if (this->State.CanAcceptQuery())
        {
            healthPolicy = make_shared<ClusterHealthPolicy>(Management::ManagementConfig::GetConfig().GetClusterHealthPolicy());
            return ErrorCode::Success();
        }
        else
        {
            HMEvents::Trace->NoHealthPolicy(this->EntityIdString, this->PartitionedReplicaId.TraceId, castedAttributes);
            return ErrorCode(
                ErrorCodeValue::HealthEntityNotFound,
                wformatString("{0}", Resources::GetResources().ClusterPolicyNotSet));
        }
    }

    healthPolicy = castedAttributes.HealthPolicy;
    return ErrorCode::Success();
}

Common::ErrorCode ClusterEntity::GetClusterUpgradeHealthPolicy(
    __inout ServiceModel::ClusterUpgradeHealthPolicySPtr & upgradeHealthPolicy)
{
    auto attributes = this->GetAttributesCopy();
    auto & castedAttributes = GetCastedAttributes(attributes);
    if (!castedAttributes.AttributeSetFlags.IsUpgradeHealthPolicySet())
    {
        // If entity can accept queries, return config value
        // otherwise, return error
        if (this->State.CanAcceptQuery())
        {
            upgradeHealthPolicy = make_shared<ClusterUpgradeHealthPolicy>(Management::ManagementConfig::GetConfig().GetClusterUpgradeHealthPolicy());
            return ErrorCode::Success();
        }
        else
        {
            HMEvents::Trace->NoHealthPolicy(this->EntityIdString, this->PartitionedReplicaId.TraceId, castedAttributes);
            return ErrorCode(
                ErrorCodeValue::HealthEntityNotFound,
                wformatString("{0}", Resources::GetResources().ClusterPolicyNotSet));
        }
    }

    upgradeHealthPolicy = castedAttributes.UpgradeHealthPolicy;
    return ErrorCode::Success();
}

Common::ErrorCode ClusterEntity::GetClusterAndApplicationHealthPoliciesIfNotSet(
    __inout ServiceModel::ClusterHealthPolicySPtr & healthPolicy,
    __inout ServiceModel::ApplicationHealthPolicyMapSPtr & applicationHealthPolicies)
{
    if (healthPolicy && applicationHealthPolicies)
    {
        return ErrorCode::Success();
    }

    auto attributes = this->GetAttributesCopy();
    if (!this->State.CanAcceptQuery())
    {
        HMEvents::Trace->NoHealthPolicy(this->EntityIdString, this->PartitionedReplicaId.TraceId, *attributes);
        return ErrorCode(
            ErrorCodeValue::HealthEntityNotFound,
            wformatString("{0}", Resources::GetResources().ClusterPolicyNotSet));
    }

    auto & castedAttributes = GetCastedAttributes(attributes);
    if (!healthPolicy)
    {
        if (!castedAttributes.AttributeSetFlags.IsHealthPolicySet())
        {
            healthPolicy = make_shared<ClusterHealthPolicy>(Management::ManagementConfig::GetConfig().GetClusterHealthPolicy());
        }
        else
        {
            healthPolicy = castedAttributes.HealthPolicy;
        }
    }

    if (!applicationHealthPolicies)
    {
        if (!castedAttributes.AttributeSetFlags.IsApplicationHealthPoliciesSet())
        {
            applicationHealthPolicies = make_shared<ApplicationHealthPolicyMap>();
        }
        else
        {
            applicationHealthPolicies = castedAttributes.ApplicationHealthPolicies;
        }
    }

    ASSERT_IFNOT(healthPolicy, "{0}: GetClusterAndApplicationHealthPoliciesIfNotSet: cluster health policy not set", *attributes);
    ASSERT_IFNOT(applicationHealthPolicies, "{0}: GetClusterAndApplicationHealthPoliciesIfNotSet: application health policies not set", *attributes);
    return ErrorCode::Success();
}


Common::ErrorCode ClusterEntity::UpdateContextHealthPoliciesCallerHoldsLock(
    __inout QueryRequestContext & context)
{
    if (!context.ClusterPolicy || !context.ApplicationHealthPolicies)
    {
        auto & castedAttributes = GetCastedAttributes(this->InternalAttributes);

        if (!context.ClusterPolicy)
        {
            if (!castedAttributes.AttributeSetFlags.IsHealthPolicySet())
            {
                // The state was already checked - since the attributes are not set, it means that update was never called
                // Save the attributes in memory but don't save them to store
                context.ClusterPolicy = make_shared<ClusterHealthPolicy>(Management::ManagementConfig::GetConfig().GetClusterHealthPolicy());
            }
            else
            {
                // Save the cluster health policy set in attributes into the context, to be used for processing
                auto copy = castedAttributes.HealthPolicy;
                context.ClusterPolicy = move(copy);
            }
        }

        if (!context.ApplicationHealthPolicies)
        {
            if (!castedAttributes.AttributeSetFlags.IsApplicationHealthPoliciesSet())
            {
                context.ApplicationHealthPolicies = make_shared<ApplicationHealthPolicyMap>();
            }
            else
            {
                // Save the app health policy map set in attributes into the context, to be used for processing
                auto copy = castedAttributes.ApplicationHealthPolicies;
                context.ApplicationHealthPolicies = move(copy);
            }
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode ClusterEntity::GetClusterUpgradeSnapshot(
    Common::ActivityId const & activityId,
    __inout ClusterUpgradeStateSnapshot & snapshot)
{
    std::vector<HealthEntitySPtr> nodes;
    ErrorCode error = HealthManagerReplicaObj.EntityManager->Nodes.GetEntities(activityId, nodes);
    if (!error.IsSuccess()) { return error; }

    shared_ptr<ClusterHealthPolicy> healthPolicy;
    error = GetClusterHealthPolicy(healthPolicy);
    if (!error.IsSuccess()) { return error; }

    std::map<wstring, HealthCount> nodesPerUd;
    HealthCount nodesHealthCount;
    for (auto it = nodes.begin(); it != nodes.end(); ++it)
    {
        auto nodeAttributes = (*it)->GetAttributesCopy();
        auto & castedAttributes = NodeEntity::GetCastedAttributes(nodeAttributes);
        if (!castedAttributes.AttributeSetFlags.IsNodeNameSet())
        {
            HMEvents::Trace->ClusterUpgradeSkipNode(activityId, (*it)->EntityIdString, castedAttributes);
            continue;
        }

        FABRIC_HEALTH_STATE healthState;
        error = (*it)->GetEventsHealthState(activityId, healthPolicy->ConsiderWarningAsError, healthState);
        if (!error.IsSuccess())
        {
            continue;
        }

        nodesHealthCount.AddResult(healthState);

        if (castedAttributes.AttributeSetFlags.IsUpgradeDomainSet())
        {
            wstring const & ud = castedAttributes.UpgradeDomain;
            auto itUd = nodesPerUd.find(ud);
            if (itUd == nodesPerUd.end())
            {
                itUd = nodesPerUd.insert(make_pair(ud, HealthCount())).first;
            }

            itUd->second.AddResult(healthState);
        }
    }

    snapshot.SetGlobalState(nodesHealthCount.ErrorCount, nodesHealthCount.TotalCount);
    for (auto const & entry : nodesPerUd)
    {
        snapshot.AddUpgradeDomainEntry(entry.first, entry.second.ErrorCount, entry.second.TotalCount);
    }

    return ErrorCode::Success();
}

Common::ErrorCode ClusterEntity::AppendToClusterUpgradeSnapshot(
    Common::ActivityId const & activityId,
    std::vector<std::wstring> const & upgradeDomains,
    __inout ClusterUpgradeStateSnapshot & snapshot)
{
    // Determine the UDs that don't have a baseline
    std::map<wstring, HealthCount> nodesPerUd;
    for (wstring const & ud : upgradeDomains)
    {
        if (!snapshot.HasUpgradeDomainEntry(ud))
        {
            nodesPerUd.insert(make_pair(ud, HealthCount()));
        }
    }

    if (nodesPerUd.empty())
    {
        return ErrorCode::Success();
    }

    std::vector<HealthEntitySPtr> nodes;
    ErrorCode error = HealthManagerReplicaObj.EntityManager->Nodes.GetEntities(activityId, nodes);
    if (!error.IsSuccess()) { return error; }

    shared_ptr<ClusterHealthPolicy> healthPolicy;
    error = GetClusterHealthPolicy(healthPolicy);
    if (!error.IsSuccess()) { return error; }

    HealthCount nodesHealthCount;
    for (auto it = nodes.begin(); it != nodes.end(); ++it)
    {
        auto nodeAttributes = (*it)->GetAttributesCopy();
        auto & castedAttributes = NodeEntity::GetCastedAttributes(nodeAttributes);

        if (castedAttributes.AttributeSetFlags.IsUpgradeDomainSet() && castedAttributes.AttributeSetFlags.IsNodeNameSet())
        {
            auto itUd = nodesPerUd.find(castedAttributes.UpgradeDomain);
            if (itUd != nodesPerUd.end())
            {
                FABRIC_HEALTH_STATE healthState;
                error = (*it)->GetEventsHealthState(activityId, healthPolicy->ConsiderWarningAsError, healthState);
                if (!error.IsSuccess())
                {
                    continue;
                }

                itUd->second.AddResult(healthState);
            }
        }
    }

    for (auto const & entry : nodesPerUd)
    {
        if (entry.second.TotalCount == 0)
        {
            HealthManagerReplica::WriteInfo(
                TraceComponent,
                "{0}: no entry was found for UD {1}",
                activityId,
                entry.first);
        }

        snapshot.AddUpgradeDomainEntry(entry.first, entry.second.ErrorCount, entry.second.TotalCount);
    }

    return ErrorCode::Success();
}

Common::ErrorCode ClusterEntity::EvaluateHealth(
    Common::ActivityId const & activityId,
    vector<wstring> const & upgradeDomains,
    ServiceModel::ClusterHealthPolicySPtr const & policy,
    ServiceModel::ClusterUpgradeHealthPolicySPtr const & upgradePolicy,
    ServiceModel::ApplicationHealthPolicyMapSPtr const & appHealthPolicyMap,
    ClusterUpgradeStateSnapshot const & baseline,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations,
    __inout std::vector<std::wstring> & applicationsWithoutAppType)
{
    ErrorCode error(ErrorCodeValue::Success);

    aggregatedHealthState = FABRIC_HEALTH_STATE_UNKNOWN;

    auto healthPolicy = policy;
    auto applicationHealthPolicies = appHealthPolicyMap;
    error = GetClusterAndApplicationHealthPoliciesIfNotSet(healthPolicy, applicationHealthPolicies);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = GetEventsHealthState(activityId, healthPolicy->ConsiderWarningAsError, aggregatedHealthState, unhealthyEvaluations);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = EvaluateNodesForClusterUpgrade(activityId, *healthPolicy, upgradePolicy, upgradeDomains, baseline, aggregatedHealthState, unhealthyEvaluations);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = EvaluateApplicationsForClusterUpgrade(activityId, *healthPolicy, *applicationHealthPolicies, upgradeDomains, aggregatedHealthState, unhealthyEvaluations, applicationsWithoutAppType);
    if (!error.IsSuccess())
    {
        return error;
    }

    return ErrorCode::Success();
}

ErrorCode ClusterEntity::GetNodesAggregatedHealthStates(
    __inout QueryRequestContext & context)
{
    ASSERT_IFNOT(context.ClusterPolicy, "{0}: health policy not set", context);

    ErrorCode error(ErrorCodeValue::Success);
    std::vector<HealthEntitySPtr> nodes;
    if (!context.ContinuationToken.empty())
    {
        NodeId lastNodeId;
        error = PagingStatus::GetContinuationTokenData<NodeId>(context.ContinuationToken, lastNodeId);
        if (!error.IsSuccess())
        {
            HealthManagerReplica::WriteInfo(
                TraceComponent,
                "{0}: error interpreting the continuation token {1} as nodeid: {2} {3}",
                context.ActivityId,
                context.ContinuationToken,
                error,
                error.Message);
            return error;
        }

        HealthManagerReplica::WriteNoise(
            TraceComponent,
            "{0}: continuation token {1} results in nodeid {2}",
            context.ActivityId,
            context.ContinuationToken,
            lastNodeId.IdValue);

        error = HealthManagerReplicaObj.EntityManager->Nodes.GetEntities(context.ActivityId, lastNodeId.IdValue, nodes);
    }
    else
    {
        error = HealthManagerReplicaObj.EntityManager->Nodes.GetEntities(context.ActivityId, nodes);
    }

    if (!error.IsSuccess())
    {
        return error;
    }

    // If performance becomes an issue, consider creating an overload for GetEntities that takes a maxResults parameter,
    // so that we don't add things to "nodes" list that we can't add to the pager.
    ListPager<NodeAggregatedHealthState> pager;
    pager.SetMaxResults(context.MaxResults);
    for (auto it = nodes.begin(); it != nodes.end(); ++it)
    {
        auto node = NodesCache::GetCastedEntityPtr(*it);

        if (!node->Match(context.Filters))
        {
            continue;
        }

        FABRIC_HEALTH_STATE nodeHealthState;
        ServiceModel::HealthEvaluationList unhealthyEvaluations;
        error = node->EvaluateHealth(context.ActivityId, *context.ClusterPolicy, /*out*/nodeHealthState, /*out*/unhealthyEvaluations);
        if (error.IsSuccess())
        {
            auto nodeAttributes = node->GetAttributesCopy();
            auto & castedAttributes = NodeEntity::GetCastedAttributes(nodeAttributes);
            if (castedAttributes.AttributeSetFlags.IsNodeNameSet())
            {
                error = pager.TryAdd(
                    NodeAggregatedHealthState(
                        castedAttributes.NodeName,
                        castedAttributes.EntityId,
                        nodeHealthState));
                bool benignError;
                bool hasError;
                CheckListPagerErrorAndTrace(error, this->EntityIdString, context.ActivityId, castedAttributes.NodeName, hasError, benignError);
                if (hasError && benignError)
                {
                    break;
                }
                else if (hasError && !benignError)
                {
                    return error;
                }
            }
        }
        else
        {
            HMEvents::Trace->InternalMethodFailed(
                this->EntityIdString,
                context.ActivityId,
                L"EvaluateHealth",
                error,
                error.Message);
        }
    }

    context.AddQueryListResults<NodeAggregatedHealthState>(move(pager));
    return ErrorCode::Success();
}

ErrorCode ClusterEntity::EvaluateNodesForClusterUpgrade(
    Common::ActivityId const & activityId,
    ServiceModel::ClusterHealthPolicy const & healthPolicy,
    ServiceModel::ClusterUpgradeHealthPolicySPtr const & upgradePolicy,
    std::vector<std::wstring> const & upgradeDomains,
    ClusterUpgradeStateSnapshot const & baseline,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations)
{
    ErrorCode error(ErrorCodeValue::Success);

    std::vector<HealthEntitySPtr> nodes;
    error = HealthManagerReplicaObj.EntityManager->Nodes.GetEntities(activityId, nodes);
    if (!error.IsSuccess()) { return error; }

    auto upgradeHealthPolicy = upgradePolicy;
    bool checkDelta = false;
    if (baseline.IsValid())
    {
        checkDelta = true;
        if (!upgradeHealthPolicy)
        {
            error = GetClusterUpgradeHealthPolicy(upgradeHealthPolicy);
            if (!error.IsSuccess()) { return error; }
            ASSERT_IFNOT(upgradeHealthPolicy, "{0}: {1}: EvaluateNodesForClusterUpgrade: upgrade health policy is null and delta check is required", this->PartitionedReplicaId, activityId);
        }
    }

    std::map<wstring, GroupHealthStateCount> nodesPerUd;
    for (wstring const & ud : upgradeDomains)
    {
        nodesPerUd.insert(make_pair(ud, GroupHealthStateCount(healthPolicy.MaxPercentUnhealthyNodes)));
    }

    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);
    std::vector<HealthEvaluation> childUnhealthyEvaluationsList;

    HealthCount nodesHealthCount;
    for (auto it = nodes.begin(); it != nodes.end(); ++it)
    {
        auto node = NodesCache::GetCastedEntityPtr(*it);
        auto nodeAttributes = node->GetAttributesCopy();
        auto & castedAttributes = NodeEntity::GetCastedAttributes(nodeAttributes);
        if (!castedAttributes.AttributeSetFlags.IsNodeNameSet())
        {
            HMEvents::Trace->ClusterUpgradeSkipNode(activityId, node->EntityIdString, castedAttributes);
            continue;
        }

        FABRIC_HEALTH_STATE healthState;
        std::vector<HealthEvaluation> childUnhealthyEvaluations;
        error = node->EvaluateHealth(activityId, healthPolicy, /*out*/healthState, /*out*/childUnhealthyEvaluations);
        if (error.IsSuccess())
        {
            if (canImpactAggregatedHealth)
            {
                nodesHealthCount.AddResult(healthState);

                HealthEvaluationBaseSPtr childEvaluation;
                if (!childUnhealthyEvaluations.empty())
                {
                    childEvaluation = make_shared<NodeHealthEvaluation>(
                        castedAttributes.NodeName,
                        healthState,
                        move(childUnhealthyEvaluations));

                    childUnhealthyEvaluationsList.push_back(HealthEvaluation(childEvaluation));
                }

                // Keep track of per-UD nodes
                if (castedAttributes.AttributeSetFlags.IsUpgradeDomainSet())
                {
                    wstring const & upgradeDomain = castedAttributes.UpgradeDomain;
                    auto itNodePerUd = nodesPerUd.find(upgradeDomain);
                    if (itNodePerUd != nodesPerUd.end())
                    {
                        itNodePerUd->second.Add(healthState, move(childEvaluation));
                    }
                }
            }
        }
        else if (!CanIgnoreChildEvaluationError(error))
        {
            return error;
        }
    }

    if (canImpactAggregatedHealth)
    {
        FABRIC_HEALTH_EVALUATION_KIND evaluationKind = FABRIC_HEALTH_EVALUATION_KIND_INVALID;
        if (UpdateHealthState(
            activityId,
            HealthEntityKind::Node,
            nodesHealthCount,
            healthPolicy.MaxPercentUnhealthyNodes,
            aggregatedHealthState))
        {
            evaluationKind = FABRIC_HEALTH_EVALUATION_KIND_NODES;
            // Do not create health evaluation in place, as it may be replaced by another reason in subsequent checks
        }

        // If not error, evaluate global delta threshold
        if (checkDelta && aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
        {
            if (baseline.IsGlobalDeltaRespected(
                    nodesHealthCount.ErrorCount,
                    nodesHealthCount.TotalCount,
                    upgradeHealthPolicy->MaxPercentDeltaUnhealthyNodes))
            {
                HMEvents::Trace->UpgradeGlobalDeltaRespected(
                    activityId,
                    baseline.GlobalUnhealthyState.ErrorCount,
                    baseline.GlobalUnhealthyState.TotalCount,
                    nodesHealthCount,
                    upgradeHealthPolicy->MaxPercentDeltaUnhealthyNodes);
            }
            else
            {
                // Delta threshold is not respected, consider error
                HMEvents::Trace->UpgradeGlobalDeltaNotRespected(
                    this->PartitionedReplicaId.TraceId,
                    activityId,
                    baseline.GlobalUnhealthyState.ErrorCount,
                    baseline.GlobalUnhealthyState.TotalCount,
                    nodesHealthCount,
                    upgradeHealthPolicy->MaxPercentDeltaUnhealthyNodes);

                // Unhealthy evaluation points to delta;
                // we can create the evaluation in place, since this is final reason for failure
                evaluationKind = FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK;
                aggregatedHealthState = FABRIC_HEALTH_STATE_ERROR;
                unhealthyEvaluations.clear();
                unhealthyEvaluations.push_back(
                    HealthEvaluation(make_shared<DeltaNodesCheckHealthEvaluation>(
                        aggregatedHealthState,
                        baseline.GlobalUnhealthyState.ErrorCount,
                        baseline.GlobalUnhealthyState.TotalCount,
                        nodesHealthCount.TotalCount,
                        upgradeHealthPolicy->MaxPercentDeltaUnhealthyNodes,
                        HealthCount::FilterUnhealthy(childUnhealthyEvaluationsList, aggregatedHealthState))));
            }
        }

        // If not error, evaluate nodes health per UD
        if (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
        {
            for (auto it = nodesPerUd.begin(); it != nodesPerUd.end(); ++it)
            {
                if (checkDelta)
                {
                    ULONG baselineErrorCount;
                    ULONG baselineTotalCount;
                    error = baseline.TryGetUpgradeDomainEntry(it->first, baselineErrorCount, baselineTotalCount);
                    if (!error.IsSuccess())
                    {
                        HealthManagerReplica::WriteWarning(
                            TraceComponent,
                            "{0}: {1}: baseline for UD {2} check failed getting upgrade domain entry: {3}",
                            this->PartitionedReplicaId.TraceId,
                            activityId,
                            it->first,
                            error);
                        return error;
                    }

                    if (ClusterUpgradeStateSnapshot::IsDeltaRespected(
                            baselineErrorCount,
                            baselineTotalCount,
                            it->second.Count.ErrorCount,
                            it->second.Count.TotalCount,
                            upgradeHealthPolicy->MaxPercentUpgradeDomainDeltaUnhealthyNodes))
                    {
                        HMEvents::Trace->UpgradeUDDeltaRespected(
                            activityId,
                            it->first,
                            baselineErrorCount,
                            baselineTotalCount,
                            it->second.Count,
                            upgradeHealthPolicy->MaxPercentUpgradeDomainDeltaUnhealthyNodes);

                    }
                    else
                    {
                        // Delta threshold is not respected for UD, consider error
                        HMEvents::Trace->UpgradeUDDeltaNotRespected(
                            this->PartitionedReplicaId.TraceId,
                            activityId,
                            it->first,
                            baselineErrorCount,
                            baselineTotalCount,
                            it->second.Count,
                            upgradeHealthPolicy->MaxPercentUpgradeDomainDeltaUnhealthyNodes);

                        // Unhealthy evaluation points to delta per UD;
                        // we can create the evaluation in place, since this is final reason for failure
                        evaluationKind = FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK;
                        aggregatedHealthState = FABRIC_HEALTH_STATE_ERROR;
                        unhealthyEvaluations.clear();
                        unhealthyEvaluations.push_back(
                            HealthEvaluation(make_shared<UpgradeDomainDeltaNodesCheckHealthEvaluation>(
                                aggregatedHealthState,
                                it->first,
                                baselineErrorCount,
                                baselineTotalCount,
                                it->second.Count.TotalCount,
                                upgradeHealthPolicy->MaxPercentUpgradeDomainDeltaUnhealthyNodes,
                                HealthCount::FilterUnhealthy(childUnhealthyEvaluationsList, aggregatedHealthState))));
                    }
                }
                else
                {
                    auto udHealthState = it->second.GetHealthState();
                    if (udHealthState == FABRIC_HEALTH_STATE_ERROR)
                    {
                        HMEvents::Trace->UnhealthyChildrenPerUd(
                            this->EntityIdString,
                            this->PartitionedReplicaId.TraceId,
                            activityId,
                            HealthEntityKind::Node,
                            it->first,
                            it->second.Count,
                            wformatString(udHealthState),
                            healthPolicy.MaxPercentUnhealthyNodes);

                        aggregatedHealthState = FABRIC_HEALTH_STATE_ERROR;
                        evaluationKind = FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES;
                        auto const & perUdHealthInfo = it->second;
                        unhealthyEvaluations.clear();
                        unhealthyEvaluations.push_back(
                            HealthEvaluation(make_shared<UDNodesHealthEvaluation>(
                                aggregatedHealthState,
                                it->first /*UD*/,
                                perUdHealthInfo.GetUnhealthy(),
                                perUdHealthInfo.Count.TotalCount,
                                perUdHealthInfo.MaxPercentUnhealthy)));
                        break;
                    }
                    else if (it->second.Count.TotalCount > 0)
                    {
                        HMEvents::Trace->ChildrenPerUd(
                            this->EntityIdString,
                            this->PartitionedReplicaId.TraceId,
                            activityId,
                            HealthEntityKind::Node,
                            it->first,
                            it->second.Count,
                            wformatString(udHealthState),
                            healthPolicy.MaxPercentUnhealthyNodes);
                    }
                }
            }
        }

        if (evaluationKind == FABRIC_HEALTH_EVALUATION_KIND_NODES)
        {
            unhealthyEvaluations.clear();
            unhealthyEvaluations.push_back(
                HealthEvaluation(make_shared<NodesHealthEvaluation>(
                    aggregatedHealthState,
                    HealthCount::FilterUnhealthy(childUnhealthyEvaluationsList, aggregatedHealthState),
                    nodesHealthCount.TotalCount,
                    healthPolicy.MaxPercentUnhealthyNodes)));
        }
    }

    return ErrorCode::Success();
}

ErrorCode ClusterEntity::EvaluateNodeChunks(
    Common::ActivityId const & activityId,
    ServiceModel::ClusterHealthPolicy const & healthPolicy,
    ServiceModel::NodeHealthStateFilterList const & nodeFilters,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::NodeHealthStateChunkList & childrenHealthStates)
{
    std::vector<HealthEntitySPtr> nodes;
    ErrorCode error = HealthManagerReplicaObj.EntityManager->Nodes.GetEntities(activityId, nodes);
    if (!error.IsSuccess()) { return error; }

    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);

    HealthCount nodesHealthCount;
    ULONG totalCount = 0;
    for (auto it = nodes.begin(); it != nodes.end(); ++it)
    {
        auto node = NodesCache::GetCastedEntityPtr(*it);
        auto nodeAttributes = node->GetAttributesCopy();
        auto & castedAttributes = NodeEntity::GetCastedAttributes(nodeAttributes);
        if (!castedAttributes.AttributeSetFlags.IsNodeNameSet())
        {
            HMEvents::Trace->QuerySkipNode(activityId, node->EntityIdString, castedAttributes);
            continue;
        }

        // Find the filter that applies to this node
        auto itNodeFilter = nodeFilters.cend();
        for (auto itFilter = nodeFilters.cbegin(); itFilter != nodeFilters.cend(); ++itFilter)
        {
            if (itFilter->NodeNameFilter == castedAttributes.NodeName)
            {
                // Found the most specific filter, use it
                itNodeFilter = itFilter;
                break;
            }

            if (itFilter->NodeNameFilter.empty())
            {
                itNodeFilter = itFilter;
            }
        }

        FABRIC_HEALTH_STATE healthState;
        std::vector<HealthEvaluation> childUnhealthyEvaluations;
        error = node->EvaluateHealth(activityId, healthPolicy, /*out*/healthState, /*out*/childUnhealthyEvaluations);
        if (error.IsSuccess())
        {
            if (itNodeFilter != nodeFilters.cend() && itNodeFilter->IsRespected(healthState))
            {
                childrenHealthStates.Add(NodeHealthStateChunk(castedAttributes.NodeName, healthState));
                ++totalCount;
            }

            if (canImpactAggregatedHealth)
            {
                nodesHealthCount.AddResult(healthState);
            }
        }
        else if (!CanIgnoreChildEvaluationError(error))
        {
            return error;
        }
    }

    childrenHealthStates.TotalCount = totalCount;
    UpdateHealthState(
        activityId,
        HealthEntityKind::Node,
        nodesHealthCount,
        healthPolicy.MaxPercentUnhealthyNodes,
        aggregatedHealthState);

    return ErrorCode::Success();
}

ErrorCode ClusterEntity::EvaluateNodes(
    Common::ActivityId const & activityId,
    ServiceModel::ClusterHealthPolicy const & healthPolicy,
    ServiceModel::NodeHealthStatesFilterUPtr const & nodesFilter,
    HealthStatisticsUPtr const & healthStats,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations,
    __inout std::vector<NodeAggregatedHealthState> & childrenHealthStates)
{
    std::vector<HealthEntitySPtr> nodes;
    ErrorCode error = HealthManagerReplicaObj.EntityManager->Nodes.GetEntities(activityId, nodes);
    if (!error.IsSuccess()) { return error; }

    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);
    std::vector<HealthEvaluation> childUnhealthyEvaluationsList;

    HealthCount nodesHealthCount;
    HealthCount totalNodeCount;
    for (auto it = nodes.begin(); it != nodes.end(); ++it)
    {
        auto node = NodesCache::GetCastedEntityPtr(*it);
        auto nodeAttributes = node->GetAttributesCopy();
        auto & castedAttributes = NodeEntity::GetCastedAttributes(nodeAttributes);
        if (!castedAttributes.AttributeSetFlags.IsNodeNameSet())
        {
            HMEvents::Trace->QuerySkipNode(activityId, node->EntityIdString, castedAttributes);
            continue;
        }

        FABRIC_HEALTH_STATE healthState;
        std::vector<HealthEvaluation> childUnhealthyEvaluations;
        error = node->EvaluateHealth(activityId, healthPolicy, /*out*/healthState, /*out*/childUnhealthyEvaluations);
        if (error.IsSuccess())
        {
            totalNodeCount.AddResult(healthState);

            if (!nodesFilter || nodesFilter->IsRespected(healthState))
            {
                childrenHealthStates.push_back(NodeAggregatedHealthState(
                    castedAttributes.NodeName,
                    castedAttributes.EntityId,
                    healthState));
            }

            if (canImpactAggregatedHealth)
            {
                nodesHealthCount.AddResult(healthState);

                HealthEvaluationBaseSPtr childEvaluation;
                if (!childUnhealthyEvaluations.empty())
                {
                    childEvaluation = make_shared<NodeHealthEvaluation>(
                        castedAttributes.NodeName,
                        healthState,
                        move(childUnhealthyEvaluations));

                    childUnhealthyEvaluationsList.push_back(HealthEvaluation(childEvaluation));
                }
            }
        }
        else if (!CanIgnoreChildEvaluationError(error))
        {
            return error;
        }
    }

    if (UpdateHealthState(
        activityId,
        HealthEntityKind::Node,
        nodesHealthCount,
        healthPolicy.MaxPercentUnhealthyNodes,
        aggregatedHealthState))
    {
        unhealthyEvaluations.clear();
        unhealthyEvaluations.push_back(
            HealthEvaluation(make_shared<NodesHealthEvaluation>(
                aggregatedHealthState,
                HealthCount::FilterUnhealthy(childUnhealthyEvaluationsList, aggregatedHealthState),
                nodesHealthCount.TotalCount,
                healthPolicy.MaxPercentUnhealthyNodes)));
    }

    if (healthStats)
    {
        healthStats->Add(EntityKind::Node, totalNodeCount.GetHealthStateCount());
    }

    return ErrorCode::Success();
}

ErrorCode ClusterEntity::EvaluateApplicationsForClusterUpgrade(
    Common::ActivityId const & activityId,
    ServiceModel::ClusterHealthPolicy const & healthPolicy,
    ServiceModel::ApplicationHealthPolicyMap const & applicationHealthPolicies,
    std::vector<std::wstring> const & upgradeDomains,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations,
    __inout std::vector<std::wstring> & applicationsWithoutAppType)
{
    vector<HealthEntitySPtr> applications;
    ErrorCode error = HealthManagerReplicaObj.EntityManager->Applications.GetEntities(activityId, applications);
    if (!error.IsSuccess())
    {
        return error;
    }

    GroupHealthStateCount applicationsHealthCount(healthPolicy.MaxPercentUnhealthyApplications);
    std::vector<HealthEvaluation> systemAppUnhealthyChildren;
    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);

    std::map<wstring, GroupHealthStateCount> appsPerAppType;
    if (CommonConfig::GetConfig().EnableApplicationTypeHealthEvaluation)
    {
        for (auto const & entry : healthPolicy.ApplicationTypeMap)
        {
            appsPerAppType.insert(make_pair(entry.first, GroupHealthStateCount(entry.second)));
        }
    }

    for (auto it = applications.begin(); it != applications.end(); ++it)
    {
        FABRIC_HEALTH_STATE healthState = FABRIC_HEALTH_STATE_UNKNOWN;
        std::vector<HealthEvaluation> childUnhealthyEvaluations;

        auto application = ApplicationsCache::GetCastedEntityPtr(*it);
        wstring const & appName = application->EntityId.ApplicationName;

        // Check whether the app type name is set; if not, force CM to populate it, even if EnableApplicationTypeHealthEvaluation is disabled
        wstring appTypeName;
        if (!application->IsSystemApp && !application->IsAdHocApp())
        {
            auto appTypeAttributeError = application->GetApplicationTypeName(appTypeName);

            // Reply with error to let CM know that it needs to send the app types
            if (appTypeAttributeError.IsError(ErrorCodeValue::ApplicationTypeNotFound))
            {
                // Create a list of applications that are missing the app type to be passed to CM.
                applicationsWithoutAppType.push_back(appName);
                for (auto itNext = ++it; itNext != applications.end(); ++itNext)
                {
                    auto nextApplication = ApplicationsCache::GetCastedEntityPtr(*itNext);
                    if (nextApplication->NeedsApplicationTypeName())
                    {
                        applicationsWithoutAppType.push_back(nextApplication->EntityId.ApplicationName);
                    }
                }

                HealthManagerReplica::WriteInfo(TraceComponent, "{0}: found {1} applications with app type not set", activityId, applicationsWithoutAppType.size());
                return appTypeAttributeError;
            }
            else if (appTypeAttributeError.IsError(ErrorCodeValue::HealthEntityNotFound) ||
                     appTypeAttributeError.IsError(ErrorCodeValue::HealthEntityDeleted))
            {
                // Go to next application
                continue;
            }
        }

        // Get the application policy if specified in the app policy map.
        // If not specified, pass null and the evaluation will use the application policy specified in the manifest or the default one
        auto appHealthPolicy = applicationHealthPolicies.GetApplicationHealthPolicy(appName);
        error = application->EvaluateHealth(activityId, appHealthPolicy, upgradeDomains, nullptr, /*out*/healthState, /*out*/childUnhealthyEvaluations);
        if (error.IsSuccess())
        {
            if (application->IsSystemApp)
            {
                if (healthState != FABRIC_HEALTH_STATE_OK)
                {
                    HMEvents::Trace->SystemAppUnhealthy(activityId, wformatString(healthState));
                    if (aggregatedHealthState < healthState)
                    {
                        aggregatedHealthState = healthState;
                        systemAppUnhealthyChildren = move(childUnhealthyEvaluations);
                    }
                }
            }
            else if (canImpactAggregatedHealth)
            {
                HealthEvaluationBaseSPtr childEvaluation;
                if (!childUnhealthyEvaluations.empty())
                {
                    childEvaluation = make_shared<ApplicationHealthEvaluation>(appName, healthState, move(childUnhealthyEvaluations));
                }

                bool processed = false;
                if (!appTypeName.empty())
                {
                    // If the application is specified in the app map, add to map specific count
                    auto itAppTypeMap = appsPerAppType.find(appTypeName);
                    if (itAppTypeMap != appsPerAppType.end())
                    {
                        processed = true;
                        itAppTypeMap->second.Add(healthState, move(childEvaluation));
                    }
                }

                if (!processed)
                {
                    // Add to the global pool of applications, measured against MaxPercentUnhealthyApplications
                    applicationsHealthCount.Add(healthState, move(childEvaluation));
                }
            }
        }
        else if (application->IsSystemApp)
        {
            // fabric:/System is required to evaluate the cluster health
            HealthManagerReplica::WriteInfo(TraceComponent, "{0}: Error evaluating fabric:/System application: {1}", activityId, error);
            return error;
        }
        else if (!CanIgnoreChildEvaluationError(error))
        {
            HealthManagerReplica::WriteInfo(
                TraceComponent,
                "{0}: failed to evaluate health for application {1}: {2}",
                activityId,
                application->EntityIdString,
                error);
            return error;
        }
    }

    return ComputeApplicationsHealth(
        activityId,
        move(systemAppUnhealthyChildren),
        aggregatedHealthState,
        applicationsHealthCount,
        appsPerAppType,
        unhealthyEvaluations);
}

ErrorCode ClusterEntity::EvaluateApplicationChunks(
    Common::ActivityId const & activityId,
    ServiceModel::ClusterHealthPolicy const & healthPolicy,
    ServiceModel::ApplicationHealthPolicyMap const & applicationHealthPolicies,
    ServiceModel::ApplicationHealthStateFilterList const & applicationFilters,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ApplicationHealthStateChunkList & childrenHealthStates)
{
    vector<HealthEntitySPtr> applications;
    auto error = HealthManagerReplicaObj.EntityManager->Applications.GetEntities(activityId, applications);
    if (!error.IsSuccess())
    {
        return error;
    }

    GroupHealthStateCount applicationsHealthCount(healthPolicy.MaxPercentUnhealthyApplications);
    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);

    std::map<wstring, GroupHealthStateCount> appsPerAppType;
    if (CommonConfig::GetConfig().EnableApplicationTypeHealthEvaluation)
    {
        for (auto const & entry : healthPolicy.ApplicationTypeMap)
        {
            appsPerAppType.insert(make_pair(entry.first, GroupHealthStateCount(entry.second)));
        }
    }

    std::vector<std::wstring> upgradeDomains;
    ULONG totalCount = 0;
    for (auto it = applications.begin(); it != applications.end(); ++it)
    {
        FABRIC_HEALTH_STATE healthState = FABRIC_HEALTH_STATE_UNKNOWN;

        auto application = ApplicationsCache::GetCastedEntityPtr(*it);
        wstring const & appName = application->EntityId.ApplicationName;

        wstring appTypeName;
        auto appTypeError = application->GetApplicationTypeName(appTypeName);

        // Find the filter that applies to this application
        auto itApplicationFilter = applicationFilters.cend();
        for (auto itFilter = applicationFilters.cbegin(); itFilter != applicationFilters.cend(); ++itFilter)
        {
            if (itFilter->ApplicationNameFilter == application->EntityId.ApplicationName &&
                !application->EntityId.ApplicationName.empty())
            {
                // Found the most specific filter, use it
                itApplicationFilter = itFilter;
                break;
            }

            if (!itFilter->ApplicationTypeNameFilter.empty())
            {
                // The filter is for a specific app type, it only applies if the app type name is set and same.
                if (itFilter->ApplicationTypeNameFilter == appTypeName)
                {
                    // Remember the application type filter, but continue searching in case there are more specific filters (like app name)
                    itApplicationFilter = itFilter;
                }
            }
            else if (itApplicationFilter == applicationFilters.cend() && itFilter->ApplicationNameFilter.empty())
            {
                // No specific application or app type filters found, remember the default filter and continue search.
                itApplicationFilter = itFilter;
            }
        }

        auto appHealthPolicy = applicationHealthPolicies.GetApplicationHealthPolicy(appName);

        ServiceHealthStateChunkList services;
        DeployedApplicationHealthStateChunkList deployedApplications;
        if (itApplicationFilter != applicationFilters.cend())
        {
            error = application->EvaluateFilteredHealth(activityId, appHealthPolicy, upgradeDomains, itApplicationFilter->ServiceFilters, itApplicationFilter->DeployedApplicationFilters, /*out*/healthState, /*out*/services, /*out*/deployedApplications);
        }
        else
        {
            ServiceHealthStateFilterList serviceFilters;
            DeployedApplicationHealthStateFilterList deployedApplicationFilters;
            error = application->EvaluateFilteredHealth(activityId, appHealthPolicy, upgradeDomains, serviceFilters, deployedApplicationFilters, /*out*/healthState, /*out*/services, /*out*/deployedApplications);
        }

        if (error.IsSuccess())
        {
            if (itApplicationFilter != applicationFilters.cend() && itApplicationFilter->IsRespected(healthState))
            {
                childrenHealthStates.Add(ApplicationHealthStateChunk(appName, appTypeName, healthState, move(services), move(deployedApplications)));
                ++totalCount;
            }

            if (application->IsSystemApp)
            {
                if (healthState != FABRIC_HEALTH_STATE_OK)
                {
                    HMEvents::Trace->SystemAppUnhealthy(activityId, wformatString(healthState));
                    if (aggregatedHealthState < healthState)
                    {
                        aggregatedHealthState = healthState;
                    }
                }
            }
            else if (canImpactAggregatedHealth)
            {
                bool processed = false;
                if (!appsPerAppType.empty() && !appName.empty())
                {
                    if (appTypeError.IsSuccess() && !appTypeName.empty())
                    {
                        // If the application is specified in the app map, add to map specific count
                        auto itAppTypeMap = appsPerAppType.find(appTypeName);
                        if (itAppTypeMap != appsPerAppType.end())
                        {
                            processed = true;
                            itAppTypeMap->second.Add(healthState, nullptr);
                        }
                    }
                }

                if (!processed)
                {
                    // Add to the global pool of applications, measured against MaxPercentUnhealthyApplications
                    applicationsHealthCount.Add(healthState, nullptr);
                }
            }
        }
        else if (application->IsSystemApp)
        {
            // fabric:/System is required to evaluate the cluster health
            HealthManagerReplica::WriteInfo(TraceComponent, "{0}: Error evaluating fabric:/System application: {1}", activityId, error);
            return error;
        }
        else if (!CanIgnoreChildEvaluationError(error))
        {
            HealthManagerReplica::WriteInfo(
                TraceComponent,
                "{0}: failed to evaluate health for application {1}: {2}",
                activityId,
                application->EntityIdString,
                error);
            return error;
        }
    }

    childrenHealthStates.TotalCount = totalCount;

    // If not error, evaluate applications health count per global policy
    if (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
    {
        FABRIC_HEALTH_STATE applicationsHealth = applicationsHealthCount.GetHealthState();
        HMEvents::Trace->ChildrenState(
            this->EntityIdString,
            this->PartitionedReplicaId.TraceId,
            activityId,
            HealthEntityKind::Application,
            applicationsHealthCount.Count,
            wformatString(applicationsHealth),
            applicationsHealthCount.MaxPercentUnhealthy);

        if (aggregatedHealthState < applicationsHealth)
        {
            aggregatedHealthState = applicationsHealth;
        }
    }

    // If not error, and per app type policy is specified, evaluate applications health count per app type policy
    if (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
    {
        for (auto it = appsPerAppType.begin(); it != appsPerAppType.end(); ++it)
        {
            auto appTypeHealthState = it->second.GetHealthState();
            if (aggregatedHealthState < appTypeHealthState)
            {
                aggregatedHealthState = appTypeHealthState;
            }
        }
    }

    return ErrorCode::Success();
}

ErrorCode ClusterEntity::EvaluateApplications(
    Common::ActivityId const & activityId,
    ServiceModel::ClusterHealthPolicy const & healthPolicy,
    ServiceModel::ApplicationHealthPolicyMap const & applicationHealthPolicies,
    ServiceModel::ApplicationHealthStatesFilterUPtr const & applicationsFilter,
    HealthStatisticsUPtr const & healthStats,
    ServiceModel::ClusterHealthStatisticsFilterUPtr const & healthStatsFilter,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations,
    __inout std::vector<ApplicationAggregatedHealthState> & childrenHealthStates)
{
    vector<HealthEntitySPtr> applications;
    auto error = HealthManagerReplicaObj.EntityManager->Applications.GetEntities(activityId, applications);
    if (!error.IsSuccess())
    {
        return error;
    }

    HealthCount totalApplicationCount;
    GroupHealthStateCount applicationsHealthCount(healthPolicy.MaxPercentUnhealthyApplications);
    std::vector<HealthEvaluation> systemAppUnhealthyChildren;
    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);

    std::map<wstring, GroupHealthStateCount> appsPerAppType;
    if (CommonConfig::GetConfig().EnableApplicationTypeHealthEvaluation)
    {
        for (auto const & entry : healthPolicy.ApplicationTypeMap)
        {
            appsPerAppType.insert(make_pair(entry.first, GroupHealthStateCount(entry.second)));
        }
    }

    std::vector<std::wstring> upgradeDomains;
    for (auto it = applications.begin(); it != applications.end(); ++it)
    {
        FABRIC_HEALTH_STATE healthState = FABRIC_HEALTH_STATE_UNKNOWN;
        std::vector<HealthEvaluation> childUnhealthyEvaluations;

        auto application = ApplicationsCache::GetCastedEntityPtr(*it);
        wstring const & appName = application->EntityId.ApplicationName;

        shared_ptr<ApplicationHealthPolicy> appHealthPolicy = applicationHealthPolicies.GetApplicationHealthPolicy(appName);

        bool addStats = (healthStats != nullptr);
        if (application->IsSystemApp)
        {
            if (!healthStatsFilter || !healthStatsFilter->IncludeSystemApplicationHealthStatistics)
            {
                addStats = false;
            }
        }

        if (addStats)
        {
            error = application->EvaluateHealth(activityId, appHealthPolicy, upgradeDomains, healthStats, healthState, childUnhealthyEvaluations);
        }
        else
        {
            // Do not add health stats for this particular application
            error = application->EvaluateHealth(activityId, appHealthPolicy, upgradeDomains, nullptr, healthState, childUnhealthyEvaluations);
        }

        if (error.IsSuccess())
        {
            if (!applicationsFilter || applicationsFilter->IsRespected(healthState))
            {
                childrenHealthStates.push_back(ApplicationAggregatedHealthState(appName, healthState));
            }

            if (addStats)
            {
                totalApplicationCount.AddResult(healthState);
            }

            if (application->IsSystemApp)
            {
                if (healthState != FABRIC_HEALTH_STATE_OK)
                {
                    HMEvents::Trace->SystemAppUnhealthy(activityId, wformatString(healthState));
                    if (aggregatedHealthState < healthState)
                    {
                        aggregatedHealthState = healthState;
                        systemAppUnhealthyChildren = move(childUnhealthyEvaluations);
                    }
                }
            }
            else
            {
                if (canImpactAggregatedHealth)
                {
                    HealthEvaluationBaseSPtr childEvaluation;
                    if (!childUnhealthyEvaluations.empty())
                    {
                        childEvaluation = make_shared<ApplicationHealthEvaluation>(appName, healthState, move(childUnhealthyEvaluations));
                    }

                    bool processed = false;
                    if (!appsPerAppType.empty() && !appName.empty())
                    {
                        wstring appTypeName;
                        auto appTypeError = application->GetApplicationTypeName(appTypeName);
                        if (appTypeError.IsSuccess() && !appTypeName.empty())
                        {
                            // If the application is specified in the app map, add to map specific count
                            auto itAppTypeMap = appsPerAppType.find(appTypeName);
                            if (itAppTypeMap != appsPerAppType.end())
                            {
                                processed = true;
                                itAppTypeMap->second.Add(healthState, move(childEvaluation));
                            }
                        }
                    }

                    if (!processed)
                    {
                        // Add to the global pool of applications, measured against MaxPercentUnhealthyApplications
                        applicationsHealthCount.Add(healthState, move(childEvaluation));
                    }
                }
            }
        }
        else if (application->IsSystemApp)
        {
            // fabric:/System is required to evaluate the cluster health
            HealthManagerReplica::WriteInfo(TraceComponent, "{0}: Error evaluating fabric:/System application: {1}", activityId, error);
            return error;
        }
        else if (!CanIgnoreChildEvaluationError(error))
        {
            HealthManagerReplica::WriteInfo(
                TraceComponent,
                "{0}: failed to evaluate health for application {1}: {2}",
                activityId,
                application->EntityIdString,
                error);
            return error;
        }
    }

    if (healthStats)
    {
        healthStats->Add(EntityKind::Application, totalApplicationCount.GetHealthStateCount());
        if (totalApplicationCount.TotalCount == 0)
        {
            healthStats->Add(EntityKind::Service);
            healthStats->Add(EntityKind::Partition);
            healthStats->Add(EntityKind::Replica);
            healthStats->Add(EntityKind::DeployedApplication);
            healthStats->Add(EntityKind::DeployedServicePackage);
        }
    }

    return ComputeApplicationsHealth(
        activityId,
        move(systemAppUnhealthyChildren),
        aggregatedHealthState,
        applicationsHealthCount,
        appsPerAppType,
        unhealthyEvaluations);
}

Common::ErrorCode ClusterEntity::ComputeApplicationsHealth(
    Common::ActivityId const & activityId,
    std::vector<HealthEvaluation> && systemAppUnhealthyChildren,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __in GroupHealthStateCount & applicationsHealthCount,
    __in std::map<wstring, GroupHealthStateCount> & appsPerAppType,
    __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations)
{
    FABRIC_HEALTH_EVALUATION_KIND evaluationKind = FABRIC_HEALTH_EVALUATION_KIND_INVALID;
    if (!systemAppUnhealthyChildren.empty())
    {
        evaluationKind = FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION;
    }

    // If not error, evaluate applications health count per global policy
    if (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
    {
        FABRIC_HEALTH_STATE applicationsHealth = applicationsHealthCount.GetHealthState();
        HMEvents::Trace->ChildrenState(
            this->EntityIdString,
            this->PartitionedReplicaId.TraceId,
            activityId,
            HealthEntityKind::Application,
            applicationsHealthCount.Count,
            wformatString(applicationsHealth),
            applicationsHealthCount.MaxPercentUnhealthy);

        if (aggregatedHealthState < applicationsHealth)
        {
            aggregatedHealthState = applicationsHealth;
            evaluationKind = FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS;
        }
    }

    // If not error, and per app type policy is specified, evaluate applications health count per app type policy
    auto appTypeUnhealthyIt = appsPerAppType.end();
    if (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
    {
        for (auto it = appsPerAppType.begin(); it != appsPerAppType.end(); ++it)
        {
            auto appTypeHealthState = it->second.GetHealthState();
            if (aggregatedHealthState < appTypeHealthState)
            {
                aggregatedHealthState = appTypeHealthState;
                appTypeUnhealthyIt = it;
                evaluationKind = FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS;
            }
        }
    }

    if (evaluationKind == FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS)
    {
        // Create applications reason
        unhealthyEvaluations.clear();
        unhealthyEvaluations.push_back(
            HealthEvaluation(make_shared<ApplicationsHealthEvaluation>(
            aggregatedHealthState,
            applicationsHealthCount.GetUnhealthy(),
            applicationsHealthCount.Count.TotalCount,
            applicationsHealthCount.MaxPercentUnhealthy)));
    }
    else if (evaluationKind == FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS)
    {
        // Create applications per app type reason
        unhealthyEvaluations.clear();
        unhealthyEvaluations.push_back(
            HealthEvaluation(make_shared<ApplicationTypeApplicationsHealthEvaluation>(
            /*applicationTypeName*/ appTypeUnhealthyIt->first,
            aggregatedHealthState,
            appTypeUnhealthyIt->second.GetUnhealthy(),
            appTypeUnhealthyIt->second.Count.TotalCount,
            appTypeUnhealthyIt->second.MaxPercentUnhealthy)));
    }
    else if (evaluationKind == FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION)
    {
        // Create system application reason
        unhealthyEvaluations.clear();
        unhealthyEvaluations.push_back(
            HealthEvaluation(make_shared<SystemApplicationHealthEvaluation>(aggregatedHealthState, move(systemAppUnhealthyChildren))));
    }

    return ErrorCode::Success();
}

ErrorCode ClusterEntity::GetApplicationsAggregatedHealthStates(
    __inout QueryRequestContext & context)
{
    vector<HealthEntitySPtr> applications;
    ErrorCode error(ErrorCodeValue::Success);
    if (!context.ContinuationToken.empty())
    {
        error = HealthManagerReplicaObj.EntityManager->Applications.GetEntities(context.ActivityId, ApplicationHealthId(context.ContinuationToken), applications);
    }
    else
    {
        error = HealthManagerReplicaObj.EntityManager->Applications.GetEntities(context.ActivityId, applications);
    }

    if (!error.IsSuccess())
    {
        return error;
    }

    ListPager<ApplicationAggregatedHealthState> pager;
    pager.SetMaxResults(context.MaxResults);

    vector<wstring> upgradeDomains;
    for (auto it = applications.begin(); it != applications.end(); ++it)
    {
        auto application = ApplicationsCache::GetCastedEntityPtr(*it);

        if (!application->Match(context.Filters))
        {
            continue;
        }

        if (application->IsSystemApp || application->IsAdHocApp())
        {
            // The get applications query does not need to return these items
            // If returned, they mess up the maxResults count
            continue;
        }

        FABRIC_HEALTH_STATE applicationHealthState = FABRIC_HEALTH_STATE_UNKNOWN;
        vector<HealthEvaluation> unhealthyEvaluations;
        error = application->EvaluateHealth(context.ActivityId, context.ApplicationPolicy, upgradeDomains, nullptr, /*out*/applicationHealthState, /*out*/unhealthyEvaluations);

        if (error.IsSuccess())
        {
            error = pager.TryAdd(ApplicationAggregatedHealthState(application->EntityId.ApplicationName, applicationHealthState));
            bool benignError;
            bool hasError;
            CheckListPagerErrorAndTrace(error, this->EntityIdString, context.ActivityId, application->EntityId.ApplicationName, hasError, benignError);
            if (hasError && benignError)
            {
                break;
            }
            else if (hasError && !benignError)
            {
                return error;
            }
        }
        else
        {
            HMEvents::Trace->InternalMethodFailed(
                this->EntityIdString,
                context.ActivityId,
                L"EvaluateHealth",
                error,
                error.Message);
        }
    }

    context.AddQueryListResults<ApplicationAggregatedHealthState>(move(pager));
    return ErrorCode::Success();
}

ErrorCode ClusterEntity::GetApplicationUnhealthyEvaluations(
    QueryRequestContext & context)
{
    vector<HealthEntitySPtr> applications;
    ErrorCode error;
    if (!context.ContinuationToken.empty())
    {
        error = HealthManagerReplicaObj.EntityManager->Applications.GetEntities(context.ActivityId, ApplicationHealthId(context.ContinuationToken), applications);
    }
    else
    {
        error = HealthManagerReplicaObj.EntityManager->Applications.GetEntities(context.ActivityId, applications);
    }

    if (!error.IsSuccess())
    {
        return error;
    }

    ListPager<ApplicationUnhealthyEvaluation> pager;
    pager.SetMaxResults(context.MaxResults);

    vector<wstring> upgradeDomains;
    for (HealthEntitySPtr const & obj : applications)
    {
        auto application = ApplicationsCache::GetCastedEntityPtr(obj);
        if (!application->Match(context.Filters))
        {
            continue;
        }

        if (application->IsSystemApp || application->IsAdHocApp())
        {
            continue;
        }

        FABRIC_HEALTH_STATE healthState = FABRIC_HEALTH_STATE_UNKNOWN;
        vector<HealthEvaluation> unhealthyEvaluations;
        error = application->EvaluateHealth(
            context.ActivityId,
            context.ApplicationPolicy,
            upgradeDomains,
            nullptr,
            /*out*/healthState,
            /*out*/unhealthyEvaluations,
            true); // isDeployedChildrenPrecedent

        if (error.IsSuccess())
        {
            vector<HealthEvent> tempEvent;
            EntityHealthBase healthTemp(
                move(tempEvent),
                healthState,
                move(unhealthyEvaluations));

            error = healthTemp.UpdateUnhealthyEvalautionsPerMaxAllowedSize(QueryRequestContext::GetMaxAllowedSize());
            if (!error.IsSuccess())
            {
                return error;
            }

            error = pager.TryAdd(ApplicationUnhealthyEvaluation(application->EntityId.ApplicationName, healthState, healthTemp.TakeUnhealthyEvaluations()));
            bool benignError, hasError;
            CheckListPagerErrorAndTrace(error, this->EntityIdString, context.ActivityId, application->EntityId.ApplicationName, hasError, benignError);
            if (hasError && benignError)
            {
                break;
            }
            else if (hasError && !benignError)
            {
                return error;
            }
        }
        else
        {
            HMEvents::Trace->InternalMethodFailed(
                this->EntityIdString,
                context.ActivityId,
                L"EvaluateHealth",
                error,
                error.Message);
        }
    }

    context.AddQueryListResults<ApplicationUnhealthyEvaluation>(move(pager));
    return ErrorCode::Success();
}

ErrorCode ClusterEntity::GetDeployedApplicationsOnNodeAggregatedHealthStates(
    __inout QueryRequestContext & context)
{
    // In ProcessInnerQueryRequestAsyncOperation, we've already checked that nodeName is provided for context.Filters.

    vector<HealthEntitySPtr> applications;
    ErrorCode error(ErrorCodeValue::Success);

    // If the user has specified an application name, go directly to that application
    auto it = context.Filters.find(HealthAttributeNames::ApplicationName);
    if (it != context.Filters.end())
    {
        // Not adding a move here because if anyone tries to use it and doesn't see this code, it'll be very error prone
        wstring applicationName = it->second;
        auto healthId = ApplicationHealthId(move(applicationName));

        // This jumps directly to which application we want.
        error = HealthManagerReplicaObj.EntityManager->Applications.GetEntitiesByName(context.ActivityId, &healthId, applications);
    }
    else // Find all applications
    {
        // Get all the applications respecting continuation token, but not page size
        if (!context.ContinuationToken.empty())
        {
            error = HealthManagerReplicaObj.EntityManager->Applications.GetEntities(context.ActivityId, ApplicationHealthId(context.ContinuationToken), applications);
        }
        else
        {
            error = HealthManagerReplicaObj.EntityManager->Applications.GetEntities(context.ActivityId, applications);
        }

        // Note: vector<HealthEntitySPtr> applications is sorted by application name
        // This is because GetEntities is reading in and adding items from entities_ which is essentially a map object,
        // and maps are sorted objects. They are sorted by the key of the map, which for application entities should be the application name,
        // which is what we've provided as a continuation token for this query.
    }

    if (!error.IsSuccess())
    {
        return error;
    }

    // We've finished using the continuation token, so remove it from context.
    // If we don't remove it, and then call get deployed applications by an application name, then the expected continuation token for that portion of the query
    // becomes the node id instead of the application name.
    context.ClearContinuationToken();

    ListPager<DeployedApplicationAggregatedHealthState> pager;
    pager.SetMaxResults(context.MaxResults);
    vector<DeployedApplicationAggregatedHealthState> unpagedList;
    // Go through each application and call application entity to retrieve deployed application
    for (auto & app : applications)
    {
        auto application = ApplicationsCache::GetCastedEntityPtr(app);

        // The results from ProcessQuery are stored in context.
        application->ProcessQuery(context);

        // In ProcessQuery, context.QueryResult will be updated with the returned result of only what it finds.
        // Take that data and try to add it to our own ListPager
        // We create a new object here every time to ensure that we are not overriding an error code object.
        // Otherwise, if there is no list, then we never read the error code value (MoveList reads the error message),
        // and in such a case, if we define queryResult before the for loop starts, on the for loop iteration after the iteration with
        // no list, we will trigger an assert from overriding the ErrorCode object stored inside the query result.
        QueryResult queryResult = context.MoveQueryResult();

        // If there is no returned result, then we don't need to do anything
        if (queryResult.HasList())
        {
            // MoveList fails is there is no list.
            error = queryResult.MoveList(unpagedList);
            if (!error.IsSuccess())
            {
                return error;
            }

            if (unpagedList.size() == 1)
            {
                if (application->IsSystemApp && context.MaxResults != std::numeric_limits<int64>::max())
                {
                    // We add one here to accomodate the system application
                    pager.SetMaxResults(context.MaxResults + 1);
                }

                // Creates a copy here as this string will be consumed later.
                auto continuationToken = unpagedList[0].ApplicationName;
                error = pager.TryAdd(move(unpagedList[0]));
                if (!error.IsSuccess())
                {
                    pager.TraceErrorFromTryAdd(error, TraceComponent, this->PartitionedReplicaId.TraceId, L"GetDeployedApplicationsOnNode");

                    if (!pager.IsBenignError(error))
                    {
                        return error;
                    }

                    // We need to update the continuation token of the pager manaually because it is using the continuation token
                    // returned by DeployedApplicationAggregatedHealthState per application instance, which is the node name.
                    // For our purposes we need it to be the application name.
                    pager.OverrideContinuationTokenManual(continuationToken);

                    break;
                }
            }
            else if (unpagedList.size() != 0)
            {
                HealthManagerReplica::WriteError(
                    TraceComponent,
                    "GetDeployedApplicationsOnNodeAggregatedHealthStates returned more than one application deployed on a node. Returned {0}",
                    unpagedList);

                // This message isn't too helpful, but if this part of the code was hit, then something weird went wrong.
                // For this particular query, because a node should always be provided, we expect this to return at most one result.
                return ErrorCode(
                    ErrorCodeValue::InvalidOperation,
                    wformatString("{0} {1}", StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_OPERATION)));
            }
        }
    }

    context.AddQueryListResults<DeployedApplicationAggregatedHealthState>(move(pager));
    return ErrorCode::Success();
}

bool ClusterEntity::CleanupChildren()
{
    // Cluster entity should never be cleaned up
    return false;
}

Common::ErrorCode ClusterEntity::ComputeClusterHealthWithoutEvents(
    bool canCache,
    Common::ActivityId const & activityId,
    ServiceModel::ClusterHealthPolicy const & healthPolicy,
    ServiceModel::ApplicationHealthPolicyMap const & applicationHealthPolicies,
    ServiceModel::NodeHealthStatesFilterUPtr const & nodesFilter,
    ServiceModel::ApplicationHealthStatesFilterUPtr const & applicationsFilter,
    ClusterHealthStatisticsFilterUPtr const & healthStatsFilter,
    __inout ServiceModel::NodeAggregatedHealthStateList & nodeHealthStates,
    __inout ServiceModel::ApplicationAggregatedHealthStateList & applicationHealthStates,
    __inout CachedClusterHealthSPtr & partialClusterHealth,
    __inout bool & cached,
    __inout Common::TimeSpan & elapsed)
{
    HealthStatisticsUPtr healthStats;
    if (!healthStatsFilter || !healthStatsFilter->ExcludeHealthStatistics)
    {
        // Include healthStats
        healthStats = make_unique<HealthStatistics>();
    }

    int64 evaluationStartTime = Stopwatch::GetTimestamp();
    Stopwatch watch;
    watch.Start();

    //
    // Evaluate children only:
    // If system application is ERROR, return ERROR due to system app
    // If nodes or applications are at ERROR, return ERROR
    // If nodes or applications are at WARNING (at least one event/child unhealthy) and WARNING is the max health, return WARNING
    //
    FABRIC_HEALTH_STATE aggregatedHealthState = FABRIC_HEALTH_STATE_OK;
    HealthEvaluationList unhealthyEvaluations;
    auto error = EvaluateNodes(activityId, healthPolicy, nodesFilter, healthStats, aggregatedHealthState, unhealthyEvaluations, nodeHealthStates);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = EvaluateApplications(activityId, healthPolicy, applicationHealthPolicies, applicationsFilter, healthStats, healthStatsFilter, aggregatedHealthState, unhealthyEvaluations, applicationHealthStates);
    if (!error.IsSuccess())
    {
        return error;
    }

    watch.Stop();
    this->HealthManagerReplicaObj.HealthManagerCounters->OnEvaluateClusterHealthCompleted(evaluationStartTime);

    elapsed = watch.Elapsed;

    cached = false;
    partialClusterHealth = make_shared<CachedClusterHealth>();
    partialClusterHealth->AggregatedHealthState = aggregatedHealthState;
    partialClusterHealth->HealthStats = move(healthStats);
    partialClusterHealth->UnhealthyEvaluation = move(unhealthyEvaluations);

    if (canCache)
    {
        // StatisticsDurationOffset is used for testing caching - introduce artificial delay in tests to hit this code path.
        auto elapsedWithOffset = elapsed + ManagementConfig::GetConfig().StatisticsDurationOffset;
        if (elapsedWithOffset > ManagementConfig::GetConfig().MaxStatisticsDurationBeforeCaching)
        {
            // Query took a long time, so start the caching timer.
            TimeSpan timerInterval = TimeSpan::FromTicks(elapsedWithOffset.Ticks * 2);
            TimeSpan maxInterval = ManagementConfig::GetConfig().MaxCacheStatisticsTimerInterval;
            if (timerInterval > maxInterval)
            {
                timerInterval = maxInterval;
            }

            HMEvents::Trace->CacheSlowStats(
                this->EntityIdString,
                activityId,
                elapsedWithOffset,
                wformatString(partialClusterHealth->HealthStats));

            cached = true;

            { // lock
                AcquireWriteLock lock(statsLock_);
                cachedClusterHealth_ = partialClusterHealth;
                if (this->State.IsClosed)
                {
                    return ErrorCode(ErrorCodeValue::InvalidState);
                }

                // Set the timer to refresh in the background
                if (!statsTimer_)
                {
                    auto root = this->shared_from_this();
                    statsTimer_ = Timer::Create(
                        HealthStatsTimerTag,
                        [this, root](TimerSPtr const &) { this->HealthStatsTimerCallback(); },
                        true);
                }

                // Schedule timer to only fire once. The timer will be rescheduled if needed
                // when the new query estimation happens.
                statsTimer_->Change(timerInterval);
            } // endlock

            useCachedStats_.store(true);
        }
        else if (DisableCachedEntityStats())
        {
            // Clear the cached values, if any, since they are old.
            // Do not re-schedule timer, it will be set on next incoming query.
            // The errors are usually not expected and can happen mostly if not primary anymore
            // Since this is not a common scenario, do not try to re-schedule the evaluation as a query optimization.
            HMEvents::Trace->DisableCacheStatsQuick(
                this->EntityIdString,
                activityId,
                elapsedWithOffset);
        }
    }

    return ErrorCode::Success();
}

Common::ErrorCode ClusterEntity::ComputeDetailQueryResult(
    __inout QueryRequestContext & context,
    FABRIC_HEALTH_STATE entityEventsState,
    std::vector<ServiceModel::HealthEvent> && queryEvents,
    ServiceModel::HealthEvaluationList && eventsUnhealthyEvaluations)
{
    ErrorCode error(ErrorCodeValue::Success);

    unique_ptr<ApplicationHealthStatesFilter> applicationsFilter;
    error = context.GetApplicationsFilter(applicationsFilter);
    if (!error.IsSuccess()) { return error; }

    unique_ptr<NodeHealthStatesFilter> nodesFilter;
    error = context.GetNodesFilter(nodesFilter);
    if (!error.IsSuccess()) { return error; }

    ClusterHealthStatisticsFilterUPtr healthStatsFilter;
    error = context.GetClusterHealthStatsFilter(healthStatsFilter);
    if (!error.IsSuccess()) { return error; }

    // CanCacheResult was updated inside the context
    // and is only true for queries that match SFX usage:
    // no policies, include health stats with exclude system stats.
    // When caching happens, the nodes and apps health state may not be included (depending on filters).
    // To be able to use the cached result, the query must not ask for any nodes and applications.
    CachedClusterHealthSPtr cachedClusterHealth;
    if (context.CanCacheResult &&
       (applicationsFilter && (applicationsFilter->HealthStateFilter == static_cast<DWORD>(FABRIC_HEALTH_STATE_FILTER_NONE))) &&
       (nodesFilter && (nodesFilter->HealthStateFilter == static_cast<DWORD>(FABRIC_HEALTH_STATE_FILTER_NONE))))
    {
        // Check if we should use cached stats outside the lock.
        // The first queries will not use the cached stats, no matter how long it takes.
        // Only after one query completes and takes a long time the results are cached.
        // The code is optimized for queries that take a short time, in which case we want to
        // detect the changes quicker rather than use cached values.
        if (useCachedStats_.load())
        {
            { // lock
                AcquireReadLock lock(statsLock_);
                cachedClusterHealth = cachedClusterHealth_;
            } // endlock
        }
    }

    FABRIC_HEALTH_STATE aggregatedHealthState = entityEventsState;
    HealthEvaluationList unhealthyEvaluations = move(eventsUnhealthyEvaluations);

    std::vector<NodeAggregatedHealthState> nodeHealthStates;
    std::vector<ApplicationAggregatedHealthState> applicationHealthStates;
    bool cached;
    TimeSpan elapsed = TimeSpan::Zero;
    if (cachedClusterHealth)
    {
        cached = true;
    }
    else
    {
        error = ComputeClusterHealthWithoutEvents(
            context.CanCacheResult,
            context.ActivityId,
            *context.ClusterPolicy,
            *context.ApplicationHealthPolicies,
            nodesFilter,
            applicationsFilter,
            healthStatsFilter,
            nodeHealthStates,
            applicationHealthStates,
            cachedClusterHealth,
            cached,
            elapsed);
        if (!error.IsSuccess()) { return error; }
    }

    // Update based on the children aggregated health state
    bool updateUnhealthyEvaluations = false;
    if (aggregatedHealthState < cachedClusterHealth->AggregatedHealthState)
    {
        aggregatedHealthState = cachedClusterHealth->AggregatedHealthState;
        updateUnhealthyEvaluations = true;
    }

    HealthStatisticsUPtr healthStats;
    if (cached)
    {
        ASSERT_IFNOT(cachedClusterHealth->HealthStats, "{0}: got CachedClusterHealth with null health stats", context.ActivityId);

        // Copy the health stats and unhealthy evaluations, so they are left in the cached entry
        healthStats = make_unique<HealthStatistics>(*cachedClusterHealth->HealthStats);
        if (updateUnhealthyEvaluations)
        {
            unhealthyEvaluations = cachedClusterHealth->UnhealthyEvaluation;
        }
    }
    else
    {
        // Move the health stats, they are not needed for caching
        healthStats = move(cachedClusterHealth->HealthStats);
        if (updateUnhealthyEvaluations)
        {
            unhealthyEvaluations = move(cachedClusterHealth->UnhealthyEvaluation);
        }
    }

    auto health = make_unique<ClusterHealth>(
        aggregatedHealthState,
        move(unhealthyEvaluations),
        move(nodeHealthStates),
        move(applicationHealthStates),
        move(queryEvents),
        move(healthStats));
    error = health->UpdateUnhealthyEvalautionsPerMaxAllowedSize(QueryRequestContext::GetMaxAllowedSize());
    if (!error.IsSuccess())
    {
        return error;
    }

    if (cached)
    {
        HMEvents::Trace->Cluster_QueryCompletedCached(
            this->EntityIdString,
            context,
            *health,
            context.GetFilterDescription());
    }
    else
    {
        HMEvents::Trace->Cluster_QueryCompleted(
            this->EntityIdString,
            context,
            *health,
            context.GetFilterDescription(),
            elapsed);
    }

    context.SetQueryResult(QueryResult(std::move(health)));
    return ErrorCode::Success();
}

Common::ErrorCode ClusterEntity::ComputeQueryEntityHealthStateChunkResult(
    __inout QueryRequestContext & context,
    FABRIC_HEALTH_STATE entityEventsState)
{
    ErrorCode error(ErrorCodeValue::Success);
    ApplicationHealthStateFilterListWrapper applicationFilters;
    error = context.GetApplicationsFilter(applicationFilters);
    if (!error.IsSuccess())
    {
        return error;
    }

    NodeHealthStateFilterListWrapper nodeFilters;
    error = context.GetNodesFilter(nodeFilters);
    if (!error.IsSuccess())
    {
        return error;
    }


    auto aggregatedHealthState = entityEventsState;
    NodeHealthStateChunkList nodeHealthStates;
    ApplicationHealthStateChunkList applicationHealthStates;

    /* Summary:
    * If events are at ERROR, return ERROR due to events
    * If system application is ERROR, return ERROR due to system app
    * If nodes or applications are at ERROR, return ERROR
    * If events, nodes or applications are at WARNING (at least one event/child unhealthy) and WARNING is the max health, return WARNING
    */

    error = EvaluateNodeChunks(context.ActivityId, *context.ClusterPolicy, nodeFilters.Items, aggregatedHealthState, nodeHealthStates);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = EvaluateApplicationChunks(context.ActivityId, *context.ClusterPolicy, *context.ApplicationHealthPolicies, applicationFilters.Items, aggregatedHealthState, applicationHealthStates);
    if (!error.IsSuccess())
    {
        return error;
    }

    HMEvents::Trace->Cluster_ChunkQueryCompleted(
        EntityIdString,
        context,
        wformatString(aggregatedHealthState),
        static_cast<uint64>(applicationHealthStates.Count),
        static_cast<uint64>(nodeHealthStates.Count),
        context.GetFilterDescription());

    auto chunk = make_unique<ClusterHealthChunk>(
        aggregatedHealthState,
        move(nodeHealthStates),
        move(applicationHealthStates));

    size_t estimatedSize = chunk->EstimateSize();
    size_t maxAllowedSize = QueryRequestContext::GetMaxAllowedSize();
    if (estimatedSize >= maxAllowedSize)
    {
        return ErrorCode(
            ErrorCodeValue::EntryTooLarge,
            wformatString("{0} {1} {2}", Resources::GetResources().EntryTooLarge, estimatedSize, maxAllowedSize));
    }

    context.SetQueryResult(ServiceModel::QueryResult(std::move(chunk)));
    return ErrorCode::Success();
}

Common::ErrorCode ClusterEntity::SetDetailQueryResult(
    __inout QueryRequestContext & context,
    FABRIC_HEALTH_STATE entityEventsState,
    std::vector<ServiceModel::HealthEvent> && queryEvents,
    ServiceModel::HealthEvaluationList && unhealthyEvaluations)
{
    ASSERT_IFNOT(context.ClusterPolicy, "{0}: health policy not set", context);
    ASSERT_IFNOT(context.ApplicationHealthPolicies, "{0}: application health policies not set", context);

    switch (context.ContextKind)
    {
    case RequestContextKind::QueryEntityDetail:
        return ComputeDetailQueryResult(context, entityEventsState, move(queryEvents), move(unhealthyEvaluations));
    case RequestContextKind::QueryEntityHealthStateChunk:
        return ComputeQueryEntityHealthStateChunkResult(context, entityEventsState);
    default:
        Assert::CodingError("{0}: context kind {1} not supported", context, context.ContextKind);
    }
}

Common::ErrorCode ClusterEntity::SetChildrenAggregatedHealthQueryResult(
    __inout QueryRequestContext & context)
{
    switch (context.ChildrenKind)
    {
        case HealthEntityKind::Node:
            return GetNodesAggregatedHealthStates(context);
        case HealthEntityKind::Application:
            if (context.ContextKind == RequestContextKind::QueryEntityUnhealthyChildren)
            {
                return GetApplicationUnhealthyEvaluations(context);
            }
            else
            {
                return GetApplicationsAggregatedHealthStates(context);
            }
        case HealthEntityKind::DeployedApplication:
            // Only this particular query (GetDeployedApplicationsOnNode) should reach here.
            // This is because getting DeployedApplication by node name doesn't follow the hierarchy.
            return GetDeployedApplicationsOnNodeAggregatedHealthStates(context);

        default:
            Assert::CodingError(
                "{0}: {1}: SetChildrenAggregatedHealthQueryResult: can't return children of type {2}",
                this->PartitionedReplicaId.TraceId,
                this->EntityIdString,
                context.ChildrenKind);
    }
}

Common::ErrorCode ClusterEntity::Close()
{
    auto error = __super::Close();
    if (!error.IsSuccess()) { return error; }

    { // lock
        AcquireWriteLock lock(statsLock_);
        if (statsTimer_)
        {
            HealthManagerReplica::WriteInfo(TraceComponent, "Close: reset statsTimer.");
            statsTimer_->Cancel();
            statsTimer_.reset();
        }
    } // endlock

    return ErrorCode::Success();
}

bool ClusterEntity::DisableCachedEntityStats()
{
    // If the cached was used previously, clear it.
    bool expected = true;
    if (useCachedStats_.compare_exchange_weak(expected, false))
    {
        AcquireWriteLock lock(statsLock_);
        cachedClusterHealth_.reset();
        return true;
    }

    return false;
}

void ClusterEntity::HealthStatsTimerCallback()
{
    ActivityId activityId;

    auto nodesFilter = make_unique<NodeHealthStatesFilter>(FABRIC_HEALTH_STATE_FILTER_NONE);
    auto applicationsFilter = make_unique<ApplicationHealthStatesFilter>(FABRIC_HEALTH_STATE_FILTER_NONE);
    auto healthStatsFilter = make_unique<ClusterHealthStatisticsFilter>(false, false);

    ClusterHealthPolicySPtr healthPolicy;
    ApplicationHealthPolicyMapSPtr applicationHealthPolicies;
    auto error = GetClusterAndApplicationHealthPoliciesIfNotSet(healthPolicy, applicationHealthPolicies);
    if (error.IsSuccess())
    {
        std::vector<NodeAggregatedHealthState> nodeHealthStates;
        std::vector<ApplicationAggregatedHealthState> applicationHealthStates;
        CachedClusterHealthSPtr cachedClusterHealth;
        bool cached;
        TimeSpan elapsed;
        error = ComputeClusterHealthWithoutEvents(
            true /*canCache*/,
            activityId,
            *healthPolicy,
            *applicationHealthPolicies,
            nodesFilter,
            applicationsFilter,
            healthStatsFilter,
            nodeHealthStates,
            applicationHealthStates,
            cachedClusterHealth,
            cached,
            elapsed);
    }

    if (!error.IsSuccess())
    {
        if (DisableCachedEntityStats())
        {
            HMEvents::Trace->DisableCacheStatsOnError(this->EntityIdString, activityId, error);
        }
    }
}

bool ClusterEntity::CanIgnoreChildEvaluationError(Common::ErrorCode const & error)
{
    return error.IsError(ErrorCodeValue::HealthEntityNotFound) || error.IsError(ErrorCodeValue::InvalidState);
}
