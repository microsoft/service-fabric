// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "Common/StringUtility.h"
#include "SchedulerStage.h"
#include "IConstraint.h"
#include "Common/StringUtility.h"
#include "ServiceModel/federation/NodeId.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServiceEntryDiagnosticsData
        {
            DENY_COPY(ServiceEntryDiagnosticsData);
        public:

            ServiceEntryDiagnosticsData(
                std::wstring && placementConstraints)
            : placementConstraints_(std::move(placementConstraints))
            {
            }

            ServiceEntryDiagnosticsData(ServiceEntryDiagnosticsData && other)
            : placementConstraints_(std::move(other.placementConstraints_))
            {
            }

            ServiceEntryDiagnosticsData & operator = (ServiceEntryDiagnosticsData && other)
            {
                if (this != &other)
                {
                    placementConstraints_ = std::move(other.placementConstraints_);
                }

                return *this;
            }

        std::wstring placementConstraints_;
        };

        class Node;
        class Service;
        class IConstraintDiagnosticsData;
        class BalancingDiagnosticsData;
        class BasicDiagnosticInfo
        {
            std::vector<Common::Guid> partitionIds_;
            std::vector<std::pair<Federation::NodeId, std::wstring>> nodes_;

        public:
            BasicDiagnosticInfo();
            BasicDiagnosticInfo(BasicDiagnosticInfo && other);
            BasicDiagnosticInfo & operator = (BasicDiagnosticInfo && other);

            __declspec (property(get = get_PartitionIds)) std::vector<Common::Guid> const & PartitionIds;
            std::vector<Common::Guid> const & get_PartitionIds() const { return partitionIds_; }

            __declspec (property(get = get_Nodes)) std::vector<std::pair<Federation::NodeId, std::wstring>> const & Nodes;
            std::vector<std::pair<Federation::NodeId, std::wstring>> const & get_Nodes() const { return nodes_; }

            std::wstring serviceName_;
            std::wstring miscellanious_;

            void AddPartition(Common::Guid const & partitionId);
            void AddNode(Federation::NodeId const & nodeId, std::wstring const & nodeName);

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
            {
                writer.Write("ServiceName: {0} \r\n", serviceName_.empty() ? L"N/A" : serviceName_);
                writer.Write("\t Partitions: \r\n");

                for (auto p : PartitionIds)
                {
                    writer.Write("\t\t {0}\r\n", p);
                }

                writer.Write("Nodes: \r\n");

                for (auto n : Nodes)
                {
                    writer.Write("\t {0} {1}\r\n", n.second, n.first);
                }

                if (!miscellanious_.empty()) writer.Write("Miscellanious: {0} \r\n", miscellanious_);
            };

        };

        typedef std::shared_ptr<BalancingDiagnosticsData> BalancingDiagnosticsDataSPtr;

        class ServiceDomainSchedulerStageDiagnostics
        {
            DENY_COPY(ServiceDomainSchedulerStageDiagnostics);
        public:
            ServiceDomainSchedulerStageDiagnostics();
            ServiceDomainSchedulerStageDiagnostics(ServiceDomainSchedulerStageDiagnostics && other);
            ServiceDomainSchedulerStageDiagnostics & operator = (ServiceDomainSchedulerStageDiagnostics && other);

            bool CreationActionNeeded() const {return newReplicaNeeded_ || upgradeNeeded_ || dropNeeded_;}

            __declspec (property(get=get_DecisionGuid)) Common::Guid DecisionGuid;
            Common::Guid get_DecisionGuid() const { return decisionGuid_; }

            std::wstring DiagnosticsMessage();

            bool skipRefreshSchedule_;

            SchedulerStage::Stage stage_;
            bool newReplicaNeeded_;
            bool upgradeNeeded_;
            bool dropNeeded_;

            bool constraintCheckEnabledConfig_;
            bool constraintCheckPaused_;
            bool balancingEnabledConfig_;
            bool balancingPaused_;

            bool hasMovableReplica_;

            bool placementExpired_;
            bool constraintCheckExpired_;
            bool balancingExpired_;

            bool noExpiredTimerActions_;

            bool wasConstraintCheckRun_;
            bool creationWithMoveEnabled_;
            bool isConstraintCheckNodeChangeThrottled_;

            bool isbalanced_;
            bool isBalancingNodeChangeThrottled_;
            bool isBalancingSmallChangeThrottled_;

            std::vector<std::shared_ptr<IConstraintDiagnosticsData>> constraintsDiagnosticsData_;
            BalancingDiagnosticsDataSPtr balancingDiagnosticsData_;

            bool decisionScheduled_;

            std::wstring metricProperty_;
            std::wstring metricString_;

            bool AllViolationsSkippable();

            bool IsTracingNeeded();

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;
        private:
            bool WhyNot(Common::TextWriter& writer, SchedulerStage::Stage stage_) const;
            Common::Guid decisionGuid_;

        };

        class SchedulingDiagnostics
        {
            DENY_COPY(SchedulingDiagnostics);

        public:
            typedef std::shared_ptr<ServiceDomainSchedulerStageDiagnostics> ServiceDomainSchedulerStageDiagnosticsSPtr;

            SchedulingDiagnostics();

            void AddStageDiagnostics(std::wstring const & serviceDomainId, ServiceDomainSchedulerStageDiagnosticsSPtr const & stageDiagnostics);
            void CleanupStageDiagnostics();
            ServiceDomainSchedulerStageDiagnosticsSPtr GetLatestStageDiagnostics(std::wstring const & serviceDomainId);

            BalancingDiagnosticsDataSPtr AddOrGetSDBalancingDiagnostics(std::wstring const & serviceDomainId, bool createIfNull);

            void setKeepHistory(bool keepHistory)
            {
                this->keepHistory_ = keepHistory;
            }


        private:
            bool keepHistory_;
            //Having this be a vector type so that it's easy to add snapshot diffing in subsequent updates
            std::map<std::wstring, std::vector<ServiceDomainSchedulerStageDiagnosticsSPtr>> schedulerDiagnosticsMap_;
            //This is separated out because the Balancing stats are run at a different cadence than constraint/placement is, so the lifetimes need to be separately managed
            std::map<std::wstring, std::shared_ptr<BalancingDiagnosticsData>> sdBalancingDiagnosticsMap_;

        };

        class IConstraintDiagnosticsData
        {
         public:
             static std::shared_ptr<IConstraintDiagnosticsData> MakeSharedChildOfType(IConstraint::Enum childType);

             virtual std::wstring DiagnosticsMessage() = 0;

             virtual void AddBasicDiagnosticsInfo(BasicDiagnosticInfo && info);

             virtual void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
             {
                 writer.Write("{0} \r\n", constraintType_);

                 for (auto const & i : basicDiagnosticsInfo_)
                 {
                     writer.Write("\t\t\t {0} \r\n", i);
                 }
             };

             virtual std::wstring RelevantNodes();


             IConstraint::Enum constraintType_;
             std::vector<BasicDiagnosticInfo> basicDiagnosticsInfo_;
             bool changed_;
             std::vector<Node> const * plbNodesPtr_;
        };

        class DecisionToken
        {
            DENY_COPY(DecisionToken);

            Common::Guid decisionId_;

            // 0th Bit ClientAPICall
            // 1st Bit Skip
            // 2nd Bit NoneExpired
            // 3rd Bit Placement
            // 4th Bit Balancing
            // 9th Bit Constraint
            // 10th Bit Onwards, which Constraints
            int32 reasonToken_;

        public:
            DecisionToken() : decisionId_(Common::Guid::NewGuid()), reasonToken_(0) {};
            DecisionToken(Common::Guid id, int32 reason) : decisionId_(id), reasonToken_(reason) {};
            DecisionToken(DecisionToken && other) : decisionId_(std::move(other.decisionId_)), reasonToken_(std::move(other.reasonToken_)) {};

            __declspec (property(get=get_DecisionId)) Common::Guid DecisionId;
            Common::Guid get_DecisionId() const { return decisionId_; }

            __declspec (property(get=get_ReasonToken)) int32 ReasonToken;
            int32 get_ReasonToken() const { return reasonToken_; }

            void WriteToEtw(uint16 contextSequenceId) const;

            void SetToken(Common::Guid const & id, SchedulerStage::Stage stage, std::vector<std::shared_ptr<IConstraintDiagnosticsData>> & data);

            bool CheckToken(SchedulerStage::Stage stage) const;

            bool CheckToken(SchedulerStage::Stage stage, IConstraint::Enum constraintType) const;

            SchedulerStage::Stage GetStage() const;

            std::wstring ToString() const;
        };

        void WriteToTextWriter(Common::TextWriter & w, DecisionToken const & val);

        class MetricBalancingDiagnosticInfo
        {
        public:
            bool isDefrag_ = false;

            int64 minLoad_ = 0;
            int64 maxLoad_ = 0;

            Federation::NodeId minNode_;
            std::wstring minNodeName_;
            Federation::NodeId maxNode_;
            std::wstring maxNodeName_;

            int64 metricActivityThreshold_ = 0;
            double metricBalancingThreshold_ = 1;
            std::wstring metricName_;
            int globalOrServiceIndex_ = -1;
            std::wstring serviceName_;

            bool isBalanced_ = false;
            bool needsDefrag_ = false;

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
            {
                writer.Write("\t--Metric: [Name, ActivityThreshold, BalancingThreshold]: [{0}, {1}, {2}] \r\n", metricName_, metricActivityThreshold_, metricBalancingThreshold_);
                writer.Write("\t--Minimum [Load, Node]: [{0}, {1} -- {2}] \r\n", minLoad_, minNodeName_, minNode_);
                writer.Write("\t--Maximum [Load, Node]: [{0}, {1} -- {2}] \r\n", maxLoad_, maxNodeName_, maxNode_);
                writer.Write("\t--[Metric Imbalance Scope, isBalanced, needsDefrag_]: [{0}, {1}, {2}]", (globalOrServiceIndex_ == -1) ? L"Global" : serviceName_, isBalanced_, needsDefrag_);
            }
        };

        class BalancingDiagnosticsData
        {
            DENY_COPY(BalancingDiagnosticsData);
        public:
            BalancingDiagnosticsData();
            BalancingDiagnosticsData(BalancingDiagnosticsData && other);
            BalancingDiagnosticsData & operator = (BalancingDiagnosticsData && other);

            bool changed_;
            std::map<Federation::NodeId, uint64> const * plbNodeToIndexMap_;
            std::vector<Node> const * plbNodesPtr_;
            std::vector<Service const*> const* servicesListPtr_;
            std::vector<MetricBalancingDiagnosticInfo> metricDiagnostics;

            std::wstring DiagnosticsMessage()
            {
                return L"BalancingDiagnosticsData                               " + Common::StringUtility::ToWString(changed_) + Common::StringUtility::ToWString(metricDiagnostics.size());
            };

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
            {
                writer.Write("Number of Metric Imbalances {0}, \r\n", metricDiagnostics.size());

                for (auto i : metricDiagnostics)
                {
                    writer.Write("{0} \r\n\r\n", i);
                }
            };
        };

////////////////////////////////////////////////////////////////////////////////////////////////////
        /* The Below Classes are for Scheduler's IsConstraintSatisfied() Diagnostics */
////////////////////////////////////////////////////////////////////////////////////////////////////

        class MetricCapacityDiagnosticInfo
        {

        public:

            MetricCapacityDiagnosticInfo();
            MetricCapacityDiagnosticInfo(
                std::wstring const & metricName,
                Federation::NodeId const & nodeId,
                std::wstring const & nodeName,
                int64 load,
                int64 capacity,
                std::wstring const & applicationName = L"");
            MetricCapacityDiagnosticInfo(MetricCapacityDiagnosticInfo && other);
            MetricCapacityDiagnosticInfo & operator = (MetricCapacityDiagnosticInfo && other);

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
            {
                writer.Write("[{0}, {1}, {2}, {3}, {4}, {5}]", metricName_, load_, capacity_, nodeId_, nodeName_, applicationName_.empty() ? L"N/A" : applicationName_);
            };

            std::wstring metricName_;
            Federation::NodeId nodeId_;
            std::wstring nodeName_;
            int64 load_;
            int64 capacity_;
            std::wstring applicationName_;

        };

        class ReplicaExclusionStaticConstraintDiagnosticsData: public IConstraintDiagnosticsData
        {
        public:

            ReplicaExclusionStaticConstraintDiagnosticsData(){changed_ = false; constraintType_ = IConstraint::ReplicaExclusionStatic;}

            std::wstring DiagnosticsMessage()
            {
                return L"ReplicaExclusionStaticConstraintDiagnosticsData        " + Common::StringUtility::ToWString(changed_);
            };
        };

        class ReplicaExclusionDynamicConstraintDiagnosticsData: public IConstraintDiagnosticsData
        {
        public:

            ReplicaExclusionDynamicConstraintDiagnosticsData(){changed_ = false;  constraintType_ = IConstraint::ReplicaExclusionDynamic;}

            std::wstring DiagnosticsMessage()
            {
                return L"ReplicaExclusionDynamicConstraintDiagnosticsData       " + Common::StringUtility::ToWString(changed_);
            };

        };

        class PlacementConstraintDiagnosticsData: public IConstraintDiagnosticsData
        {
        public:

            PlacementConstraintDiagnosticsData(){changed_ = false;  constraintType_ = IConstraint::PlacementConstraint;}

            std::wstring DiagnosticsMessage()
            {
                return L"PlacementConstraintDiagnosticsData                     " + Common::StringUtility::ToWString(changed_);
            };

        };

        class NodeCapacityConstraintDiagnosticsData: public IConstraintDiagnosticsData
        {
        public:

            NodeCapacityConstraintDiagnosticsData(){changed_ = false;  constraintType_ = IConstraint::NodeCapacity;}

            virtual void AddMetricDiagnosticInfo(MetricCapacityDiagnosticInfo && info);

            std::vector<MetricCapacityDiagnosticInfo> metricDiagnosticsInfo_;

            std::wstring DiagnosticsMessage()
            {
                return L"NodeCapacityConstraintDiagnosticsData                  " + Common::StringUtility::ToWString(changed_);
            };

            virtual void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
            {
                writer.Write("{0} \r\n", constraintType_);
                writer.Write("\t\t[MetricName, Load, Capacity, NodeId, NodeName, ApplicationName] \r\n");

                for (auto const & i : metricDiagnosticsInfo_)
                {
                    writer.Write("\t\t--{0} \r\n", i);
                }
            };
        };

        class AffinityConstraintDiagnosticsData: public IConstraintDiagnosticsData
        {
        public:

            AffinityConstraintDiagnosticsData(){changed_ = false; constraintType_ = IConstraint::Affinity;}

            std::wstring DiagnosticsMessage()
            {
                return L"AffinityConstraintDiagnosticsData                      " + Common::StringUtility::ToWString(changed_);
            };

        };


        class FaultDomainConstraintDiagnosticsData: public IConstraintDiagnosticsData
        {
        public:

            FaultDomainConstraintDiagnosticsData(){changed_ = false;  constraintType_ = IConstraint::FaultDomain;}

            std::wstring DiagnosticsMessage()
            {
                return L"FaultDomainConstraintDiagnosticsData                   " + Common::StringUtility::ToWString(changed_);
            };

        };

        class UpgradeDomainConstraintDiagnosticsData: public IConstraintDiagnosticsData
        {
        public:

            UpgradeDomainConstraintDiagnosticsData(){changed_ = false; constraintType_ = IConstraint::UpgradeDomain;}

            std::wstring DiagnosticsMessage()
            {
                return L"UpgradeDomainConstraintDiagnosticsData                 " + Common::StringUtility::ToWString(changed_);
            };

        };

        class PreferredLocationConstraintDiagnosticsData: public IConstraintDiagnosticsData
        {
        public:

            PreferredLocationConstraintDiagnosticsData(){changed_ = false; constraintType_ = IConstraint::PreferredLocation;}

            std::wstring DiagnosticsMessage()
            {
                return L"PreferredLocationConstraintDiagnosticsData             " + Common::StringUtility::ToWString(changed_);
            }

        };

        class ScaleoutCountConstraintDiagnosticsData: public IConstraintDiagnosticsData
        {
        public:

            ScaleoutCountConstraintDiagnosticsData(){changed_ = false; constraintType_ = IConstraint::ScaleoutCount;}

            std::wstring DiagnosticsMessage()
            {
                return L"ScaleoutCountConstraintDiagnosticsData                 " + Common::StringUtility::ToWString(changed_);
            };

        };

        class ApplicationCapacityConstraintDiagnosticsData: public IConstraintDiagnosticsData
        {
        public:

            ApplicationCapacityConstraintDiagnosticsData(){changed_ = false; constraintType_ = IConstraint::ApplicationCapacity;}

            virtual void AddMetricDiagnosticInfo(MetricCapacityDiagnosticInfo && info);

            std::vector<MetricCapacityDiagnosticInfo> metricDiagnosticsInfo_;

            std::wstring DiagnosticsMessage()
            {
                return L"ApplicationCapacityConstraintDiagnosticsData           " + Common::StringUtility::ToWString(changed_);
            };

            virtual void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
            {
                writer.Write("{0} \r\n", constraintType_);
                writer.Write("\t\t[MetricName, Load, Capacity, NodeId, NodeName, ApplicationName] \r\n");

                for (auto const & i : metricDiagnosticsInfo_)
                {
                    writer.Write("\t\t--{0} \r\n", i);
                }
            };
        };

        class ThrottlingConstraintDiagnosticsData : public IConstraintDiagnosticsData
        {
        public:

            ThrottlingConstraintDiagnosticsData() { changed_ = false; constraintType_ = IConstraint::ScaleoutCount; }

            std::wstring DiagnosticsMessage()
            {
                return L"ThrottlingCountConstraintDiagnosticsData                 " + Common::StringUtility::ToWString(changed_);
            };

        };

////////////////////////////////////////////////////////////////////////////////////////////////////
        /* The Below Classes are for unplaced replica diagnostics*/
////////////////////////////////////////////////////////////////////////////////////////////////////

        class NodeCapacityViolationDiagnosticsData : public IConstraintDiagnosticsData
        {
            DENY_COPY(NodeCapacityViolationDiagnosticsData);
        public:
            NodeCapacityViolationDiagnosticsData(
                std::wstring && metricName,
                int64 replicaLoad,
                int64 currentLoad,
                int64 disappearingLoad,
                int64 appearingLoad,
                int64 reservedLoad,
                int64 capacity,
                bool diagnosticsSucceeded);

            NodeCapacityViolationDiagnosticsData(NodeCapacityViolationDiagnosticsData && other);

            NodeCapacityViolationDiagnosticsData & operator = (NodeCapacityViolationDiagnosticsData && other);

            std::wstring DiagnosticsMessage()
            {
                return diagnosticsSucceeded_ ? Common::wformatString(L"Blocked by Metric {0}: (ReplicaLoad: {1} + ExistingLoad: {2} + DisappearingLoad: {3} + ReservedLoad: {4} PotentialLoad: {5} ) > CurrentCapacity: {6} ",
                                metricName_, replicaLoad_, currentLoad_, disappearingLoad_, reservedLoad_, appearingLoad_, capacity_) : Common::wformatString("Diagnostics Failed");
            }

            std::wstring metricName_;
            int64 replicaLoad_;
            int64 currentLoad_;
            int64 disappearingLoad_;
            int64 appearingLoad_;
            int64 reservedLoad_;
            int64 capacity_;
            bool diagnosticsSucceeded_;
        };
    }
}
