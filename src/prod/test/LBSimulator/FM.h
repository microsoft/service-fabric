// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Node.h"
#include "Service.h"
#include "FailoverUnit.h"
#include "LoadDistribution.h"

//#include "Placement.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class PlacementAndLoadBalancing;
        class DecisionToken;
    }
}

namespace LBSimulator
{
    class LBSimulatorConfig;

    class FM;
    typedef std::shared_ptr<FM> FMSPtr;
    struct PLBRun
    {
        INT64 creations;
        INT64 creationsWithMove;
        INT64 movements;
        int noOfChainviolations;
        std::wstring action;
        std::vector<std::wstring> metrics;
        std::vector<double> averages;
        std::vector<double> stDeviations;
        std::set<std::pair<int, Common::Guid>> blockListBefore;
        std::set<std::pair<int, Common::Guid>> blockListAfter;
    };

    class FM : public Common::ComponentRoot, public Reliability::LoadBalancingComponent::IFailoverManager
    {
    public:
        static FMSPtr Create(LBSimulatorConfig const& config);

        /// <summary>
        /// Mapping from NodeName to Nodeproperties.
        /// </summary>
        /// <returns>Mapping from NodeName to Nodeproperties.</returns>
        __declspec (property(get = get_NodeToNodePropertyMap)) std::map<std::wstring, std::map<std::wstring, std::wstring>> NodeToNodePropertyMap;
        std::map<std::wstring, std::map<std::wstring, std::wstring>> get_NodeToNodePropertyMap() const {
            return nodeToNodePropertyMap_; }
        std::map<std::wstring, std::map<std::wstring, std::wstring>> get_NodeToNodePropertyMap() {
            return nodeToNodePropertyMap_; }
        /// <summary>
        /// The Number of PLB runs so far. returned as size_t(unsigned).
        /// </summary>
        /// <returns>Number Of PLB runs</returns>
        __declspec (property(get = get_NoOfPLBRuns)) size_t & NoOfPLBRuns;
        size_t  get_NoOfPLBRuns() const { return (history_.size()); }
        size_t  get_NoOfPLBRuns() { return (history_.size()); }
        /// <summary>
        /// The Number of PLB runs so far. returned as size_t(unsigned).
        /// </summary>
        /// <returns>Number Of PLB runs</returns>
        __declspec (property(get = get_NonEmpty)) bool & NonEmpty;
        bool  get_NonEmpty() const { return (!nodes_.empty()); }
        bool  get_NonEmpty() { return (!nodes_.empty()); }
        /// <summary>
        /// The list of Metrics that will be logged during PLB Runs.
        /// </summary>
        /// <returns>List of logged metrics</returns>
        __declspec (property(get = get_LoggedMetrics)) std::vector<std::wstring> LoggedMetrics;
        std::vector<std::wstring>  get_LoggedMetrics() const { return (loggedMetrics_); }
        std::vector<std::wstring>  get_LoggedMetrics() { return (loggedMetrics_); }
        /// <summary>
        /// The Total Number of Movements Applied So Far by the FM.
        /// </summary>
        /// <returns>Number of Movements</returns>
        __declspec (property(get = get_NoOfMovementsApplied)) int NoOfMovementsApplied;
        int get_NoOfMovementsApplied() const { return (noOfMovementsApplied_); }
        int get_NoOfMovementsApplied() { return (noOfMovementsApplied_); }
        /// <summary>
        /// The Total Number of Swaps Applied So Far by the FM.
        /// </summary>
        /// <returns>Number of Swaps</returns>
        __declspec (property(get = get_NoOfSwapsApplied)) int NoOfSwapsApplied;
        int get_NoOfSwapsApplied() const { return (noOfSwapsApplied_); }
        int get_NoOfSwapsApplied() { return (noOfSwapsApplied_); }

        void LogMetric(std::wstring metric);
        bool IsLogged(std::wstring metric);

        void CreateNode(Node && node);
        void DeleteNode(int nodeIndex);
        void CreateService(Service && service, bool createFailoverUnit = true);
        void AddFailoverUnit(FailoverUnit && failoverUnit);
        void AddReplica(Common::Guid failoverUnitGuid, FailoverUnit::Replica && replica);
        void DeleteReplica(Common::Guid failoverUnitGuid, int nodeIndex);
        void PromoteReplica(Common::Guid failoverUnitGuid, int nodeIndex);
        void MoveReplica(Common::Guid failoverUnitGuid, int sourceNodeIndex, int targetNodeIndex);
        void DeleteService(int serviceIndex);
        void ReportLoad(Common::Guid failoverUnitGuid, int role, std::wstring const& metric, uint load);
        Node const & GetNode(int nodeIndex) const;
        Service const & GetService(int serviceIndex) const;
        FailoverUnit const & GetFailoverUnit(int failoverUnitIndex) const;
        FailoverUnit const & GetFailoverUnit(Common::Guid failoverUnitGuid) const;
        void ReportBatchLoad(int serviceIndex, std::wstring const& metric, LoadDistribution const& loadDistribution);
        void ReportBatchMoveCost(uint moveCost);
        void ExecutePlacementAndLoadBalancing();
        void GetClusterAggregates(std::map<std::wstring, std::pair<double, double>> &metricAggregates);
        void GetNodeLoads(std::map<int, std::vector< std::pair<std::wstring, int64>>> &nodeLoads);
        void Reset(bool createPLB = true);

        Reliability::LoadBalancingComponent::PlacementAndLoadBalancing * GetPLBObject();

        void Reset(
            std::vector<Reliability::LoadBalancingComponent::NodeDescription> && nodes,
            std::vector<Reliability::LoadBalancingComponent::ApplicationDescription> && applications,
            std::vector<Reliability::LoadBalancingComponent::ServiceTypeDescription> && serviceTypes,
            std::vector<Reliability::LoadBalancingComponent::ServiceDescription> && services,
            std::vector<Reliability::LoadBalancingComponent::FailoverUnitDescription> && failoverUnits,
            std::vector<Reliability::LoadBalancingComponent::LoadOrMoveCostDescription> && loadsOrMoveCosts);

        void CreateNewService(Service && service);
        void CreateNewService(std::vector<Service::Metric> &metrics);
        void CreateNewService(std::vector<std::wstring> metricNames, std::vector<uint> metricPrimarySize);
        void WaitFuStable(Common::Guid failoverUnitIndex);
        void PrintNodeState(bool printToConsole = false);
        int ForceExecutePLB(int executionCount = 1, bool needPlacement = false, bool needConstraintCheck = false);
        void BreakChains(std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitMovement> & moveActions);
        int CountChainsOnFullNodes(std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitMovement> & moveActions);
        bool VerifyAll();
        bool AddNodeProperty(std::wstring nodeType, std::map<std::wstring, std::wstring> nodeProperties);
        bool IsBlocked(int nodeIndex, Common::Guid failoverUnitId);
        void RemoveFromBlockList(int nodeIndex, Common::Guid failoverUnitIndex);
        virtual void ProcessFailoverUnitMovements(std::map<Common::Guid,
            Reliability::LoadBalancingComponent::FailoverUnitMovement> && moveActions, Reliability::LoadBalancingComponent::DecisionToken && dToken);
        virtual void UpdateAppUpgradePLBSafetyCheckStatus(ServiceModel::ApplicationIdentifier const&);
        virtual void UpdateFailoverUnitTargetReplicaCount(Common::Guid const & partitionId, int targetCount);
        PLBRun GetPLBRunDetails(int iterationNumber);
        std::wstring NodesToString();
        std::wstring ServicesToString();
        std::wstring FailoverUnitsToString(size_t start = 0, size_t end = SIZE_MAX);
        std::wstring PlacementToString();
        void GetNodeProperties(std::wstring nodeType, std::map<std::wstring, std::wstring> & nodePropertyVector);
        void ForcePLBUpdate();
        void ForcePLBStateChange();
        bool SetServiceStartFailoverUnitIndex(int serviceIndex, int startFailoverUnitIndex);
        void InitiateAutoScaleServiceRepartitioning(std::wstring const&, int change);
    private:
        struct DomainData
        {
            DomainData(std::wstring && segment, int nodeCount)
                : Segment(move(segment)), NodeCount(nodeCount), ReplicaCount(0)
            {
            }
            std::wstring Segment;
            int NodeCount;
            int ReplicaCount;
        };
        FM(LBSimulatorConfig const& config);
        void CreatePLB();
        void CreatePLB(
            std::vector<Reliability::LoadBalancingComponent::NodeDescription> && nodes,
            std::vector<Reliability::LoadBalancingComponent::ApplicationDescription> && applications,
            std::vector<Reliability::LoadBalancingComponent::ServiceTypeDescription> && serviceTypes,
            std::vector<Reliability::LoadBalancingComponent::ServiceDescription> && services,
            std::vector<Reliability::LoadBalancingComponent::FailoverUnitDescription> && failoverUnits,
            std::vector<Reliability::LoadBalancingComponent::LoadOrMoveCostDescription> && loadsOrMoveCosts);
        void ProcessFailoverUnits();
        std::set<std::wstring> GetUniqueMetrics();
        void InternalReportLoad(int failoverUnitIndex, int role, std::wstring const& metric, uint load);
        void UpdateLoad(int failoverUnitIndex);
        Node & GetNode(int nodeIndex);
        Service & GetService(int serviceIndex);
        FailoverUnit & GetFailoverUnit(int failoverUnitIndex);
        void GetFailoverUnits (std::wstring serviceName, std::vector<Common::Guid> failoverUnits);
        FailoverUnit & GetFailoverUnit(Common::Guid guid);
        bool VerifyNodes();
        bool VerifyServices();
        bool VerifyStable();
        bool FailoverUnitsStable();

        int GetMetricAggregates(const std::wstring metric, double &avg, double &dev);
        int GetTotalMetricCount();

        LBSimulatorConfig const& config_;
        Common::Random random_;
        Reliability::LoadBalancingComponent::IPlacementAndLoadBalancingUPtr plb_;

        std::map<std::wstring, std::map<std::wstring, std::wstring>> nodeToNodePropertyMap_;
        std::map<int, Node> nodes_;
        std::map<int, Service> services_;
        std::vector<std::wstring> loggedMetrics_;

        int nextFailoverUnitIndex_;
        std::map<Common::Guid, FailoverUnit> failoverUnits_;

        mutable Common::RwLock lock_;

        int noOfMovementsApplied_;
        int noOfSwapsApplied_;
        int totalChainViolations_;
        int64 currentTicks_;
        std::vector<PLBRun> history_;
        std::vector<PLBRun> archive_;
        std::set<std::pair<int, Common::Guid>> blockList_;
        int nodeBlockDuration_;

        std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitMovement> moveActions_;
    };
}
