// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::HealthManager;

INITIALIZE_SIZE_ESTIMATION(SequenceStreamInformation)

SequenceStreamInformation::SequenceStreamInformation()
    : kind_(FABRIC_HEALTH_REPORT_KIND_INVALID)
    , sourceId_()
    , instance_(0)
    , fromLsn_(0)
    , upToLsn_(0)
    , reportCount_(0)
    , invalidateLsn_(FABRIC_INVALID_SEQUENCE_NUMBER)
{
}

SequenceStreamInformation::SequenceStreamInformation(
    FABRIC_HEALTH_REPORT_KIND kind,
    std::wstring const & sourceId,
    FABRIC_INSTANCE_ID instance,
    FABRIC_SEQUENCE_NUMBER fromLsn,
    FABRIC_SEQUENCE_NUMBER upToLsn,
    uint64 reportCount,
    FABRIC_SEQUENCE_NUMBER invalidateLsn)
    : kind_(kind)
    , sourceId_(sourceId)
    , instance_(instance)
    , fromLsn_(fromLsn)
    , upToLsn_(upToLsn)
    , reportCount_(reportCount)
    , invalidateLsn_(invalidateLsn)
{
    ASSERT_IF(fromLsn <= FABRIC_INVALID_SEQUENCE_NUMBER, "SequenceStreamInformation: fromLsn {0} is not valid", fromLsn);
    ASSERT_IF(fromLsn >= upToLsn, "SequenceStreamInformation: invalid relation between fromLsn {0} and upToLsn {1}", fromLsn, upToLsn);
}

SequenceStreamInformation::SequenceStreamInformation(
    FABRIC_HEALTH_REPORT_KIND kind,
    std::wstring const & sourceId,
    FABRIC_INSTANCE_ID instance)
    : kind_(kind)
    , sourceId_(sourceId)
    , instance_(instance)
    , fromLsn_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , upToLsn_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , reportCount_(0)
    , invalidateLsn_(FABRIC_INVALID_SEQUENCE_NUMBER)
{
}

SequenceStreamInformation::~SequenceStreamInformation()
{
}

SequenceStreamInformation::SequenceStreamInformation(SequenceStreamInformation const & other)
    : kind_(other.kind_)
    , sourceId_(other.sourceId_)
    , instance_(other.instance_)
    , fromLsn_(other.fromLsn_)
    , upToLsn_(other.upToLsn_)
    , reportCount_(other.reportCount_)
    , invalidateLsn_(other.invalidateLsn_)
{
}

SequenceStreamInformation & SequenceStreamInformation::operator = (SequenceStreamInformation const & other)
{
    if (this != &other)
    {
        kind_ = other.kind_;
        sourceId_ = other.sourceId_;
        instance_ = other.instance_;
        fromLsn_ = other.fromLsn_;
        upToLsn_ = other.upToLsn_;
        reportCount_ = other.reportCount_;
        invalidateLsn_ = other.invalidateLsn_;
    }

    return *this;
}

SequenceStreamInformation::SequenceStreamInformation(SequenceStreamInformation && other)
    : kind_(move(other.kind_))
    , sourceId_(move(other.sourceId_))
    , instance_(move(other.instance_))
    , fromLsn_(move(other.fromLsn_))
    , upToLsn_(move(other.upToLsn_))
    , reportCount_(move(other.reportCount_))
    , invalidateLsn_(move(other.invalidateLsn_))
{
}

SequenceStreamInformation & SequenceStreamInformation::operator = (SequenceStreamInformation && other)
{
    if (this != &other)
    {
        kind_ = move(other.kind_);
        sourceId_ = move(other.sourceId_);
        instance_ = move(other.instance_);
        fromLsn_ = move(other.fromLsn_);
        upToLsn_ = move(other.upToLsn_);
        reportCount_ = move(other.reportCount_);
        invalidateLsn_ = move(other.invalidateLsn_);
    }

    return *this;
}

bool SequenceStreamInformation::IsForInstanceUpdate() const
{
    return (upToLsn_ == FABRIC_INVALID_SEQUENCE_NUMBER);
}

void SequenceStreamInformation::WriteTo(__in Common::TextWriter& writer, FormatOptions const &) const
{
    writer.Write(
        "{0}+{1}:{2} [{3}, {4}): {5} reports, invalidate: {6}", 
        wformatString(kind_),
        sourceId_,
        instance_,
        fromLsn_,
        upToLsn_,
        reportCount_,
        invalidateLsn_);
}

std::wstring SequenceStreamInformation::ToString() const
{
    return wformatString(*this);
}
