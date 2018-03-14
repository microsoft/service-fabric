// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

ReplicaHealthId::ReplicaHealthId()
    : partitionId_(Guid::NewGuid())
    , replicaId_(FABRIC_INVALID_REPLICA_ID)
{
}

ReplicaHealthId::ReplicaHealthId(
    PartitionHealthId partitionId,
    FABRIC_REPLICA_ID replicaId)
    : partitionId_(partitionId)
    , replicaId_(replicaId)
{
}

ReplicaHealthId::ReplicaHealthId(ReplicaHealthId const & other)
    : partitionId_(other.partitionId_)
    , replicaId_(other.replicaId_) 
{
}

ReplicaHealthId & ReplicaHealthId::operator = (ReplicaHealthId const & other)
{
    if (this != &other)
    {
        partitionId_ = other.partitionId_;
        replicaId_ = other.replicaId_;
    }

    return *this;
}

ReplicaHealthId::ReplicaHealthId(ReplicaHealthId && other)
    : partitionId_(move(other.partitionId_))
    , replicaId_(move(other.replicaId_))
{
}

ReplicaHealthId & ReplicaHealthId::operator = (ReplicaHealthId && other)
{
    if (this != &other)
    {
        partitionId_ = move(other.partitionId_);
        replicaId_ = move(other.replicaId_);
     }

    return *this;
}

ReplicaHealthId::~ReplicaHealthId()
{
}

bool ReplicaHealthId::operator == (ReplicaHealthId const & other) const
{
    return replicaId_ == other.replicaId_ &&
           partitionId_ == other.partitionId_;
}

bool ReplicaHealthId::operator != (ReplicaHealthId const & other) const
{
    return !(*this == other);
}

bool ReplicaHealthId::operator < (ReplicaHealthId const & other) const
{
    if (partitionId_ < other.partitionId_)
    {
        return true;
    }
    else if (partitionId_ == other.partitionId_)
    {
        return replicaId_ < other.replicaId_;
    }
    else
    {
        return false;
    }
}

void ReplicaHealthId::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "{0}{1}{2}",
        partitionId_,
        Constants::TokenDelimeter,
        replicaId_);
}

std::wstring ReplicaHealthId::ToString() const
{
    return wformatString(*this);
}

bool ReplicaHealthId::TryParse(
    std::wstring const & entityIdStr,
    __inout PartitionHealthId & partitionId,
    __inout FABRIC_REPLICA_ID & replicaId)
{
    std::vector<wstring> tokens;
    StringUtility::Split<wstring>(entityIdStr, tokens, Constants::TokenDelimeter);
    if (tokens.size() != 2)
    {
        return false;
    }

    partitionId = Common::Guid(tokens[0]);
    if (!TryParseInt64(tokens[1], replicaId))
    {
        return false;
    }
            
    return true;
}
