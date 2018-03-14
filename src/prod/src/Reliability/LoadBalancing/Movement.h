// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "PartitionEntry.h"
#include "PlacementReplica.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class NodeEntry;
        class ServiceEntry;

        class Movement
        {
        public:
            struct Type
            {
                enum Enum
                {
                    None = 0,
                    Swap = 1,
                    Move = 2,
                    Add = 3,
                    Promote = 4,
                    AddAndPromote = 5,
                    Void = 6,
                    Drop = 7
                };
            };

            static Movement Invalid;

            static Movement Create(PlacementReplica const* r1, PlacementReplica const* r2, bool forUpgrade = false);

            static Movement Create(PlacementReplica const* replica, NodeEntry const* targetNode, bool forUpgrade = false);

            static Movement CreatePromoteSecondaryMovement(PartitionEntry const* partition,
                NodeEntry const* targetNode,
                PlacementReplica const* targetReplica,
                bool isTestMode);

            //A migrate is whatever necessary to get a primary onto the target node minimally, whether, swap, add and promote, or move
            static Movement CreateMigratePrimaryMovement(PartitionEntry const* partition, NodeEntry const* targetNode, bool isTestMode);

            static Movement CreateAddAndPromoteMovement(PlacementReplica const* replica, NodeEntry const* targetNode);

            static Movement CreateVoidMovement(PartitionEntry const* partition, NodeEntry const* targetNode);

            static Movement CreateDrop(PlacementReplica const* replica, NodeEntry const* sourceNode);

            Movement();

            // using compiler generated copy constructor and assignment constructor

            __declspec (property(get=get_Service)) ServiceEntry const* Service;
            ServiceEntry const* get_Service() const { return partition_->Service; }

            __declspec (property(get=get_Partition)) PartitionEntry const* Partition;
            PartitionEntry const* get_Partition() const { return partition_; }

            __declspec (property(get=get_SourceToBeDeletedReplica)) PlacementReplica const* SourceToBeDeletedReplica;
            PlacementReplica const* get_SourceToBeDeletedReplica() const
            { return (!sourceOrNewReplica_ || sourceOrNewReplica_->IsNew) ? nullptr: sourceOrNewReplica_; }

            __declspec (property(get=get_TargetToBeDeletedReplica)) PlacementReplica const* TargetToBeDeletedReplica;
            PlacementReplica const* get_TargetToBeDeletedReplica() const { return targetReplica_; }

            __declspec (property(get=get_SourceToBeAddedReplica)) PlacementReplica const* SourceToBeAddedReplica;
            PlacementReplica const* get_SourceToBeAddedReplica() const { return targetReplica_; }

            __declspec (property(get=get_TargetToBeAddedReplica)) PlacementReplica const* TargetToBeAddedReplica;
            PlacementReplica const* get_TargetToBeAddedReplica() const { return sourceOrNewReplica_; }

            __declspec (property(get=get_SourceNode)) NodeEntry const* SourceNode;
            NodeEntry const* get_SourceNode() const { return sourceNode_; }

            __declspec (property(get=get_TargetNode)) NodeEntry const* TargetNode;
            NodeEntry const* get_TargetNode() const { return targetNode_; }

            // the role of existing replica on source node
            __declspec (property(get=get_SourceRole)) ReplicaRole::Enum SourceRole;
            ReplicaRole::Enum get_SourceRole() const
            { return (!sourceOrNewReplica_ || sourceOrNewReplica_->IsNew) ? ReplicaRole::None : sourceOrNewReplica_->Role; }

            // the role of existing replica on target node
            __declspec (property(get=get_TargetRole)) ReplicaRole::Enum TargetRole;
            ReplicaRole::Enum get_TargetRole() const { return targetReplica_ != nullptr ? targetReplica_->Role : ReplicaRole::None; }

            __declspec (property(get=get_Type)) Type::Enum MoveType;
            Type::Enum get_Type() const { return type_; }

            __declspec (property(get=get_IsSwap)) bool IsSwap;
            bool get_IsSwap() const { return type_ == Type::Swap; }

            __declspec (property(get=get_IsMove)) bool IsMove;
            bool get_IsMove() const { return type_ == Type::Move; }

            __declspec (property(get=get_IsAdd)) bool IsAdd;
            bool get_IsAdd() const { return type_ == Type::Add; }

            __declspec (property(get=get_IsVoid)) bool IsVoid;
            bool get_IsVoid() const { return type_ == Type::Void; }

            __declspec (property(get=get_IsPromote)) bool IsPromote;
            bool get_IsPromote() const { return type_ == Type::Promote;}

            __declspec (property(get=get_IsAddAndPromote)) bool IsAddAndPromote;
            bool get_IsAddAndPromote() const { return type_ == Type::AddAndPromote;}

            __declspec (property(get=get_IsDrop)) bool IsDrop;
            bool get_IsDrop() const { return type_ == Type::Drop; }

            __declspec (property(get=get_IsValid)) bool IsValid;
            bool get_IsValid() const { return partition_ != nullptr; }

            __declspec (property(get = get_IncreasingTargetLoad)) bool IncreasingTargetLoad;
            bool get_IncreasingTargetLoad() const { return IsMove || IsAdd || IsAddAndPromote; }

            __declspec (property(get = get_TargetNodeHasLoadBeforeMove)) bool TargetNodeHasLoadBeforeMove;
            bool get_TargetNodeHasLoadBeforeMove() const { return IsSwap || IsPromote; }

            bool operator == ( Movement const& other ) const;
            bool operator != ( Movement const& other ) const;

            void ForEachReplica(std::function<void(PlacementReplica const *)> processor) const;

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

        private:

            PartitionEntry const* partition_;
            PlacementReplica const* sourceOrNewReplica_; // existing replica on source node or the new replica
            PlacementReplica const* targetReplica_; // existing replica on target node
            NodeEntry const* sourceNode_;
            NodeEntry const* targetNode_;
            Type::Enum type_;
        };

        void WriteToTextWriter(Common::TextWriter & w, Movement::Type::Enum const & val);
    }
}
