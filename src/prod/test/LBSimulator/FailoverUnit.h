// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Node.h"

namespace LBSimulator
{
    class FailoverUnit
    {
        DENY_COPY(FailoverUnit);
    public:
        enum ReplicaRole
        {
            Primary = 0,
            Secondary ,
            Standby ,
            Dropped,
            None,
            ToBeDropped
        };

        struct Replica
        {
            Replica() :
                NodeIndex(-1),
                NodeInstance(-1),
                Role(-1)
            {
            }

            Replica(int nodeIndex, int nodeInstance, int role) :
                NodeIndex(nodeIndex),
                NodeInstance(nodeInstance),
                Role(role)
            {
            }

                Replica(int nodeIndex, int nodeInstance, int role, bool isStandBy,
                    bool isToBePromoted, bool isToBeDropped, bool isInBuild, bool isDown) :
            NodeIndex(nodeIndex),
                NodeInstance(nodeInstance),
                Role(role)
            {
                UNREFERENCED_PARAMETER(isStandBy);
                UNREFERENCED_PARAMETER(isToBePromoted);
                UNREFERENCED_PARAMETER(isToBeDropped);
                UNREFERENCED_PARAMETER(isInBuild);
                UNREFERENCED_PARAMETER(isDown);
            }

            int NodeIndex;
            int NodeInstance;
            int Role;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        };

        FailoverUnit(
            Common::Guid failoverUnitId,
            int serviceIndex,
            std::wstring serviceName,
            int targetReplicaCount,
            bool isStateful,
            std::map<std::wstring, uint> && primaryLoad,
            std::map<std::wstring, uint> && secondaryLoad);

        void ChangeVersion(int increment = 1);

        FailoverUnit(FailoverUnit && other);

        FailoverUnit & operator = (FailoverUnit && other);

        __declspec (property(get=get_Index)) int Index;
        int get_Index() const {return index_;}

        __declspec (property(get=get_FailoverUnitId)) Common::Guid FailoverUnitId;
        Common::Guid get_FailoverUnitId() const { return failoverUnitId_; }

        __declspec (property(get = get_ServiceName)) std::wstring ServiceName;
        std::wstring get_ServiceName() const { return serviceName_; }

        __declspec (property(get=get_ServiceIndex)) int ServiceIndex;
        int get_ServiceIndex() const {return serviceIndex_;}

        __declspec (property(get=get_TargetReplicaCount)) int TargetReplicaCount;
        int get_TargetReplicaCount() const {return targetReplicaCount_;}

        __declspec (property(get=get_IsStateful)) bool IsStateful;
        bool get_IsStateful() const {return isStateful_;}

        __declspec (property(get=get_Version)) int Version;
        int get_Version() const {return version_;}

        __declspec (property(get=get_Replicas)) std::vector<Replica> const& Replicas;
        std::vector<Replica> const& get_Replicas() const {return replicas_;}

        __declspec (property(get=get_PrimaryLoad)) std::map<std::wstring, uint> const& PrimaryLoad;
        std::map<std::wstring, uint> const& get_PrimaryLoad() const
        {
            ASSERT_IFNOT(IsStateful, "Primary load is only available for stateful partition");
            return primaryLoad_;
        }

        __declspec (property(get=get_SecondaryLoad)) std::map<std::wstring, uint> const& SecondaryLoad;
        std::map<std::wstring, uint> const& get_SecondaryLoad() const
        {
            ASSERT_IFNOT(IsStateful, "Secondary load is only available for stateful partition");
            return secondaryLoad_;
        }

        __declspec (property(get=get_InstanceLoad)) std::map<std::wstring, uint> const& InstanceLoad;
        std::map<std::wstring, uint> const& get_InstanceLoad() const
        {
            ASSERT_IFNOT(!IsStateful, "Instance load is only available for stateless partition");
            return secondaryLoad_;
        }

        void UpdateLoad(int role, std::wstring const& metric, uint load);

        uint GetLoad(Replica const& replica, std::wstring const& metricName) const;

        void AddReplica(Replica && replica);

        void ReplicaDown(int nodeIndex);

        void PromoteReplica(int nodeIndex);

        void MoveReplica(int sourceNodeIndex, int targetNodeIndex, std::map<int, LBSimulator::Node> const& nodes);

        void FailoverUnitToBeDeleted();

        void ProcessMovement(Reliability::LoadBalancingComponent::FailoverUnitMovement const& moveAction,
            std::map<int, Node> const& nodes, std::set<int> ignoredActions);

        Reliability::LoadBalancingComponent::FailoverUnitDescription GetPLBFailoverUnitDescription() const;

        Reliability::LoadBalancingComponent::LoadOrMoveCostDescription GetPLBLoadOrMoveCostDescription() const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

    private:

        std::vector<Replica>::iterator GetReplica(int nodeIndex);
        bool ContainsReplica(int nodeIndex) const;
        std::vector<Replica>::iterator GetPrimary();
        Common::Guid failoverUnitId_;
        int index_;
        std::wstring serviceName_;
        int serviceIndex_;
        int targetReplicaCount_;
        bool isStateful_;
        // dimension: metricCount
        std::map<std::wstring, uint> primaryLoad_;
        std::map<std::wstring, uint> secondaryLoad_;
        std::vector<Replica> replicas_;

        mutable int version_;
    };
}
