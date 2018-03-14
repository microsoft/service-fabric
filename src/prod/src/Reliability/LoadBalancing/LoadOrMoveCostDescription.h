// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ReplicaDescription.h"
#include "LoadMetricStats.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class LoadMetric;

        class LoadOrMoveCostDescription : public Serialization::FabricSerializable
        {
            DEFAULT_COPY_CONSTRUCTOR(LoadOrMoveCostDescription)
        public:
            static std::wstring const& GetStoreType();

            LoadOrMoveCostDescription();

            LoadOrMoveCostDescription(
                Common::Guid failoverUnitId,
                std::wstring && serviceName,
                bool isStateful,
                bool isReset = false);

            LoadOrMoveCostDescription(LoadOrMoveCostDescription && other);

            LoadOrMoveCostDescription & operator=(LoadOrMoveCostDescription && other);

            static bool CreateLoadReportForResources(
                Common::Guid const & ft, std::wstring const & serviceName, 
                bool isStateful, ReplicaRole::Enum replicaRole, Federation::NodeId const& nodeId, 
                double cpuUsage, uint64 memoryUsage, LoadOrMoveCostDescription & loadDescription);

            bool IsValidReport() { return failoverUnitId_ != Common::Guid::Empty(); }

            __declspec (property(get=get_FailoverUnitId)) Common::Guid FailoverUnitId;
            Common::Guid get_FailoverUnitId() const { return failoverUnitId_; }

            __declspec (property(get=get_ServiceName)) std::wstring const& ServiceName;;
            std::wstring const& get_ServiceName() const { return serviceName_; }

            __declspec (property(get=get_IsStateful)) bool IsStateful;;
            bool get_IsStateful() const { return isStateful_; }

            __declspec (property(get = get_IsReset)) bool IsReset;;
            bool get_IsReset() const { return isReset_; }

            __declspec (property(get=get_PrimaryEntries)) std::vector<LoadMetricStats> const & PrimaryEntries;
            std::vector<LoadMetricStats> const & get_PrimaryEntries() const
            {
                ASSERT_IFNOT(isStateful_, "Accessing primary entries of a stateless failover unit's load {0}", failoverUnitId_);
                return primaryEntries_;
            }

            __declspec (property(get=get_SecondaryEntries)) std::vector<LoadMetricStats> const & SecondaryEntries;
            std::vector<LoadMetricStats> const & get_SecondaryEntries() const
            {
                return secondaryEntries_;
            }

            __declspec (property(get = get_SecondaryEntriesMap)) std::map<Federation::NodeId, std::vector<LoadMetricStats>> const & SecondaryEntriesMap;
            std::map<Federation::NodeId, std::vector<LoadMetricStats>> const & get_SecondaryEntriesMap() const
            {
                return secondaryEntriesMap_;
            }

            void AdjustTimestamps(Common::TimeSpan diff);

            bool MergeLoads(
                ReplicaRole::Enum replicaRole,
                std::vector<LoadMetric> && loads,
                Common::StopwatchTime timestamp,
                bool useNodeId = true,
                Federation::NodeId const& nodeId = Federation::NodeId()
                );

            bool MergeLoads(LoadOrMoveCostDescription && loads);

            void AlignSingletonReplicaLoads();

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;

            FABRIC_FIELDS_06(failoverUnitId_, serviceName_, isStateful_, primaryEntries_, secondaryEntries_, secondaryEntriesMap_);

        private:
            static bool FindAndUpdate(std::vector<LoadMetricStats> & loadList_, LoadMetric && newLoad, Common::StopwatchTime timestamp);
            static bool FindAndUpdate(std::vector<LoadMetricStats> & loadList_, LoadMetricStats && newLoad);
            static bool FindAndUpdateSecondaryLoad(std::map<Federation::NodeId, std::vector<LoadMetricStats>> & secondaryLoad, Federation::NodeId const&, std::vector<LoadMetricStats> && newLoad);
            static bool FindAndUpdateSecondaryLoad(std::map<Federation::NodeId, std::vector<LoadMetricStats>> & secondaryLoad, Federation::NodeId const&, std::vector<LoadMetric> && newLoad, Common::StopwatchTime timestamp);

            Common::Guid failoverUnitId_;
            std::wstring serviceName_;
            bool isStateful_;
            std::vector<LoadMetricStats> primaryEntries_;
            std::vector<LoadMetricStats> secondaryEntries_;

            //map of nodeId to loads
            std::map<Federation::NodeId, std::vector<LoadMetricStats>> secondaryEntriesMap_;

            // True only if load report is actually partiton load reset
            bool isReset_;
        };
    }
}

DEFINE_USER_MAP_UTILITY(Federation::NodeId, std::vector<Reliability::LoadBalancingComponent::LoadMetricStats>);
DEFINE_USER_ARRAY_UTILITY(Reliability::LoadBalancingComponent::LoadOrMoveCostDescription);
