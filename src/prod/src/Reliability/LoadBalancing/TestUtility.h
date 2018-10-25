// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "PlacementAndLoadBalancingPublic.h"
#include "PlacementAndLoadBalancing.h"

namespace PlacementAndLoadBalancingUnitTest
{
    template<class T>
    class ScopeChange
    {
        DENY_COPY(ScopeChange);
    public:
        ScopeChange(T & target, T value) : original_(target), target_(target) { target_ = value; }
        ~ScopeChange() { target_ = original_; }
    private:
        T original_;
        T & target_;
    };

#define VerifySystemErrorThrown(function) \
    { \
        Assert::DisableDebugBreakInThisScope breakDisabled; \
        bool systemExceptionThrown = false; \
        try \
        { \
            function; \
        } \
        catch (std::system_error const&) \
        { \
            systemExceptionThrown = true; \
        } \
        VERIFY_IS_TRUE(systemExceptionThrown); \
    }

#define PLBConfigScopeChange(key, type, newValue) \
    class ScopeChangeClass##key \
    { \
    public: \
    ScopeChangeClass##key() : original_(PLBConfig::GetConfig().key) {} \
    ~ScopeChangeClass##key() { SetValue(original_); } \
    void SetValue(type value) { PLBConfig::GetConfig().key = value; } \
    private: \
    type original_; \
    } ScopeChangeObject##key; \
    ScopeChangeObject##key.SetValue(newValue);

#define PLBConfigScopeModify(key, newValue) \
    ScopeChangeObject##key.SetValue(newValue);

#define PLBConfigCondScopeChange(key, condFn, config1, config2, type, newValue) \
    class ScopeChangeClass##key \
    { \
    public: \
    ScopeChangeClass##key(bool cond) : cond_(cond), original_(cond ? PLBConfig::GetConfig().config1 : PLBConfig::GetConfig().config2) {} \
    ~ScopeChangeClass##key() { SetValue(original_); } \
    void SetValue(type value) { cond_ ? PLBConfig::GetConfig().config1 = value : PLBConfig::GetConfig().config2 = value; } \
    private: \
    type original_; \
    bool cond_; \
    } ScopeChangeObject##key(condFn()); \
    ScopeChangeObject##key.SetValue(newValue);

#define PLBConfigCondScopeModify(key, newValue) \
    ScopeChangeObject##key.SetValue(newValue);

#define FailoverConfigScopeChange(key, type, newValue) \
    class ScopeChangeClass##key \
    { \
    public: \
    ScopeChangeClass##key() : original_(Reliability::FailoverConfig::GetConfig().key.GetValue()) {} \
    ~ScopeChangeClass##key() { SetValue(original_); } \
    void SetValue(type value) { Reliability::FailoverConfig::GetConfig().key.Test_SetValue(value); } \
    private: \
    type original_; \
    } ScopeChangeObject##key; \
    ScopeChangeObject##key.SetValue(newValue);

#define CountIf(container, predicate) \
    count_if(container.begin(), container.end(), [=](decltype(*container.begin()) const & value){ return predicate; })

    Federation::NodeId CreateNodeId(int id);

    Federation::NodeInstance CreateNodeInstance(int id, uint64 instance);

    Common::Guid CreateGuid(int i);

    int GetNodeId(Federation::NodeId nodeId);

    int GetFUId(Common::Guid fuId);

    std::vector<std::wstring> GetActionListString(std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitMovement> const& actionList);

    size_t GetActionRawCount(std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitMovement> const& actionList);

    bool ActionMatch(std::wstring const& expected, std::wstring const& actual);

    bool OneOfMatch(std::wstring const& expected, std::wstring const& actual);

    std::vector<Reliability::LoadBalancingComponent::ReplicaDescription> CreateReplicas(wstring replicas, std::vector<bool>& isReplicaUp);
    
    std::vector<Reliability::LoadBalancingComponent::ReplicaDescription> CreateReplicas(std::wstring replicas, bool isUp = true);

    std::vector<Reliability::LoadBalancingComponent::ServiceMetric> CreateMetrics(std::wstring metrics);

    std::map<std::wstring, uint> CreateCapacities(std::wstring capacityStr);

    std::map<std::wstring, Reliability::LoadBalancingComponent::ApplicationCapacitiesDescription> CreateApplicationCapacities(std::wstring appCapacities);

    size_t NumberOfDomains(std::vector<std::set<std::wstring>> const& metricNameSetVector);

    Reliability::LoadBalancingComponent::LoadOrMoveCostDescription CreateLoadOrMoveCost(
        int failoverUnitId,
        std::wstring serviceName,
        std::wstring metricName,
        uint primaryLoad,
        uint secondaryLoad);

    Reliability::LoadBalancingComponent::LoadOrMoveCostDescription CreateLoadOrMoveCost(
        int failoverUnitId,
        std::wstring serviceName,
        std::wstring metricName,
        uint instanceLoad);

    Reliability::LoadBalancingComponent::LoadOrMoveCostDescription CreateLoadOrMoveCost(
        int failoverUnitId,
        std::wstring serviceName,
        std::wstring metricName,
        uint primaryLoad,
        std::map<Federation::NodeId, uint> const& secondaryLoads,
        bool isStateful = true);

    Reliability::LoadBalancingComponent::NodeDescription CreateNodeDescription(
        int nodeId,
        std::wstring fdPath = std::wstring(),
        std::wstring ud = std::wstring(),
        std::map<std::wstring, std::wstring> && nodeProperties = std::map<std::wstring, std::wstring>(),
        std::wstring capacityStr = std::wstring(),
        bool isUp = true);

    Reliability::LoadBalancingComponent::NodeDescription CreateNodeDescriptionWithCapacity(
        int nodeId,
        std::wstring capacityStr,
        bool isUp = true);

    Reliability::LoadBalancingComponent::NodeDescription CreateNodeDescriptionWithResources(
        int nodeId,
        uint cpuCores,
        uint memoryInMegaBytes,
        std::map<std::wstring, std::wstring> && nodeProperties = std::map<std::wstring, std::wstring>(),
        std::wstring capacityStr = std::wstring(),
        std::wstring ud = L"",
        std::wstring fdPath = L"",
        bool isUp = true);

    Reliability::LoadBalancingComponent::NodeDescription CreateNodeDescriptionWithPlacementConstraintAndCapacity(
        int nodeId,
        std::wstring capacityStr,
        std::map<std::wstring, std::wstring> && nodeProperties);

    Reliability::LoadBalancingComponent::NodeDescription CreateNodeDescriptionWithFaultDomainAndCapacity(
        int nodeId,
        std::wstring fdPath,
        std::wstring capacityStr,
        Reliability::NodeDeactivationIntent::Enum deactivationIntent = Reliability::NodeDeactivationIntent::Enum::None,
        Reliability::NodeDeactivationStatus::Enum deactivationStatus = Reliability::NodeDeactivationStatus::Enum::None);

    Reliability::LoadBalancingComponent::NodeDescription CreateNodeDescriptionWithDomainsAndCapacity(
        int nodeId,
        std::wstring fdPath,
        std::wstring ud,
        std::wstring capacityStr,
        Reliability::NodeDeactivationIntent::Enum deactivationIntent = Reliability::NodeDeactivationIntent::Enum::None,
        Reliability::NodeDeactivationStatus::Enum deactivationStatus = Reliability::NodeDeactivationStatus::Enum::None);

    Reliability::LoadBalancingComponent::ServicePackageDescription CreateServicePackageDescription(
        ServiceModel::ServicePackageIdentifier servicePackageIdentifier,
        double cpuCores,
        uint memoryInBytes,
        std::vector<std::wstring>&& codePackages = std::vector<std::wstring>());

    Reliability::LoadBalancingComponent::ServicePackageDescription CreateServicePackageDescription(
        std::wstring servicePackageName,
        std::wstring appTypeName,
        std::wstring applicationName,
        double cpuCores,
        uint memoryInBytes,
        ServiceModel::ServicePackageIdentifier & spId,
        std::vector<std::wstring>&& codePackages = std::vector<std::wstring>());

    Reliability::LoadBalancingComponent::ServiceDescription CreateServiceDescriptionWithServicePackage(
        std::wstring serviceName,
        std::wstring serviceTypeName,
        bool isStateful,
        ServiceModel::ServicePackageIdentifier & servicePackageIdentifier,
        std::vector<Reliability::LoadBalancingComponent::ServiceMetric> && metrics = std::vector<Reliability::LoadBalancingComponent::ServiceMetric>(),
        bool isSharedServicePackage = true,
        wstring appName = L"",
        std::wstring placementConstraint = L"");

    Reliability::LoadBalancingComponent::ServiceDescription CreateServiceDescription(
        std::wstring serviceName,
        std::wstring serviceTypeName,
        bool isStateful,
        std::vector<Reliability::LoadBalancingComponent::ServiceMetric> && metrics = std::vector<Reliability::LoadBalancingComponent::ServiceMetric>(),
        uint defaultMoveCost = FABRIC_MOVE_COST_LOW,
        bool onEveryNode = false,
        int targetReplicaSetSize = 0,
        bool hasPersistedState = true,
        ServiceModel::ServicePackageIdentifier servicePackageIdentifier = ServiceModel::ServicePackageIdentifier(),
        bool isShared = true);

    Reliability::LoadBalancingComponent::ServiceDescription CreateServiceDescriptionWithPartitionAndReplicaCount(
        std::wstring serviceName,
        std::wstring serviceTypeName,
        bool isStateful,
        int partitionCount,
        int replicaCount,
        std::vector<Reliability::LoadBalancingComponent::ServiceMetric> && metrics = std::vector<Reliability::LoadBalancingComponent::ServiceMetric>());

    Reliability::LoadBalancingComponent::ServiceDescription CreateServiceDescriptionWithAffinity(
        std::wstring serviceName,
        std::wstring serviceTypeName,
        bool isStateful,
        std::wstring affinitizedService,
        std::vector<Reliability::LoadBalancingComponent::ServiceMetric> && metrics = std::vector<Reliability::LoadBalancingComponent::ServiceMetric>(),
        bool onEveryNode = false,
        int targetReplicaSetSize = 0,
        bool hasPersistedState = true,
        bool allignedAffinity = true,
        ServiceModel::ServicePackageIdentifier servicePackageIdentifier = ServiceModel::ServicePackageIdentifier(),
        bool isShared = true);

    Reliability::LoadBalancingComponent::ServiceDescription CreateServiceDescriptionWithAffinityWithEmptyApplication(
        std::wstring serviceName,
        std::wstring serviceTypeName,
        bool isStateful,
        std::wstring affinitizedService,
        std::vector<Reliability::LoadBalancingComponent::ServiceMetric> && metrics = std::vector<Reliability::LoadBalancingComponent::ServiceMetric>());

    Reliability::LoadBalancingComponent::ServiceDescription CreateServiceDescriptionWithAffinityAndApplication(
        std::wstring serviceName,
        std::wstring serviceTypeName,
        bool isStateful,
        std::wstring applicationName,
        std::wstring affinitizedService,
        std::vector<Reliability::LoadBalancingComponent::ServiceMetric> && metrics = std::vector<Reliability::LoadBalancingComponent::ServiceMetric>());

    Reliability::LoadBalancingComponent::ServiceDescription CreateServiceDescriptionWithNonAlignedAffinity(
        std::wstring serviceName,
        std::wstring serviceTypeName,
        bool isStateful,
        std::wstring affinitizedService,
        std::vector<Reliability::LoadBalancingComponent::ServiceMetric> && metrics = std::vector<Reliability::LoadBalancingComponent::ServiceMetric>());

    Reliability::LoadBalancingComponent::ServiceDescription CreateServiceDescriptionWithConstraint(
        std::wstring serviceName,
        std::wstring serviceTypeName,
        bool isStateful,
        std::wstring placementConstraint,
        std::vector<Reliability::LoadBalancingComponent::ServiceMetric> && metrics = std::vector<Reliability::LoadBalancingComponent::ServiceMetric>(),
        bool isOnEveryNode = false,
        int targetReplicaSetSize = 0);

    Reliability::LoadBalancingComponent::ServiceDescription CreateServiceDescriptionWithApplication(
        std::wstring serviceName,
        std::wstring serviceTypeName,
        std::wstring applicationName,
        bool isStateful,
        std::vector<Reliability::LoadBalancingComponent::ServiceMetric> && metrics = std::vector<Reliability::LoadBalancingComponent::ServiceMetric>(),
        ServiceModel::ServicePackageIdentifier servicePackageIdentifier = ServiceModel::ServicePackageIdentifier(),
        bool isShared = true,
        int partitionCount = 0);

    Reliability::LoadBalancingComponent::ServiceDescription CreateServiceDescriptionWithEmptyApplication(
        std::wstring serviceName,
        std::wstring serviceTypeName,
        bool isStateful,
        std::vector<Reliability::LoadBalancingComponent::ServiceMetric> && metrics = std::vector<Reliability::LoadBalancingComponent::ServiceMetric>());

    vector<Reliability::ServiceScalingPolicyDescription> CreateAutoScalingPolicyForPartition(
        wstring metricName,
        double lowerLoadThreshold,
        double upperLoadThreshold,
        uint scaleIntervalInSeconds,
        long minInstanceCount,
        long maxInstanceCount,
        long scaleIncrement);

    vector<Reliability::ServiceScalingPolicyDescription> CreateAutoScalingPolicyForService(
        wstring metricName,
        double lowerLoadThreshold,
        double upperLoadThreshold,
        uint scaleIntervalInSeconds,
        long minPartitionCount,
        long maxPartitionCount,
        long scaleIncrement,
        bool useOnlyPrimaryLoad = false);

    Reliability::LoadBalancingComponent::ServiceDescription CreateServiceDescriptionWithAutoscalingPolicy(
        std::wstring serviceName,
        std::wstring serviceTypeName,
        bool isStateful,
        int targetReplicaSetSize,
        vector<Reliability::ServiceScalingPolicyDescription> && scalingPolicies,
        std::vector<Reliability::LoadBalancingComponent::ServiceMetric> && metrics = std::vector<Reliability::LoadBalancingComponent::ServiceMetric>(),
        int partitionCount = 1);

    Reliability::LoadBalancingComponent::ApplicationDescription CreateApplicationDescriptionWithCapacities(
        std::wstring applicationName,
        int scaleoutCount,
        std::map<std::wstring, Reliability::LoadBalancingComponent::ApplicationCapacitiesDescription> && capacities,
        std::vector<Reliability::LoadBalancingComponent::ServicePackageDescription> servicePackages = std::vector<Reliability::LoadBalancingComponent::ServicePackageDescription>(),
        std::wstring appTypeName = L"");

    Reliability::LoadBalancingComponent::ApplicationDescription CreateApplicationDescriptionWithCapacities(
        std::wstring applicationName,
        int minimalCount,
        int scaleoutCount,
        std::map<std::wstring, Reliability::LoadBalancingComponent::ApplicationCapacitiesDescription> && capacities,
        bool isUpgradeInProgress = false,
        std::set<std::wstring> && completedUDs = std::set<std::wstring>());

    Reliability::LoadBalancingComponent::ApplicationDescription CreateApplicationWithServicePackages(
        std::wstring appTypeName,
        std::wstring appName,
        std::vector<Reliability::LoadBalancingComponent::ServicePackageDescription> servicePackages,
        std::vector<Reliability::LoadBalancingComponent::ServicePackageDescription> servicePackagesUpgrading = std::vector<Reliability::LoadBalancingComponent::ServicePackageDescription>());

    Reliability::LoadBalancingComponent::ApplicationDescription CreateApplicationWithServicePackagesAndCapacity(
        std::wstring appTypeName,
        std::wstring appName,
        int minimumCount,
        int scaleoutCount,
        std::map<std::wstring, Reliability::LoadBalancingComponent::ApplicationCapacitiesDescription> && capacities,
        std::vector<Reliability::LoadBalancingComponent::ServicePackageDescription> servicePackages,
        std::vector<Reliability::LoadBalancingComponent::ServicePackageDescription> servicePackagesUpgrading = std::vector<Reliability::LoadBalancingComponent::ServicePackageDescription>());

    void VerifyPLBAction(Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & plb, std::wstring const& action, std::wstring const & metricName = L"");

    void GetClusterLoadMetricInformation(
        Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & plb,
        ServiceModel::LoadMetricInformation& loadMetricInfo,
        std::wstring metricName);

    void GetNodeLoadMetricInformation(
        Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & plb,
        ServiceModel::NodeLoadMetricInformation& nodeLoadMetricInfo,
        int nodeIndex,
        std::wstring const& metricName);

    void VerifyCapacity(
        ServiceModel::ClusterLoadInformationQueryResult & queryResult,
        std::wstring metricName,
        uint expectedValue);

    void VerifyLoad(
        ServiceModel::ClusterLoadInformationQueryResult & queryResult,
        std::wstring metricName,
        uint expectedValue);

    void VerifyLoad(
        ServiceModel::ClusterLoadInformationQueryResult & queryResult,
        std::wstring metricName,
        uint expectedValue,
        bool expectedIsCapacityViolation);

    void VerifyClusterLoadDouble(
        ServiceModel::ClusterLoadInformationQueryResult & queryResult,
        std::wstring metricName,
        uint expectedValue,
        double expectedValueD,
        uint expectedCapacity,
        uint expectedRemaining,
        double expectedRemainingD,
        bool expectedIsCapacityViolation);

    void VerifyNodeLoadQuery(
        Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & plb,
        int nodeIndex,
        std::wstring const& metricName,
        std::vector<uint> const & expectedValues,
        bool metricMustBePresent = true);

    void VerifyNodeLoadQuery(
        Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & plb,
        int nodeIndex,
        std::wstring const& metricName,
        uint expectedValue,
        bool metricMustBePresent = true);

    void VerifyNodeLoadQueryWithDoubleValues(
        Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & plb,
        int nodeIndex,
        std::wstring const& metricName,
        uint expectedValue,
        double expectedValueD,
        uint remainingCapacity,
        double remainingCapacityD);

    void VerifyNodeCapacity(
        Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & plb,
        int nodeIndex,
        std::wstring const& metricName,
        uint expectedValue);

    void VerifyApplicationLoadQuery(ServiceModel::ApplicationLoadInformationQueryResult, std::wstring metricName, uint expectedValue);

}
