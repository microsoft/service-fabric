// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

SequenceStreamStoreData::SequenceStreamStoreData()
    : StoreData()
    , sourceId_()
    , instance_(0)
    , upToLsn_(0)
    , invalidateLsn_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , invalidateTime_(DateTime::MaxValue)
    , highestLsn_(0)
{
}

SequenceStreamStoreData::SequenceStreamStoreData(
    std::wstring const & sourceId,
    FABRIC_INSTANCE_ID instance,
    FABRIC_SEQUENCE_NUMBER upToLsn,
    FABRIC_SEQUENCE_NUMBER invalidateLsn,
    DateTime invalidateTime,
    FABRIC_SEQUENCE_NUMBER highestLsn,
    Store::ReplicaActivityId const & activityId)
    : StoreData(activityId)
    , sourceId_(sourceId)
    , instance_(instance)
    , upToLsn_(upToLsn)
    , invalidateLsn_(invalidateLsn)
    , invalidateTime_(invalidateTime)
    , highestLsn_(highestLsn)
{
}

SequenceStreamStoreData::SequenceStreamStoreData(SequenceStreamStoreData const & other)
    : StoreData(other)
    , sourceId_(other.sourceId_)
    , instance_(other.instance_)
    , upToLsn_(other.upToLsn_)
    , invalidateLsn_(other.invalidateLsn_)
    , invalidateTime_(other.invalidateTime_)
    , highestLsn_(other.highestLsn_)
{
}

SequenceStreamStoreData & SequenceStreamStoreData::operator = (SequenceStreamStoreData const & other)
{
    if (this != &other)
    {
        sourceId_ = other.sourceId_;
        instance_ = other.instance_;
        upToLsn_ = other.upToLsn_;
        invalidateLsn_ = other.invalidateLsn_;
        invalidateTime_ = other.invalidateTime_;
        highestLsn_ = other.highestLsn_;
    }

    StoreData::operator=(other);
    return *this;
}

SequenceStreamStoreData::SequenceStreamStoreData(SequenceStreamStoreData && other)
    : StoreData(move(other))
    , sourceId_(move(other.sourceId_))
    , instance_(other.instance_)
    , upToLsn_(move(other.upToLsn_))
    , invalidateLsn_(move(other.invalidateLsn_))
    , invalidateTime_(move(other.invalidateTime_))
    , highestLsn_(other.highestLsn_)
{
}

SequenceStreamStoreData & SequenceStreamStoreData::operator = (SequenceStreamStoreData && other)
{
    if (this != &other)
    {
        sourceId_ = move(other.sourceId_);
        instance_ = other.instance_;
        upToLsn_ = move(other.upToLsn_);
        invalidateLsn_ = other.invalidateLsn_;
        invalidateTime_ = other.invalidateTime_;
        highestLsn_ = other.highestLsn_;
    }

    StoreData::operator=(move(other));
    return *this;
}

SequenceStreamStoreData::~SequenceStreamStoreData()
{
}

void SequenceStreamStoreData::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "{0}({1}:{2}=[0,{3})) invalidate: {4}@{5}, highest: {6}",
        this->Type,
        this->Key,
        instance_,
        upToLsn_,
        invalidateLsn_,
        invalidateTime_,
        highestLsn_);
}

void SequenceStreamStoreData::WriteToEtw(uint16 contextSequenceId) const
{
    HMCommonEvents::Trace->SequenceStreamStoreDataTrace(
        contextSequenceId,
        this->Type,
        this->Key,
        instance_,
        upToLsn_,
        invalidateLsn_,
        invalidateTime_,
        highestLsn_);
}
