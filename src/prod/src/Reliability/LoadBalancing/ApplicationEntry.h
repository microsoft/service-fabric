// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Application.h"
#include "LoadEntry.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class PlacementAndLoadBalancing;

        class ApplicationEntry
        {
            DENY_COPY(ApplicationEntry);
        public:

            static Common::GlobalWString const TraceDescription;

            ApplicationEntry(
                std::wstring && name,
                uint64 applicationId,
                int scaleoutCount,
                LoadEntry && appCapacities,
                LoadEntry && perNodeCapacities,
                LoadEntry && reservation,
                LoadEntry && totalLoad,
                std::map<NodeEntry const*, LoadEntry> && nodeLoads,
                std::map<NodeEntry const*, LoadEntry> && disappearingNodeLoads,
                std::map<NodeEntry const*, size_t> && nodeCounts,
                std::set<Common::TreeNodeIndex> && ugradedUDs);

            ApplicationEntry(ApplicationEntry && other);

            ApplicationEntry & operator = (ApplicationEntry && other);

            __declspec (property(get = get_Name)) std::wstring const& Name;
            std::wstring const& get_Name() const { return name_; }

            __declspec (property(get = get_ApplicationId)) uint64 const& ApplicationId;
            uint64 const& get_ApplicationId() const { return applicationId_; }

            __declspec (property(get = get_ScaleoutCount, put = set_ScaleoutCount)) int ScaleoutCount;
            int get_ScaleoutCount() const { return scaleoutCount_; }
            void set_ScaleoutCount(int value) { scaleoutCount_ = value; }

            __declspec (property(get = get_AppCapacities)) LoadEntry const& AppCapacities;
            LoadEntry const& get_AppCapacities() const { return appCapacities_; }

            __declspec (property(get = get_PerNodeCapacities)) LoadEntry const& PerNodeCapacities;
            LoadEntry const& get_PerNodeCapacities() const { return perNodeCapacities_; }

            __declspec (property(get = get_Reservation)) LoadEntry const& Reservation;
            LoadEntry const& get_Reservation() const { return reservation_; }

            __declspec (property(get = get_HasAppCapacity)) bool HasAppCapacity;
            bool get_HasAppCapacity() const { return hasAppCapacity_; }

            __declspec (property(get = get_HasPerNodeCapacity)) bool HasPerNodeCapacity;
            bool get_HasPerNodeCapacity() const { return hasPerNodeCapacity_; }

            __declspec (property(get = get_HasScaleoutOrCapacity)) bool HasScaleoutOrCapacity;
            bool get_HasScaleoutOrCapacity() const { return hasScaleoutOrCapacity_; }

            __declspec (property(get = get_HasReservation)) bool HasReservation;
            bool get_HasReservation() const { return hasReservation_; }

            __declspec (property(get = get_NodeLoads)) std::map<NodeEntry const*, LoadEntry> const& NodeLoads;
            std::map<NodeEntry const*, LoadEntry> const& get_NodeLoads() const { return nodeLoads_; }

            __declspec (property(get = get_DisappearingNodeLoads)) std::map<NodeEntry const*, LoadEntry> const& DisappearingNodeLoads;
            std::map<NodeEntry const*, LoadEntry> const& get_DisappearingNodeLoads() const { return disappearingNodeLoads_; }

            __declspec (property(get = get_NodeCounts)) std::map<NodeEntry const*, size_t> const& NodeCounts;
            std::map<NodeEntry const*, size_t> const& get_NodeCounts() const { return nodeCounts_; }

            __declspec (property(get = get_TotalLoads)) LoadEntry const& TotalLoads;
            LoadEntry const& get_TotalLoads() const { return totalLoads_; }

            __declspec (property(get = get_UpgradeCompletedUDs)) std::set<Common::TreeNodeIndex> const& UpgradeCompletedUDs;
            std::set<Common::TreeNodeIndex> const& get_UpgradeCompletedUDs() const { return upgradeCompletedUDs_; }

            __declspec (property(get = get_HasPartitionsInSingletonReplicaUpgrade, put = set_HasPartitionsInSingletonReplicaUpgrade)) bool HasPartitionsInSingletonReplicaUpgrade;
            bool get_HasPartitionsInSingletonReplicaUpgrade() const { return hasPartitionsInSingletonReplicaUpgrade_; }
            void set_HasPartitionsInSingletonReplicaUpgrade(bool value) { hasPartitionsInSingletonReplicaUpgrade_ = value; }

            __declspec (property(get = get_CorelatedSingletonReplicasInUpgrade)) std::vector<PlacementReplica const*> const& CorelatedSingletonReplicasInUpgrade;
            std::vector<PlacementReplica const*> const& get_CorelatedSingletonReplicasInUpgrade() const { return corelatedSingletonReplicasInUpgrade_; }

            void SetRelaxedScaleoutReplicaSet(Placement const* placement);
            bool IsInSingletonReplicaUpgrade(PlacementAndLoadBalancing const& plb, vector<PlacementReplica *>& replicas) const;

            // return the actual reservation.
            // If actual node load is greater than reservation capacity, the effective reservation is 0;
            // If actual load is less, the load reservation is the diff
            int64 GetReservationDiff(size_t capacityIndex, int64 actualNodeLoad) const;

            int64 GetMetricNodeLoad(NodeEntry const* node, size_t metricIndex) const;

            int64 GetDisappearingMetricNodeLoad(NodeEntry const* node, size_t metricIndex) const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            std::wstring name_;
            uint64 applicationId_;
            int scaleoutCount_;

            // Dimension: global metric #
            LoadEntry appCapacities_;
            LoadEntry perNodeCapacities_;
            LoadEntry reservation_;

            // whether any one of the metric has a finite capacity (capacity>=0)
            bool hasAppCapacity_;
            bool hasPerNodeCapacity_;
            bool hasScaleoutOrCapacity_;
            bool hasReservation_;

            // Node loads of active replicas in this application
            std::map<NodeEntry const*, LoadEntry> nodeLoads_;
            // Node loads of disappearing replicas in this application (ToBeDropped and MoveInProgress)
            std::map<NodeEntry const*, LoadEntry> disappearingNodeLoads_;

            // Node to replica count mapping
            std::map<NodeEntry const*, size_t> nodeCounts_;

            LoadEntry totalLoads_;

            std::set<Common::TreeNodeIndex> upgradeCompletedUDs_;

            // Does application have partitions which are in singleton replica upgrade
            // or in node deactivation process
            bool hasPartitionsInSingletonReplicaUpgrade_;
            // Correlated replicas in relaxed scaleout singleton replica upgrade optimization
            std::vector<PlacementReplica const*> corelatedSingletonReplicasInUpgrade_;

        };
    }
}
