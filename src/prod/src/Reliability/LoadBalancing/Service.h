// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Accumulator.h"
#include "DynamicBitSet.h"
#include "ServiceDescription.h"
#include "ReplicaDescription.h"
#include "ServiceDomainLocalMetric.h"
#include "LoadEntry.h"
#include "Hasher.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class FailoverUnit;
        class FailoverUnitDescription;
        class ReplicaDescription;
        class PlacementAndLoadBalancing;
        class SearcherSettings;
        class ServiceDomain;

        class Service
        {
            DENY_COPY(Service);

        public:

           struct Type
            {
                enum Enum
                {
                    Packing = 0,
                    Nonpacking = 1,
                    Ignore = 2,
                };
            };

            explicit Service(ServiceDescription && desc);

            Service(Service && other);

            __declspec (property(get=get_ServiceDescription)) ServiceDescription const& ServiceDesc;
            ServiceDescription const& get_ServiceDescription() const { return serviceDesc_; }

            __declspec (property(get=get_BlockList)) DynamicBitSet const& BlockList;
            DynamicBitSet const& get_BlockList() const { return overallBlockList_; }

            __declspec (property(get=get_ServiceBlockList)) DynamicBitSet const& ServiceBlockList;
            DynamicBitSet const& get_ServiceBlockList() const { return serviceBlockList_; }

            __declspec (property(get=get_PrimaryReplicaBlockList)) DynamicBitSet const& PrimaryReplicaBlockList;
            DynamicBitSet const& get_PrimaryReplicaBlockList() const { return primaryReplicaBlockList_; }

            __declspec (property(get=get_FailoverUnitId, put=set_FailoverUnitId)) Common::Guid FailoverUnitId;
            Common::Guid get_FailoverUnitId() const { return failoverUnitId_; }

            __declspec(property(get=get_FailoverUnits)) std::set<Common::Guid> const& FailoverUnits;
            std::set<Common::Guid> const& get_FailoverUnits() const { return failoverUnits_; }

            __declspec (property(get=get_ActualPartitionCount, put=set_ActualPartitionCount)) size_t ActualPartitionCount;
            size_t get_ActualPartitionCount() const { return actualPartitionCount_; }

            __declspec (property(get=get_NonEmptyPartitionCount, put=set_NonEmptyPartitionCount)) size_t NonEmptyPartitionCount;
            size_t get_NonEmptyPartitionCount() const { return nonEmptyPartitionCount_; }

            __declspec (property(get=get_IsSingleton, put=set_IsSingleton)) bool IsSingleton;
            bool get_IsSingleton() const { return isSingleton_; }

            __declspec (property(get=get_FDDistribution, put=set_FDDistribution)) Type::Enum FDDistribution;
            Type::Enum get_FDDistribution() const { return FDDistribution_; }
            void set_FDDistribution(Type::Enum value) { FDDistribution_ = value; }

            __declspec (property(get = get_PartiallyPlace, put=set_PartiallyPlace)) bool PartiallyPlace;
            bool get_PartiallyPlace() const { return partiallyPlace_; }
            void set_PartiallyPlace(bool value) { partiallyPlace_ = value; }

            __declspec(property(get = get_NextASCheck, put = put_nextASCheck)) Common::StopwatchTime NextAutoScaleCheck;
            Common::StopwatchTime get_NextASCheck() const { return nextAutoScaleCheck_; }
            void put_nextASCheck(Common::StopwatchTime const& value) { nextAutoScaleCheck_ = value; }

            void UpdateServiceBlockList(DynamicBitSet && blockList) 
            { 
                serviceBlockList_ = std::move(blockList); 
            }

            void UpdateOverallBlockList(DynamicBitSet && blockList)
            {
                overallBlockList_ = std::move(blockList);
            }

            void UpdatePrimaryReplicaBlockList(DynamicBitSet && blockList) 
            { 
                primaryReplicaBlockList_ = std::move(blockList); 
            }

            void OnFailoverUnitAdded(FailoverUnit const& failoverUnit, GuidUnorderedMap<FailoverUnit> const& existingFUTable, SearcherSettings const & settings);
            void OnFailoverUnitChanged(FailoverUnit const& oldFU, FailoverUnitDescription const& newFU);
            void OnFailoverUnitDeleted(FailoverUnit const& failoverUnit, SearcherSettings const & settings);

            // get the default loads for a new failover unit of this service
            std::vector<uint> GetDefaultLoads(ReplicaRole::Enum role) const;

            uint GetDefaultMetricLoad(uint metricIndex, ReplicaRole::Enum role) const;

            bool IsLoadAvailableForPartitions(std::wstring const& metricName, ServiceDomain const& serviceDomain);

            bool GetAverageLoadPerPartitions(std::wstring const& metricName, ServiceDomain const& serviceDomain, bool useOnlyPrimaryLoad, double & averageLoad) const;

            // get the default move cost for a new failover unit of this service
            uint GetDefaultMoveCost(ReplicaRole::Enum role) const;

            int GetCustomLoadIndex(std::wstring const& name) const;

            bool ContainsMetric(std::wstring const& name) const;

            void AddNodeLoad(FailoverUnit const& failoverUnit, std::vector<ReplicaDescription> const& replicas, SearcherSettings const & settings);
            void DeleteNodeLoad(FailoverUnit const& failoverUnit, std::vector<ReplicaDescription> const& replicas, SearcherSettings const & settings);
            LoadEntry GetNodeLoad(Federation::NodeId nodeId, bool shouldDisappear) const;
            void SetFDDistributionPolicy(std::wstring const& distPolicy);
            void SetPlaceDistributionPolicy(std::wstring const& distPolicy);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void VerifyMetricLoads(std::map<std::wstring, ServiceDomainLocalMetric> const& baseLoads, bool includeDisappearingLoad) const;

            void UpdateServiceDescription(ServiceDescription && serviceDescription) { serviceDesc_ = move(serviceDescription); }

        private:
            ServiceDescription serviceDesc_;
            size_t actualPartitionCount_;
            Common::Guid failoverUnitId_; // the first FailoverUnit id
            std::set<Common::Guid> failoverUnits_; // All failover units that belong to this service

            size_t nonEmptyPartitionCount_; // the number of partitions with non empty replicas or non-zero replicadiff

            bool isSingleton_; // whether the service only has one partition

            // all replicas load except MoveInProgress or ToBeDropped replicas
            std::vector<ServiceDomainLocalMetric> metricTable_;

            // MoveInProgress and ToBeDropped replicas load
            std::vector<ServiceDomainLocalMetric> shouldDisappearMetricTable_;

            // the following entries are updated during RefreshStates
            DynamicBitSet serviceBlockList_; // service block list
            DynamicBitSet overallBlockList_; // union of service & service type block list
            DynamicBitSet primaryReplicaBlockList_; // primary replica block list

            Type::Enum FDDistribution_;
            bool partiallyPlace_;

            Common::StopwatchTime nextAutoScaleCheck_;
        };

        void WriteToTextWriter(Common::TextWriter & w, Service::Type::Enum const & val);
    }
}
