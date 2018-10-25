// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

using namespace Store;

//
// PartitionInitData
//

MigrationInitData::PartitionInitData::PartitionInitData()
    : TargetPhase(MigrationPhase::Inactive)
{
}

void MigrationInitData::PartitionInitData::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << "[phase=" << TargetPhase << "]";
}

//
// MigrationInitData
//

MigrationInitData::MigrationInitData()
    : targetReplicaSetSize_(0)
    , minReplicaSetSize_(0)
    , partitionPhaseMap_()
    , settings_()
{ 
}

ErrorCode MigrationInitData::FromBytes(vector<byte> const & buffer)
{
    return FabricSerializer::Deserialize(*this, const_cast<vector<byte>&>(buffer));
}

ErrorCode MigrationInitData::ToBytes(__out std::vector<byte> & buffer)
{
    return FabricSerializer::Serialize(this, buffer);
}

void MigrationInitData::Initialize(MigrationSettings const & settings)
{
    settings_ = make_unique<MigrationSettings>(settings);
}

void MigrationInitData::Initialize(int targetReplicaSetSize, int minReplicaSetSize) 
{
    targetReplicaSetSize_ = targetReplicaSetSize;
    minReplicaSetSize_ = minReplicaSetSize;
}

void MigrationInitData::SetPhase(Common::Guid partitionId, MigrationPhase::Enum phase)
{
    auto initData = make_shared<PartitionInitData>();
    initData->TargetPhase = phase;
    partitionPhaseMap_[partitionId] = move(initData);
}

MigrationPhase::Enum MigrationInitData::GetPhase(Common::Guid partitionId)
{
    auto findIt = partitionPhaseMap_.find(partitionId);
    if (findIt == partitionPhaseMap_.end())
    {
        return MigrationPhase::Inactive;
    }
    else
    {
        return findIt->second->TargetPhase;
    }
}

void MigrationInitData::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << "replicas(target/min)=" << targetReplicaSetSize_ << "/" << minReplicaSetSize_;
    w << " phases=" << partitionPhaseMap_;

    if (settings_)
    {
        w << " settings=[" << *settings_ << "]";
    }
}
