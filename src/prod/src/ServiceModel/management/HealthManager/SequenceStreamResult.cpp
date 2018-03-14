// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::HealthManager;

SequenceStreamResult::SequenceStreamResult()
    : kind_(FABRIC_HEALTH_REPORT_KIND_INVALID)
    , sourceId_()
    , instance_(0)
    , upToLsn_(0)
{
}

SequenceStreamResult::SequenceStreamResult(
    FABRIC_HEALTH_REPORT_KIND kind,
    std::wstring const & sourceId)
    : kind_(kind)
    , sourceId_(sourceId)
    , instance_(0)
    , upToLsn_(0)
{
}

SequenceStreamResult::SequenceStreamResult(
    FABRIC_HEALTH_REPORT_KIND kind,
    std::wstring const & sourceId,
    FABRIC_INSTANCE_ID instance,
    FABRIC_SEQUENCE_NUMBER upToLsn)
    : kind_(kind)
    , sourceId_(sourceId)
    , instance_(instance)
    , upToLsn_(upToLsn)
{
}

SequenceStreamResult::SequenceStreamResult(
    FABRIC_HEALTH_REPORT_KIND kind,
    std::wstring && sourceId,
    FABRIC_INSTANCE_ID instance,
    FABRIC_SEQUENCE_NUMBER upToLsn)
    : kind_(kind)
    , sourceId_(move(sourceId))
    , instance_(instance)
    , upToLsn_(upToLsn)
{
}

SequenceStreamResult::SequenceStreamResult(SequenceStreamResult const & other)
    : kind_(other.kind_)
    , sourceId_(other.sourceId_)
    , instance_(other.instance_)
    , upToLsn_(other.upToLsn_)
{
}

SequenceStreamResult & SequenceStreamResult::operator = (SequenceStreamResult const & other)
{
    if (this != &other)
    {
        kind_ = other.kind_;
        sourceId_ = other.sourceId_;
        instance_ = other.instance_;
        upToLsn_ = other.upToLsn_;
    }

    return *this;
}

SequenceStreamResult::SequenceStreamResult(SequenceStreamResult && other)
    : kind_(move(other.kind_))
    , sourceId_(move(other.sourceId_))
    , instance_(move(other.instance_))
    , upToLsn_(move(other.upToLsn_))
{
}

SequenceStreamResult & SequenceStreamResult::operator = (SequenceStreamResult && other)
{
    if (this != &other)
    {
        kind_ = move(other.kind_);
        sourceId_ = move(other.sourceId_);
        instance_ = move(other.instance_);
        upToLsn_ = move(other.upToLsn_);
    }

    return *this;
}

void SequenceStreamResult::WriteTo(__in Common::TextWriter& writer, FormatOptions const &) const
{
    writer.Write(
        "SSResult({0}+{1}:{2} upto={3})", 
        kind_,
        sourceId_,
        instance_,
        upToLsn_);
}
