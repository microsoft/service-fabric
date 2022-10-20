// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LBSimulator
{
    class Node;

    class Service
    {
        DENY_COPY(Service);
    public:
        static std::wstring const PrimaryCountName;
        static std::wstring const SecondaryCountName;
        static std::wstring const ReplicaCountName;
        static std::wstring const InstanceCountName;
        static std::wstring const CountName;

        static bool IsBuiltInMetric(std::wstring const& metricName);

        struct Metric
        {
            Metric(std::wstring && name, double weight, uint primaryDefaultLoad, uint secondaryDefaultLoad)
                : Name(move(name)), Weight(weight), PrimaryDefaultLoad(primaryDefaultLoad), SecondaryDefaultLoad(secondaryDefaultLoad)
            {
            }

            std::wstring Name;
            double Weight;
            uint PrimaryDefaultLoad;
            uint SecondaryDefaultLoad;

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
            {
                writer.Write("{0} {1} {2} {3}", Name, Weight, PrimaryDefaultLoad, SecondaryDefaultLoad);
            }
        };

        Service(int index, std::wstring serviceName, bool isStateful, int partitionCount, int targetReplicaSetSize,
            wstring affinitizedService, std::set<int> && blockList, std::vector<Metric> && metrics,
            uint defaultMoveCost, std::wstring placementConstraints, int minReplicaSetSize = 0, int startFailoverUnitIndex = -1, bool hasPersistedState = true);

        Service(Service && other);

        Service & operator = (Service && other);

        __declspec (property(get=get_Index)) int Index;
        int get_Index() const {return index_;}

        __declspec (property(get=get_IsStateful)) bool IsStateful;
        bool get_IsStateful() const {return isStateful_;}

        __declspec (property(get = get_HasPersistedState)) bool HasPersistedState;
        bool get_HasPersistedState() const { return hasPersistedState_; }

        __declspec (property(get=get_PartitionCount)) int PartitionCount;
        int get_PartitionCount() const {return partitionCount_;}

        __declspec (property(get=get_ReplicaCountPerPartition)) int TargetReplicaSetSize;
        int get_ReplicaCountPerPartition() const {return targetReplicaSetSize_;}

        __declspec (property(get=get_BlockList)) std::set<int> const& BlockList;
        std::set<int> const& get_BlockList() const {return blockList_;}

        __declspec (property(get=get_IsEmpty)) bool IsEmpty;
        bool get_IsEmpty() const {return startFailoverUnitIndex_ < 0;}

        __declspec (property(get=get_StartFailoverUnitIndex)) int StartFailoverUnitIndex;
        int get_StartFailoverUnitIndex() const {return startFailoverUnitIndex_;}

        __declspec (property(get=get_EndFailoverUnitIndex)) int EndFailoverUnitIndex;
        int get_EndFailoverUnitIndex() const {return startFailoverUnitIndex_ + partitionCount_ - 1;}

        __declspec (property(get=get_Metrics)) std::vector<Metric> const& Metrics;
        std::vector<Metric> const& get_Metrics() const {return metrics_;}

        __declspec (property(get = get_AffinitizedService)) wstring AffinitizedService;
        wstring get_AffinitizedService() const { return affinitizedService_; }

        __declspec (property(get=get_DefaultMoveCost)) uint DefaultMoveCost;
        uint get_DefaultMoveCost() const { return defaultMoveCost_; }

        __declspec (property(get = get_PlacementConstraints)) std::wstring const& PlacementConstraints;
        std::wstring const& get_PlacementConstraints() const { return placementConstraints_; }

        __declspec (property(get = get_ServiceName)) std::wstring const& ServiceName;
        std::wstring const& get_ServiceName() const { return serviceName_; }

        void SetStartFailoverUnitIndex(int startIndex)
        {
            startFailoverUnitIndex_ = startIndex;
        }
        void AddToBlockList(int nodeIndex) { blockList_.insert(nodeIndex); }
        void RemoveFromBlockList(int nodeIndex) { if (!blockList_.count(nodeIndex)) { blockList_.erase(nodeIndex); } }
        bool InBlockList(int nodeIndex) const;

        Reliability::LoadBalancingComponent::ServiceDescription GetPLBServiceDescription() const;
        Reliability::LoadBalancingComponent::ServiceTypeDescription GetPLBServiceTypeDescription() const;
        Reliability::LoadBalancingComponent::ServiceTypeDescription GetPLBServiceTypeDescription(std::map<int, Node> const& nodes) const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

    private:
        // don't consider persisted service now
        std::wstring serviceName_;
        std::wstring placementConstraints_;

        int index_;
        bool isStateful_;
        bool hasPersistedState_;
        uint partitionCount_;
        uint targetReplicaSetSize_;
        //this one is not currently Used...
        int minReplicaSetSize_;
        std::set<int> blockList_;

        int startFailoverUnitIndex_;

        std::vector<Metric> metrics_;

        wstring affinitizedService_;

        uint defaultMoveCost_;
    };
}
