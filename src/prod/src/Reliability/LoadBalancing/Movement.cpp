// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PartitionEntry.h"
#include "NodeEntry.h"
#include "ServiceEntry.h"
#include "LoadBalancingDomainEntry.h"
#include "PlacementReplica.h"
#include "Movement.h"
#include "SearcherSettings.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

void Reliability::LoadBalancingComponent::WriteToTextWriter(Common::TextWriter & w, Movement::Type::Enum const & val)
{
    switch (val)
    {
        case Movement::Type::None:
            w << "None"; return;
        case Movement::Type::Swap:
            w << "Swap"; return;
        case Movement::Type::Move:
            w << "Move"; return;
        case Movement::Type::Add:
            w << "Add"; return;
        case Movement::Type::Promote:
            w << "Promote"; return;
        case Movement::Type::AddAndPromote:
            w << "AddAndPromote"; return;
        case Movement::Type::Void:
            w << "Void"; return;
        case Movement::Type::Drop:
            w << "Drop"; return;
        default:
            Common::Assert::CodingError("Unknown Movement Type {0}", val);
    }
}

Movement Movement::Invalid = Movement();

Movement Movement::Create(PlacementReplica const* r1, PlacementReplica const* r2, bool forUpgrade)
{
    Movement newMovement;

    ASSERT_IF(r1 == nullptr || r2 == nullptr || r1->IsNew || r2->IsNew || r1 == r2 || r1->Partition != r2->Partition || r1->Role == r2->Role, "Invalid input for this movement");
    ASSERT_IF((!r1->IsMovable || !r2->IsMovable) && !forUpgrade, "Invalid input for this movement since replica {0} or {1} is not movable", r1, r2);

    newMovement.partition_ = r1->Partition;
    newMovement.sourceOrNewReplica_ = r1;
    newMovement.targetReplica_ = r2;
    newMovement.sourceNode_ = r1->Node;
    newMovement.targetNode_ = r2->Node;

    newMovement.type_ = Type::Swap;

    return newMovement;
}

Movement Movement::Create(PlacementReplica const* replica, NodeEntry const* targetNode, bool forUpgrade)
{
    Movement newMovement;

    ASSERT_IF(replica == nullptr || targetNode == nullptr, "Invalid input for this movement");

    if (replica->IsNew)
    {
        PartitionEntry const* partition = replica->Partition;

        newMovement.partition_ = partition;
        newMovement.sourceOrNewReplica_ = replica;
        newMovement.targetReplica_ = nullptr;
        newMovement.sourceNode_ = nullptr;
        newMovement.targetNode_ = targetNode;

        ASSERT_IFNOT(replica->Role != ReplicaRole::None, "Invalid replica role for new replica {0}", replica->Role);
        newMovement.type_ = Type::Add;
    }
    else
    {
        ASSERT_IF((!replica->IsMovable || !replica->Partition->IsMovable) && !forUpgrade, "Invalid input for this movement since replica {0} or the related partition is not movable", replica);
        ASSERT_IF(replica->Node == targetNode, "Invalid input for this movement");
        ASSERT_IFNOT(replica->Role != ReplicaRole::None, "Invalid replica role for replica movement {0}", replica->Role);

        PartitionEntry const* partition = replica->Partition;

        newMovement.partition_ = partition;
        newMovement.sourceOrNewReplica_ = replica;
        newMovement.targetReplica_ = nullptr;
        newMovement.sourceNode_ = replica->Node;
        newMovement.targetNode_ = targetNode;

        // a failover unit with None replica should not be balanced
        newMovement.type_ = Type::Move;
    }

    return newMovement;
}

Movement Movement::CreatePromoteSecondaryMovement(PartitionEntry const* partition,
    NodeEntry const* targetNode,
    PlacementReplica const* targetReplica,
    bool isTestMode)
{
    Movement newMovement;
    ASSERT_IF(partition == nullptr, "Invalid input for CreatePromoteSecondaryMovement");
    PlacementReplica const* primary = partition->PrimaryReplica;

    newMovement.partition_ = partition;
    newMovement.sourceOrNewReplica_ = primary;
    newMovement.targetReplica_ = targetReplica;
    if (isTestMode)
    {
        // if there is a replica on the target node, it must be secondary
        ASSERT_IF(newMovement.targetReplica_ && !newMovement.targetReplica_->IsSecondary, "Target replica is not secondary for Promote movement");
    }

    newMovement.sourceNode_ = primary ? primary->Node: nullptr;
    newMovement.targetNode_ = targetNode;

    if (newMovement.sourceOrNewReplica_ && newMovement.sourceNode_)
    {
        newMovement.type_ = Type::Swap;
    }
    else
    {
        newMovement.type_ = Type::Promote;
    }

    return newMovement;
}

Movement Movement::CreateMigratePrimaryMovement(PartitionEntry const* partition, NodeEntry const* targetNode, bool isTestMode)
{
    ASSERT_IF(partition == nullptr, "Invalid input partition for MigratePrimary");
    ASSERT_IF(targetNode == nullptr, "Invalid input node for MigratePrimary");

    Movement newMovement;
    PlacementReplica const* primary = partition->PrimaryReplica;

    newMovement.partition_ = partition;
    newMovement.sourceOrNewReplica_ = primary;
    newMovement.targetReplica_ = partition->GetReplica(targetNode);

    if (isTestMode)
    {
        // if there is a replica on the target node, it must be secondary
        ASSERT_IF(newMovement.targetReplica_ && !(newMovement.targetReplica_->IsSecondary || newMovement.targetReplica_->IsStandBy), "Target replica exists and is not secondary/standby for Migrate Primary movement");
    }

    newMovement.sourceNode_ = primary ? primary->Node: nullptr;
    newMovement.targetNode_ = targetNode;

    if (newMovement.targetReplica_ == nullptr)
    {
        if (newMovement.sourceNode_ == nullptr)
        {
            newMovement.type_ = Type::Add;
        }
        else
        {
            newMovement.type_ = Type::Move;
        }
    }
    else
    {
        if (newMovement.targetReplica_ && newMovement.sourceNode_)
        {
            // If Target ReplicaRole is None this runs into an assert eventually when
            // PartitionDomainStructure::ChangeMovement is called

            newMovement.type_ = (ReplicaRole::None == newMovement.targetReplica_->Role) ? Type::Move : Type::Swap;

        }
        else
        {
            newMovement.type_ = Type::Promote;
        }
    }

    return newMovement;
}

Movement Movement::CreateAddAndPromoteMovement(PlacementReplica const* replica, NodeEntry const* targetNode)
{
    Movement newMovement;

    ASSERT_IFNOT(replica->IsNew, "Invalid replica {0} for this CreateAddAndPromoteMovement.", *replica);

    newMovement.partition_ = replica->Partition;
    newMovement.sourceOrNewReplica_ = replica;
    newMovement.targetReplica_ = nullptr;
    newMovement.sourceNode_ = nullptr;
    newMovement.targetNode_ = targetNode;

    newMovement.type_ = Type::AddAndPromote;

    return newMovement;
}

Movement Movement::CreateVoidMovement(PartitionEntry const* partition, NodeEntry const* sourceNode)
{
    Movement newMovement;

    ASSERT_IF(partition == nullptr, "Invalid input for CreateCancelMovement");

    newMovement.partition_ = partition;
    newMovement.sourceOrNewReplica_ = nullptr;
    newMovement.targetReplica_ = nullptr;
    newMovement.sourceNode_ = sourceNode;
    newMovement.targetNode_ = nullptr;

    newMovement.type_ = Type::Void;

    return newMovement;
}

Movement Movement::CreateDrop(PlacementReplica const* replica, NodeEntry const* sourceNode)
{
    Movement newMovement;

    ASSERT_IF(replica == nullptr || sourceNode == nullptr, "Invalid input for this movement");
    ASSERT_IF(replica->Role == ReplicaRole::Primary, "Invalid drop of primary replica");

    newMovement.partition_ = replica->Partition;
    newMovement.sourceOrNewReplica_ = replica;
    newMovement.sourceNode_ = sourceNode;
    newMovement.targetReplica_ = nullptr;
    newMovement.targetNode_ = nullptr;
    newMovement.type_ = Type::Drop;

    return newMovement;
}

Movement::Movement()
    : partition_(nullptr),
    sourceOrNewReplica_(nullptr),
    targetReplica_(nullptr),
    sourceNode_(nullptr),
    targetNode_(nullptr),
    type_(Type::None)
{
}

bool Movement::operator == (Movement const & other ) const
{
    return (type_ == other.type_ && partition_ == other.partition_ && sourceOrNewReplica_ == other.sourceOrNewReplica_ && targetNode_ == other.targetNode_);
}

bool Movement::operator != (Movement const & other ) const
{
    return !(*this == other);
}

void Movement::ForEachReplica(std::function<void(PlacementReplica const *)> processor) const
{
    if (sourceOrNewReplica_ != nullptr)
    {
        processor(sourceOrNewReplica_);
    }

    if (targetReplica_ != nullptr)
    {
        processor(targetReplica_);
    }
}


void Movement::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    if (IsValid)
    {
        writer.Write("{0} {1} {2} {3}", type_, *partition_, *sourceNode_, *targetNode_);
    }
    else
    {
        writer.Write("Invalid");
    }
}
