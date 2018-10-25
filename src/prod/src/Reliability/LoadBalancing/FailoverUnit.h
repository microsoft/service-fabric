// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "FailoverUnitDescription.h"
#include "Common/Common.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class SearcherSettings;
        class Service;

        class FailoverUnit
        {
            DENY_COPY_ASSIGNMENT(FailoverUnit);
        public:
            struct ResourceLoad
            {
                ResourceLoad() {}
                ResourceLoad(int CPUCores, int MemoryInMb) : CPUCores_(CPUCores), MemoryInMb_(MemoryInMb) {}

                int CPUCores_;
                int MemoryInMb_;

            };

            FailoverUnit(
                FailoverUnitDescription && fuDescription,
                std::vector<uint> && primaryEntries,
                std::vector<uint> && secondaryEntries,
                std::map<Federation::NodeId, std::vector<uint>> && secondaryEntriesMap,
                uint primaryMoveCost,
                uint secondaryMoveCost,
                bool isServiceOnEveryNode,
                std::map<Federation::NodeId, ResourceLoad> && resourceMap,
                Common::StopwatchTime nextScalingCheck = Common::StopwatchTime::Zero);


            FailoverUnit(FailoverUnit && other);

            FailoverUnit(FailoverUnit const & other);

            __declspec (property(get=get_FailoverUnitDescription)) FailoverUnitDescription const& FuDescription;
            FailoverUnitDescription const& get_FailoverUnitDescription() const { return fuDescription_; }

            __declspec (property(get=get_FailoverUnitId)) Common::Guid const& FUId;
            Common::Guid get_FailoverUnitId() const { return fuDescription_.FUId; }

            __declspec (property(get=get_PrimaryEntries)) std::vector<uint> const& PrimaryEntries;
            std::vector<uint> const& get_PrimaryEntries() const { return primaryEntries_; }

            _declspec (property(get = get_IsPrimaryLoadReported)) std::vector<bool> const& IsPrimaryLoadReported;
            std::vector<bool> const& get_IsPrimaryLoadReported() const { return isPrimaryLoadReported_; }

            __declspec (property(get=get_SecondaryEntries)) std::vector<uint> const& SecondaryEntries;
            std::vector<uint> const& get_SecondaryEntries() const { return secondaryEntries_; }

            _declspec (property(get = get_IsSecondaryLoadReported)) std::vector<bool> const& IsSecondaryLoadReported;
            std::vector<bool> const&  get_IsSecondaryLoadReported() const { return isSecondaryLoadReported_; }

            __declspec (property(get = get_SecondaryEntriesMap)) std::map<Federation::NodeId, std::vector<uint>> const& SecondaryEntriesMap;
            std::map<Federation::NodeId, std::vector<uint>> const& get_SecondaryEntriesMap() const { return secondaryEntriesMap_; }

            _declspec (property(get = get_IsSecondaryLoadMapReported)) std::map<Federation::NodeId, std::vector<bool>> const& IsSecondaryLoadMapReported;
            std::map<Federation::NodeId, std::vector<bool>> const& get_IsSecondaryLoadMapReported() const { return isSecondaryLoadMapReported_; }

            __declspec (property(get=get_PrimaryMoveCost)) uint PrimaryMoveCost;
            uint get_PrimaryMoveCost() const { return primaryMoveCost_; }

            __declspec (property(get=get_SecondaryMoveCost)) uint SecondaryMoveCost;
            uint get_SecondaryMoveCost() const { return secondaryMoveCost_; }

            __declspec (property(get=get_ActualReplicaDifference)) int ActualReplicaDifference;
            int get_ActualReplicaDifference() const { return actualReplicaDifference_; }

            __declspec (property(get = get_ResourceLoadMap)) std::map<Federation::NodeId, ResourceLoad> const& ResourceLoadMap;
            std::map<Federation::NodeId, ResourceLoad> get_ResourceLoadMap() const { return resourceLoadMap_; }

            __declspec (property(get = get_IsServiceOnEveryNode, put = put_IsServiceOnEveryNode)) bool IsServiceOnEveryNode;
            bool get_IsServiceOnEveryNode() const { return isServiceOnEveryNode_; }
            void put_IsServiceOnEveryNode(bool val) { isServiceOnEveryNode_ = val; }

            __declspec (property(get = get_NextScalingCheck, put = put_NextScalingCheck)) Common::StopwatchTime NextScalingCheck;
            Common::StopwatchTime get_NextScalingCheck() const { return nextScalingCheck_; }
            void put_NextScalingCheck(Common::StopwatchTime val) { nextScalingCheck_ = val; }

            __declspec (property(get = get_TargetReplicaSetSize)) int TargetReplicaSetSize;
            int get_TargetReplicaSetSize() const { return fuDescription_.TargetReplicaSetSize; }

            __declspec (property(get=get_UpdateCount,put=put_UpdateCount)) int UpdateCount;
            int get_UpdateCount() const { return updateCount_; }
            void put_UpdateCount(int i) { updateCount_ = i; }

            __declspec (property(get=get_IsEmpty)) bool IsEmpty;
            bool get_IsEmpty() const { return fuDescription_.Replicas.empty() && actualReplicaDifference_ == 0; }

            void UpdateDescription(FailoverUnitDescription && description);
            void UpdateActualReplicaDifference(int value);

            bool UpdateLoad(ReplicaRole::Enum role,
                size_t index,
                uint value,
                SearcherSettings const & settings,
                bool useNodeId = false,
                Federation::NodeId const& nodeId = Federation::NodeId(),
                bool isReset = false,
                bool isSingletonReplicaService = false);
            bool UpdateResourceLoad(wstring const& metricName, int value, Federation::NodeId const& nodeId);
            bool UpdateMoveCost(ReplicaRole::Enum role, uint value, bool isMoveCostReported = true);
            bool IsMoveCostValid(uint moveCost) const;
            bool IsMoveCostReported(ReplicaRole::Enum role) const;
            uint GetMoveCostValue(ReplicaRole::Enum role, SearcherSettings const & settings) const;

            uint GetSecondaryLoad(size_t metricIndex, Federation::NodeId const& nodeId, SearcherSettings const & settings) const;
            uint GetSecondaryLoadAverage(size_t metricIndex) const;

            // Update the move map with the generated movements
            void UpdateSecondaryMoves(Federation::NodeId const& src, Federation::NodeId const& dst);
            bool HasSecondaryOnNode(Federation::NodeId const& src) const;

            // Update secondary load map for every secondary move that has finished
            // this method should be called from oldFUDescription variable with newFUDescription as a parameter
            void UpdateSecondaryLoadMap(FailoverUnitDescription const & newFailoverUnitDescription, SearcherSettings const & settings);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void ResetMovementMaps();

            //we use this to cleanup resource usage entries in case there is no replica on the node preent in the new FT description
            void CleanupResourceUsage(FailoverUnitDescription const & newFailoverUnitDescription);

            // Primary load for resources (CPU or memory)
            uint GetResourcePrimaryLoad(std::wstring const & name) const;
            // Average load for resources (CPU or memory)
            uint GetResourceAverageLoad(std::wstring const & name) const;
            // Id reported - average, it not - default
            uint GetSecondaryLoadForAutoScaling(uint metricIndex) const;
            // Average load of all replicas for metric
            uint GetAverageLoadForAutoScaling(uint metricIndex) const;

            bool IsLoadAvailable(std::wstring const & name) const;

        private:

            static bool FindAndUpdateLoad(std::vector<uint>& metrics, size_t metricIndex, uint value);

            FailoverUnitDescription fuDescription_;

            // load entries for primary replica of this FailoverUnit
            std::vector<uint> primaryEntries_;
            std::vector<bool> isPrimaryLoadReported_;

            // load entries for secondary replicas or instances of this FailoverUnit
            std::vector<uint> secondaryEntries_;
            std::vector<bool> isSecondaryLoadReported_;

            // load entries for secondary replicas or instances by node id
            std::map<Federation::NodeId, std::vector<uint>> secondaryEntriesMap_;
            std::map<Federation::NodeId, std::vector<bool>> isSecondaryLoadMapReported_;

            // Secondary replica move map: <dst : src>; used to find load after a move
            std::map<Federation::NodeId, Federation::NodeId> secondaryMoves_;

            uint primaryMoveCost_;
            bool isPrimaryMoveCostReported_;

            uint secondaryMoveCost_;
            bool isSecondaryMoveCostReported_;

            int actualReplicaDifference_;
            int updateCount_;
            bool isServiceOnEveryNode_;
            
            std::map<Federation::NodeId, ResourceLoad> resourceLoadMap_;

            Common::StopwatchTime nextScalingCheck_;
        };
    }
}
