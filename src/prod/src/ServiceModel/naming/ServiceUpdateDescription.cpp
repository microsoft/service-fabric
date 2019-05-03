// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Reliability;
using namespace Naming;

StringLiteral const TraceComponent("KeyValueStoreReplica");    

ServiceUpdateDescription::ServiceUpdateDescription()
    : updateFlags_(Flags::None)
    , serviceKind_(FABRIC_SERVICE_KIND::FABRIC_SERVICE_KIND_INVALID)
    , targetReplicaSetSize_(0)
    , minReplicaSetSize_(0)
    , replicaRestartWaitDuration_(TimeSpan::MinValue)
    , quorumLossWaitDuration_(TimeSpan::MinValue)
    , standByReplicaKeepDuration_(TimeSpan::MinValue)
    , serviceCorrelations_()
    , placementConstraints_()
    , metrics_()
    , placementPolicies_()
    , defaultMoveCost_(FABRIC_MOVE_COST_LOW)
    , repartitionDescription_()
    , scalingPolicies_()
    , initializationData_()
{
}

ServiceUpdateDescription::ServiceUpdateDescription(
    bool isStateful,
    std::vector<std::wstring> && namesToAdd,
    std::vector<std::wstring> && namesToRemove)
    : ServiceUpdateDescription()
{
    // Initialize all to default, and only set repartition information
    if (isStateful)
    {
        serviceKind_ = FABRIC_SERVICE_KIND::FABRIC_SERVICE_KIND_STATEFUL;
    }
    else
    {
        serviceKind_ = FABRIC_SERVICE_KIND::FABRIC_SERVICE_KIND_STATELESS;
    }
    repartitionDescription_ = make_shared<Naming::NamedRepartitionDescription>(move(namesToAdd), move(namesToRemove));
}

ErrorCode ServiceUpdateDescription::FromPublicApi(FABRIC_SERVICE_UPDATE_DESCRIPTION const & updateDescription)
{
    switch (updateDescription.Kind)
    {
    case FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS:
    {
        if (updateDescription.Value == nullptr) { return ErrorCodeValue::ArgumentNull; }

        auto * stateless = reinterpret_cast<FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION*>(updateDescription.Value);

        auto flags = stateless->Flags;

        auto validFlags = FABRIC_STATELESS_SERVICE_INSTANCE_COUNT |
            FABRIC_STATELESS_SERVICE_PLACEMENT_CONSTRAINTS |
            FABRIC_STATELESS_SERVICE_POLICY_LIST |
            FABRIC_STATELESS_SERVICE_CORRELATIONS |
            FABRIC_STATELESS_SERVICE_METRICS |
            FABRIC_STATELESS_SERVICE_MOVE_COST |
            FABRIC_STATELESS_SERVICE_SCALING_POLICY;

        if ((flags & ~validFlags) != 0) 
        { 
            return TraceAndGetInvalidArgumentError(wformatString(GET_NS_RC( Invalid_Flags ), flags));
        }
        
        serviceKind_ = FABRIC_SERVICE_KIND::FABRIC_SERVICE_KIND_STATELESS;

        if ((flags & FABRIC_STATELESS_SERVICE_INSTANCE_COUNT) != 0)
        {
            updateFlags_ |= Flags::TargetReplicaSetSize;
            targetReplicaSetSize_ = stateless->InstanceCount;
        }

        if (stateless->Reserved == nullptr) { break; }

        auto * statelessEx1 = reinterpret_cast<FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_EX1*>(stateless->Reserved);

        if ((flags & FABRIC_STATELESS_SERVICE_PLACEMENT_CONSTRAINTS) != 0)
        {
            updateFlags_ |= Flags::PlacementConstraints;
            HRESULT hr = StringUtility::LpcwstrToWstring(statelessEx1->PlacementConstraints, true /*acceptNull*/, placementConstraints_);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
        }
  
        if ((flags & FABRIC_STATELESS_SERVICE_CORRELATIONS) != 0)
        {
            updateFlags_ |= Flags::Correlations;
            for (ULONG i = 0; i < statelessEx1->CorrelationCount; i++)
            {
                wstring correlationServiceName;
                HRESULT hr = StringUtility::LpcwstrToWstring(statelessEx1->Correlations[i].ServiceName, true /*acceptNull*/, correlationServiceName);
                if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

                Reliability::ServiceCorrelationDescription correlationDescription(correlationServiceName, statelessEx1->Correlations[i].Scheme);

                serviceCorrelations_.push_back(correlationDescription);
            }
        }

        if ((flags & FABRIC_STATELESS_SERVICE_METRICS) != 0)
        {
            updateFlags_ |= Flags::Metrics;
            for (ULONG i = 0; i < statelessEx1->MetricCount; i++)
            {
                wstring metricsName;
                HRESULT hr = StringUtility::LpcwstrToWstring(statelessEx1->Metrics[i].Name, true /*acceptNull*/, metricsName);
                if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
                metrics_.push_back(Reliability::ServiceLoadMetricDescription(
                    move(metricsName),
                    statelessEx1->Metrics[i].Weight,
                    statelessEx1->Metrics[i].PrimaryDefaultLoad,
                    statelessEx1->Metrics[i].SecondaryDefaultLoad));
            }
        }

        if ((flags & FABRIC_STATELESS_SERVICE_POLICY_LIST) != 0)
        {
            updateFlags_ |= Flags::PlacementPolicyList;
            auto pList = reinterpret_cast<FABRIC_SERVICE_PLACEMENT_POLICY_LIST*>(statelessEx1->PolicyList);

            for (ULONG i = 0; i < pList->PolicyCount; i++)
            {
                std::wstring domainName;
                FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION & policyDesc = pList->Policies[i];
                ServicePlacementPolicyHelper::PolicyDescriptionToDomainName(policyDesc, domainName);
                placementPolicies_.push_back(ServiceModel::ServicePlacementPolicyDescription(move(domainName), policyDesc.Type));
            }
        }

        if (statelessEx1->Reserved == nullptr) { break; }

        auto * statelessEx2 = reinterpret_cast<FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_EX2*>(statelessEx1->Reserved);

        if ((flags & FABRIC_STATELESS_SERVICE_MOVE_COST) != 0)
        {
            updateFlags_ |= Flags::DefaultMoveCost;
            defaultMoveCost_ = statelessEx2->DefaultMoveCost;
        }

        if (statelessEx2->Reserved == nullptr) { break; }

        auto * statelessEx3 = reinterpret_cast<FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_EX3*>(statelessEx2->Reserved);

        auto error = this->FromRepartitionDescription(statelessEx3->RepartitionKind, statelessEx3->RepartitionDescription);
        if (!error.IsSuccess()) { return error; }

        if ((flags & FABRIC_STATELESS_SERVICE_SCALING_POLICY) != 0)
        {
            updateFlags_ |= Flags::ScalingPolicy;

            if (statelessEx3->ScalingPolicyCount > 1)
            {
                // Currently, only one scaling policy is allowed per service.
                // Vector is there for future uses (when services could have multiple scaling policies).
                return ErrorCode(ErrorCodeValue::InvalidServiceScalingPolicy, wformatString(GET_NS_RC(ScalingPolicy_Scaling_Count), statelessEx3->ScalingPolicyCount));
            }
            if (statelessEx3->ScalingPolicyCount > 0)
            {
                for (ULONG i = 0; i < statelessEx3->ScalingPolicyCount; i++)
                {
                    Reliability::ServiceScalingPolicyDescription scalingDescription;
                    auto scalingError = scalingDescription.FromPublicApi(statelessEx3->ServiceScalingPolicies[i]);
                    if (!scalingError.IsSuccess())
                    {
                        return scalingError;
                    }
                    scalingPolicies_.push_back(move(scalingDescription));
                }
            }
        }

        break;

    } // case stateless

    case FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL:
    {
        if (updateDescription.Value == nullptr) { return ErrorCodeValue::ArgumentNull; }

        auto * stateful = reinterpret_cast<FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION*>(updateDescription.Value);

        auto flags = stateful->Flags;

        auto validFlags = FABRIC_STATEFUL_SERVICE_TARGET_REPLICA_SET_SIZE | 
                          FABRIC_STATEFUL_SERVICE_MIN_REPLICA_SET_SIZE |
                          FABRIC_STATEFUL_SERVICE_REPLICA_RESTART_WAIT_DURATION | 
                          FABRIC_STATEFUL_SERVICE_QUORUM_LOSS_WAIT_DURATION |
                          FABRIC_STATEFUL_SERVICE_STANDBY_REPLICA_KEEP_DURATION |
                          FABRIC_STATEFUL_SERVICE_PLACEMENT_CONSTRAINTS |
                          FABRIC_STATEFUL_SERVICE_POLICY_LIST |
                          FABRIC_STATEFUL_SERVICE_CORRELATIONS |
                          FABRIC_STATEFUL_SERVICE_METRICS |
                          FABRIC_STATEFUL_SERVICE_MOVE_COST |
                          FABRIC_STATEFUL_SERVICE_SCALING_POLICY;

        if ((flags & ~validFlags) != 0) 
        { 
            return TraceAndGetInvalidArgumentError(wformatString(GET_NS_RC( Invalid_Flags ), flags));
        }

        serviceKind_ = FABRIC_SERVICE_KIND::FABRIC_SERVICE_KIND_STATEFUL;

        if ((flags & FABRIC_STATEFUL_SERVICE_TARGET_REPLICA_SET_SIZE) != 0)
        {
            updateFlags_ |= Flags::TargetReplicaSetSize;
            targetReplicaSetSize_ = stateful->TargetReplicaSetSize;
        }

        if ((flags & FABRIC_STATEFUL_SERVICE_REPLICA_RESTART_WAIT_DURATION) != 0)
        {
            updateFlags_ |= Flags::ReplicaRestartWaitDuration;
            replicaRestartWaitDuration_ = TimeSpan::FromSeconds(stateful->ReplicaRestartWaitDurationSeconds);
        }

        if ((flags & FABRIC_STATEFUL_SERVICE_QUORUM_LOSS_WAIT_DURATION) != 0)
        {
            updateFlags_ |= Flags::QuorumLossWaitDuration;
            quorumLossWaitDuration_ = TimeSpan::FromSeconds(stateful->QuorumLossWaitDurationSeconds);
        }

        if (stateful->Reserved == nullptr) { break; }

        auto * statefulEx1 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX1*>(stateful->Reserved);

        if ((flags & FABRIC_STATEFUL_SERVICE_STANDBY_REPLICA_KEEP_DURATION) != 0)
        {
            updateFlags_ |= Flags::StandByReplicaKeepDuration;
            standByReplicaKeepDuration_ = TimeSpan::FromSeconds(statefulEx1->StandByReplicaKeepDurationSeconds);
        }

        if (statefulEx1->Reserved == nullptr) { break; }

        auto * statefulEx2 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX2*>(statefulEx1->Reserved);

        if ((flags & FABRIC_STATEFUL_SERVICE_MIN_REPLICA_SET_SIZE) != 0)
        {
            updateFlags_ |= Flags::MinReplicaSetSize;
            minReplicaSetSize_ = statefulEx2->MinReplicaSetSize;
        }

        if (statefulEx2->Reserved == nullptr) { break; }

        auto * statefulEx3 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX3*>(statefulEx2->Reserved);

        if ((flags & FABRIC_STATEFUL_SERVICE_PLACEMENT_CONSTRAINTS) != 0)
        {
            updateFlags_ |= Flags::PlacementConstraints;
            HRESULT hr = StringUtility::LpcwstrToWstring(statefulEx3->PlacementConstraints, true /*acceptNull*/, placementConstraints_);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
        }

        if ((flags & FABRIC_STATEFUL_SERVICE_CORRELATIONS) != 0)
        {
            updateFlags_ |= Flags::Correlations;
            for (ULONG i = 0; i < statefulEx3->CorrelationCount; i++)
            {
                wstring correlationServiceName;
                HRESULT hr = StringUtility::LpcwstrToWstring(statefulEx3->Correlations[i].ServiceName, true /*acceptNull*/, correlationServiceName);
                if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

                Reliability::ServiceCorrelationDescription correlationDescription(correlationServiceName, statefulEx3->Correlations[i].Scheme);

                serviceCorrelations_.push_back(correlationDescription);
            }
        }

        if ((flags & FABRIC_STATEFUL_SERVICE_METRICS) != 0)
        {
            updateFlags_ |= Flags::Metrics;
            for (ULONG i = 0; i < statefulEx3->MetricCount; i++)
            {
                wstring metricsName;
                HRESULT hr = StringUtility::LpcwstrToWstring(statefulEx3->Metrics[i].Name, true /*acceptNull*/, metricsName);
                if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
                metrics_.push_back(Reliability::ServiceLoadMetricDescription(
                    move(metricsName),
                    statefulEx3->Metrics[i].Weight,
                    statefulEx3->Metrics[i].PrimaryDefaultLoad,
                    statefulEx3->Metrics[i].SecondaryDefaultLoad));
            }
        }

        if ((flags & FABRIC_STATEFUL_SERVICE_POLICY_LIST) != 0)
        {
            updateFlags_ |= Flags::PlacementPolicyList;
            auto pList = reinterpret_cast<FABRIC_SERVICE_PLACEMENT_POLICY_LIST*>(statefulEx3->PolicyList);

            for (ULONG i = 0; i < pList->PolicyCount; i++)
            {
                std::wstring domainName;
                FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION & policyDesc = pList->Policies[i];
                ServicePlacementPolicyHelper::PolicyDescriptionToDomainName(policyDesc, domainName);
                placementPolicies_.push_back(ServiceModel::ServicePlacementPolicyDescription(move(domainName), policyDesc.Type));
            }

        }

        if (statefulEx3->Reserved == nullptr) { break; }

        auto * statefulEx4 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX4*>(statefulEx3->Reserved);

        if ((flags & FABRIC_STATEFUL_SERVICE_MOVE_COST) != 0)
        {
            updateFlags_ |= Flags::DefaultMoveCost;
            defaultMoveCost_ = statefulEx4->DefaultMoveCost;
        }

        if (statefulEx4->Reserved == nullptr) { break; }

        auto * statefulEx5 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX5*>(statefulEx4->Reserved);

        auto error = this->FromRepartitionDescription(statefulEx5->RepartitionKind, statefulEx5->RepartitionDescription);
        if (!error.IsSuccess()) { return error; }

        if ((flags & FABRIC_STATEFUL_SERVICE_SCALING_POLICY) != 0)
        {
            updateFlags_ |= Flags::ScalingPolicy;

            if (statefulEx5->ScalingPolicyCount > 1)
            {
                // Currently, only one scaling policy is allowed per service.
                // Vector is there for future uses (when services could have multiple scaling policies).
                return ErrorCode(ErrorCodeValue::InvalidServiceScalingPolicy, wformatString(GET_NS_RC(ScalingPolicy_Scaling_Count), statefulEx5->ScalingPolicyCount));
            }
            if (statefulEx5->ScalingPolicyCount > 0)
            {
                for (ULONG i = 0; i < statefulEx5->ScalingPolicyCount; i++)
                {
                    Reliability::ServiceScalingPolicyDescription scalingDescription;
                    auto scalingError = scalingDescription.FromPublicApi(statefulEx5->ServiceScalingPolicies[i]);
                    if (!scalingError.IsSuccess())
                    {
                        return scalingError;
                    }
                    scalingPolicies_.push_back(move(scalingDescription));
                }
            }
        }

        break;

    } // case stateful

    default:
        return TraceAndGetInvalidArgumentError(wformatString("{0} {1}", GET_NS_RC( Invalid_Service_Kind ), static_cast<int>(updateDescription.Kind)));
    };

    return ErrorCodeValue::Success;
}

ErrorCode ServiceUpdateDescription::FromRepartitionDescription(
    FABRIC_SERVICE_PARTITION_KIND const publicKind,
    void * publicDescription)
{
    if (publicDescription == nullptr || publicKind == FABRIC_SERVICE_PARTITION_KIND_INVALID)
    {
        return ErrorCodeValue::Success;
    }

    auto kind = PartitionKind::FromPublicApi(publicKind);
    if (kind != PartitionKind::Named)
    {
        return TraceAndGetInvalidArgumentError(wformatString(GET_NS_RC(Update_Unsupported_Scheme), (kind == PartitionKind::Invalid) 
            ? static_cast<int>(publicKind) 
            : kind));
    }

    auto * casted = reinterpret_cast<FABRIC_NAMED_REPARTITION_DESCRIPTION*>(publicDescription);

    vector<wstring> toAdd;
    auto hr = StringUtility::FromLPCWSTRArray(casted->NamesToAddCount, casted->NamesToAdd, toAdd);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    vector<wstring> toRemove;
    hr = StringUtility::FromLPCWSTRArray(casted->NamesToRemoveCount, casted->NamesToRemove, toRemove);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    repartitionDescription_ = make_shared<NamedRepartitionDescription>(move(toAdd), move(toRemove));

    return ErrorCodeValue::Success;
}

ErrorCode ServiceUpdateDescription::IsValid() const
{
    if (updateFlags_ & ~Flags::ValidServiceUpdateFlags)
    {
        return TraceAndGetInvalidArgumentError(wformatString(GET_NS_RC( Invalid_Flags ), updateFlags_));
    }

    return ErrorCode::Success();
}

ErrorCode ServiceUpdateDescription::TryUpdateServiceDescription(
    __inout PartitionedServiceDescriptor & psd,
    __out bool & isUpdated,
    __out std::vector<Reliability::ConsistencyUnitDescription> & addedCuids,
    __out std::vector<Reliability::ConsistencyUnitDescription> & removedCuids) const
{
    auto & serviceDescription = psd.MutableService;

    auto error = this->TryUpdateServiceDescription(
        serviceDescription,
        true, // allowRepartition
        isUpdated);

    if (!error.IsSuccess()) { return error; }

    if (Repartition)
    {
        switch (Repartition->Kind)
        {
        case PartitionKind::Named:
        {
            auto * casted = dynamic_cast<NamedRepartitionDescription*>(Repartition.get());

            error = psd.AddNamedPartitions(casted->NamesToAdd, addedCuids);
            if (!error.IsSuccess()) { return error; }

            error = psd.RemoveNamedPartitions(casted->NamesToRemove, removedCuids);
            if (!error.IsSuccess()) { return error; }

            if (!isUpdated)
            {
                isUpdated = true;

                serviceDescription.UpdateVersion = serviceDescription.UpdateVersion + 1;
            }

            break;
        }
        default:
            return TraceAndGetInvalidArgumentError(wformatString(GET_NS_RC(Update_Unsupported_Scheme), Repartition->Kind));
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode ServiceUpdateDescription::TryUpdateServiceDescription(
    __inout ServiceDescription & serviceDescription,
    __out bool & isUpdated) const
{
    return this->TryUpdateServiceDescription(
        serviceDescription,
        false, // allowRepartition
        isUpdated);
}

ErrorCode ServiceUpdateDescription::TryUpdateServiceDescription(
    __inout ServiceDescription & serviceDescription,
    bool allowRepartition,
    __out bool & isUpdated) const
{
    isUpdated = false;

    if ((serviceKind_ == FABRIC_SERVICE_KIND::FABRIC_SERVICE_KIND_STATELESS && serviceDescription.IsStateful) ||
        (serviceKind_ == FABRIC_SERVICE_KIND::FABRIC_SERVICE_KIND_STATEFUL && !serviceDescription.IsStateful))
    {
        return TraceAndGetInvalidArgumentError(wformatString("{0} {1}", GET_NS_RC(Invalid_Service_Kind), serviceKind_));
    }

    if (UpdateTargetReplicaSetSize)
    {
        int minReplicaSetSize = (UpdateMinReplicaSetSize ? minReplicaSetSize_ : serviceDescription.MinReplicaSetSize);

        if (targetReplicaSetSize_ < 1 && (targetReplicaSetSize_ != -1 || serviceDescription.IsStateful))
        {
            return TraceAndGetInvalidArgumentError(wformatString("{0} {1}", GET_NS_RC(Invalid_Target_Replicas), targetReplicaSetSize_));
        }
        else if (serviceDescription.IsStateful && minReplicaSetSize > targetReplicaSetSize_)
        {
            return TraceAndGetInvalidArgumentError(wformatString("{0} ({1}, {2})", GET_NS_RC(Invalid_Minimum_Target), minReplicaSetSize, targetReplicaSetSize_));
        }

        serviceDescription.TargetReplicaSetSize = targetReplicaSetSize_;
        isUpdated = true;
    }

    if (UpdateMinReplicaSetSize)
    {
        if (minReplicaSetSize_ < 1 || minReplicaSetSize_ > serviceDescription.TargetReplicaSetSize)
        {
            return TraceAndGetInvalidArgumentError(wformatString("{0} ({1}, {2})", GET_NS_RC(Invalid_Minimum_Target), minReplicaSetSize_, serviceDescription.TargetReplicaSetSize));
        }

        serviceDescription.MinReplicaSetSize = minReplicaSetSize_;
        isUpdated = true;
    }

    if (UpdateReplicaRestartWaitDuration)
    {
        serviceDescription.ReplicaRestartWaitDuration = ReplicaRestartWaitDuration;
        isUpdated = true;
    }

    if (UpdateQuorumLossWaitDuration)
    {
        serviceDescription.QuorumLossWaitDuration = QuorumLossWaitDuration;
        isUpdated = true;
    }

    if (UpdateStandByReplicaKeepDuration)
    {
        serviceDescription.StandByReplicaKeepDuration = StandByReplicaKeepDuration;
        isUpdated = true;
    }

    if (UpdatePlacementConstraints)
    {
        serviceDescription.put_PlacementConstraints(std::wstring(PlacementConstraints));
        isUpdated = true;
    }

    if (UpdateMetrics)
    {
        serviceDescription.put_Metrics(std::vector<Reliability::ServiceLoadMetricDescription>(metrics_));
        isUpdated = true;
    }

    if (UpdatePlacementPolicies)
    {
        serviceDescription.put_PlacementPolicies(std::vector<ServiceModel::ServicePlacementPolicyDescription>(placementPolicies_));
        isUpdated = true;
    }

    if (UpdateCorrelations)
    {
        serviceDescription.put_Correlations(std::vector<Reliability::ServiceCorrelationDescription>(serviceCorrelations_));
        isUpdated = true;
    }

    if (UpdateDefaultMoveCost)
    {
        serviceDescription.DefaultMoveCost = defaultMoveCost_;
        isUpdated = true;
    }

    if (Repartition && !allowRepartition)
    {
        return TraceAndGetInvalidArgumentError(wformatString(GET_NS_RC(Update_Unsupported_Repartition)));
    }

    if (InitializationData)
    {
        serviceDescription.InitializationData = vector<byte>(*InitializationData);
        isUpdated = true;
    }

    if (UpdateScalingPolicy)
    {
        serviceDescription.ScalingPolicies = scalingPolicies_;
        isUpdated = true;
    }

    if (isUpdated)
    {
        serviceDescription.UpdateVersion = serviceDescription.UpdateVersion + 1;
    }

    return ErrorCode::Success();
}

ErrorCode ServiceUpdateDescription::TryDiffForUpgrade(
    __in Naming::PartitionedServiceDescriptor & targetPsd,
    __in Naming::PartitionedServiceDescriptor & activePsd,
    __out shared_ptr<ServiceUpdateDescription> & updateDescriptionResult,
    __out shared_ptr<ServiceUpdateDescription> & rollbackUpdateDescriptionResult)
{
    auto updateDescription = make_shared<ServiceUpdateDescription>();
    auto rollbackUpdateDescription = make_shared<ServiceUpdateDescription>();

    auto const & target = targetPsd.Service;
    auto const & active = activePsd.Service;

    if (targetPsd.Service.IsStateful)
    {
        if (active.MinReplicaSetSize != target.MinReplicaSetSize)
        {
            updateDescription->UpdateMinReplicaSetSize = true;
            updateDescription->MinReplicaSetSize = target.MinReplicaSetSize;
            rollbackUpdateDescription->UpdateMinReplicaSetSize = true;
            rollbackUpdateDescription->MinReplicaSetSize = active.MinReplicaSetSize;
        }

        if (active.ReplicaRestartWaitDuration != target.ReplicaRestartWaitDuration)
        {
            updateDescription->UpdateReplicaRestartWaitDuration = true;
            updateDescription->ReplicaRestartWaitDuration = target.ReplicaRestartWaitDuration;
            rollbackUpdateDescription->UpdateReplicaRestartWaitDuration = true;
            rollbackUpdateDescription->ReplicaRestartWaitDuration = active.ReplicaRestartWaitDuration;
        }

        if (active.QuorumLossWaitDuration != target.QuorumLossWaitDuration)
        {
            updateDescription->UpdateQuorumLossWaitDuration = true;
            updateDescription->QuorumLossWaitDuration = target.QuorumLossWaitDuration;
            rollbackUpdateDescription->UpdateQuorumLossWaitDuration = true;
            rollbackUpdateDescription->QuorumLossWaitDuration = active.QuorumLossWaitDuration;
        }

        if (active.StandByReplicaKeepDuration != target.StandByReplicaKeepDuration)
        {
            updateDescription->UpdateStandByReplicaKeepDuration = true;
            updateDescription->StandByReplicaKeepDuration = target.StandByReplicaKeepDuration;
            rollbackUpdateDescription->UpdateStandByReplicaKeepDuration = true;
            rollbackUpdateDescription->StandByReplicaKeepDuration = active.StandByReplicaKeepDuration;
        }
    }

    // Corresponds to min size in stateless
    if (active.TargetReplicaSetSize != target.TargetReplicaSetSize)
    {
        updateDescription->UpdateTargetReplicaSetSize = true;
        updateDescription->TargetReplicaSetSize = target.TargetReplicaSetSize;
        rollbackUpdateDescription->UpdateTargetReplicaSetSize = true;
        rollbackUpdateDescription->TargetReplicaSetSize = active.TargetReplicaSetSize;
    }

    if (active.Correlations != target.Correlations)
    {
        updateDescription->UpdateCorrelations = true;
        updateDescription->Correlations = target.Correlations;
        rollbackUpdateDescription->UpdateCorrelations = true;
        rollbackUpdateDescription->Correlations = active.Correlations;
    }

    if (active.PlacementConstraints != target.PlacementConstraints)
    {
        updateDescription->UpdatePlacementConstraints = true;
        updateDescription->PlacementConstraints = target.PlacementConstraints;
        rollbackUpdateDescription->UpdatePlacementConstraints = true;
        rollbackUpdateDescription->PlacementConstraints = active.PlacementConstraints;
    }

    if (active.Metrics != target.Metrics)
    {
        updateDescription->UpdateMetrics = true;
        updateDescription->Metrics = target.Metrics;
        rollbackUpdateDescription->UpdateMetrics = true;
        rollbackUpdateDescription->Metrics = active.Metrics;
    }

    if (active.PlacementPolicies != target.PlacementPolicies)
    {
        updateDescription->UpdatePlacementPolicies = true;
        updateDescription->PlacementPolicies = target.PlacementPolicies;
        rollbackUpdateDescription->UpdatePlacementPolicies = true;
        rollbackUpdateDescription->PlacementPolicies = active.PlacementPolicies;
    }

    if (active.DefaultMoveCost != target.DefaultMoveCost)
    {
        updateDescription->UpdateDefaultMoveCost = true;
        updateDescription->DefaultMoveCost = static_cast<FABRIC_MOVE_COST>(target.DefaultMoveCost);
        rollbackUpdateDescription->UpdateDefaultMoveCost = true;
        rollbackUpdateDescription->DefaultMoveCost = static_cast<FABRIC_MOVE_COST>(active.DefaultMoveCost);
    }

    if (activePsd.PartitionScheme == FABRIC_PARTITION_SCHEME_NAMED)
    {
        if (targetPsd.Names.empty())
        {
            return ErrorCode(
                ErrorCodeValue::ApplicationUpgradeValidationError, 
                wformatString(GET_NS_RC( Invalid_Partition_Names ), targetPsd.Service.Name));
        }

        targetPsd.SortNamesForUpgradeDiff();
        activePsd.SortNamesForUpgradeDiff();

        size_t tx = 0;
        size_t ax = 0;

        vector<wstring> addedNames;
        vector<wstring> removedNames;

        while (tx<targetPsd.Names.size() && ax<activePsd.Names.size())
        {
            if (targetPsd.Names[tx] == activePsd.Names[ax])
            {
                ++tx;
                ++ax;
            }
            else if (targetPsd.Names[tx] < activePsd.Names[ax])
            {
                addedNames.push_back(targetPsd.Names[tx++]);
            }
            else
            {
                removedNames.push_back(activePsd.Names[ax++]);
            }
        }

        while (tx<targetPsd.Names.size())
        {
            addedNames.push_back(targetPsd.Names[tx++]);
        }

        while (ax<activePsd.Names.size())
        {
            removedNames.push_back(activePsd.Names[ax++]);
        }

        // FM currently does not support adding and removing partitions 
        // in the same update operation.
        //
        if (!addedNames.empty() && !removedNames.empty())
        {
            return ErrorCode(
                ErrorCodeValue::ApplicationUpgradeValidationError, 
                wformatString(GET_NS_RC( Update_Unsupported_AddRemove ), activePsd.Service.Name));
        }

        updateDescription->Repartition = make_shared<NamedRepartitionDescription>(addedNames, removedNames);
        rollbackUpdateDescription->Repartition = make_shared<NamedRepartitionDescription>(removedNames, addedNames);
    }

    bool shouldUpdateInitData = (active.InitializationData.size() !=  target.InitializationData.size());

    if (!shouldUpdateInitData)
    {
        for (auto ix=0; ix<active.InitializationData.size(); ++ix)
        {
            if (active.InitializationData[ix] != target.InitializationData[ix])
            {
                shouldUpdateInitData = true;
                break;
            }
        }
    }

    if (shouldUpdateInitData)
    {
        updateDescription->InitializationData = make_shared<vector<byte>>(target.InitializationData);
        rollbackUpdateDescription->InitializationData = make_shared<vector<byte>>(active.InitializationData);
    }

    if (active.ScalingPolicies != target.ScalingPolicies)
    {
        updateDescription->UpdateScalingPolicy = true;
        updateDescription->ScalingPolicies = target.ScalingPolicies;
        rollbackUpdateDescription->UpdateScalingPolicy = true;
        rollbackUpdateDescription->ScalingPolicies = active.ScalingPolicies;
    }

    if (updateDescription->UpdateFlags != 0 || updateDescription->Repartition.get() != nullptr)
    {
        updateDescriptionResult = move(updateDescription);
        rollbackUpdateDescriptionResult = move(rollbackUpdateDescription);
    }

    return ErrorCodeValue::Success;
}

void ServiceUpdateDescription::WriteToEtw(uint16 contextSequenceId) const
{
    ServiceModel::ServiceModelEventSource::Trace->ServiceUpdateDescription(
        contextSequenceId,
        static_cast<int>(updateFlags_),
        targetReplicaSetSize_,
        minReplicaSetSize_,
        replicaRestartWaitDuration_,
        quorumLossWaitDuration_,
        standByReplicaKeepDuration_,
        serviceCorrelations_,
        placementConstraints_,
        metrics_,
        placementPolicies_,
        static_cast<int>(defaultMoveCost_),
        repartitionDescription_.get() == nullptr ? RepartitionDescription() : *repartitionDescription_,
        initializationData_.get() == nullptr ? -1 : static_cast<int>(initializationData_->size()));
}

void ServiceUpdateDescription::WriteTo(__in Common::TextWriter& w, Common::FormatOptions const&) const
{
    w << "flags=" << updateFlags_;
    w << ", target=" << targetReplicaSetSize_;
    w << ", min=" << minReplicaSetSize_;
    w << ", restart=" << replicaRestartWaitDuration_;
    w << ", ql=" << quorumLossWaitDuration_;
    w << ", standby=" << standByReplicaKeepDuration_;
    w << ", correlations=" << serviceCorrelations_;
    w << ", placements=" << placementConstraints_;
    w << ", metrics=" << metrics_;
    w << ", policies=" << placementPolicies_;
    w << ", movecost=" << defaultMoveCost_;

    if (repartitionDescription_) 
    { 
        w << ", repartition=[" << *repartitionDescription_ << "]"; 
    }
    else
    {
        w << ", repartition=null";
    }

    w << ", scalingPolicies=" << scalingPolicies_;
    
    if (initializationData_)
    {
        w << ", initData=" << initializationData_->size() << "bytes";
    }
    else
    {
        w << ", initData=null";
    }
}

ErrorCode ServiceUpdateDescription::TraceAndGetInvalidArgumentError(std::wstring && msg) const
{
    Trace.WriteWarning(TraceComponent, "{0}", msg);

    return ErrorCode(ErrorCodeValue::InvalidArgument, move(msg));
}
