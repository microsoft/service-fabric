// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

const ReplicaDeactivationInfo ReplicaDeactivationInfo::Dropped = ReplicaDeactivationInfo::CreateDroppedDeactivationInfo();

ReplicaDeactivationInfo::ReplicaDeactivationInfo()
{
    Clear();
}

ReplicaDeactivationInfo::ReplicaDeactivationInfo(
    Reliability::Epoch const & deactivationEpoch,
    int64 catchupLSN)
    : deactivationEpoch_(deactivationEpoch),
    catchupLSN_(catchupLSN)
{
    ASSERT_IF(deactivationEpoch.IsInvalid(), "Cannot have invalid deactivation epoch");
    ASSERT_IF(catchupLSN == FABRIC_INVALID_SEQUENCE_NUMBER, "Cannot have invalid catchup LSN");
}

ReplicaDeactivationInfo::ReplicaDeactivationInfo(ReplicaDeactivationInfo && other)
    : deactivationEpoch_(std::move(other.deactivationEpoch_)),
    catchupLSN_(std::move(other.catchupLSN_))
{
}

ReplicaDeactivationInfo ReplicaDeactivationInfo::CreateDroppedDeactivationInfo()
{
    ReplicaDeactivationInfo rv;
    rv.catchupLSN_ = Constants::UnknownLSN;
    return rv;
}

ReplicaDeactivationInfo& ReplicaDeactivationInfo::operator = (ReplicaDeactivationInfo && other)
{
    if (this != &other)
    {
        deactivationEpoch_ = std::move(other.deactivationEpoch_);
        catchupLSN_ = std::move(other.catchupLSN_);
    }

    return *this;
}

void ReplicaDeactivationInfo::Clear()
{
    catchupLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;
    deactivationEpoch_ = Epoch::InvalidEpoch();
}

bool ReplicaDeactivationInfo::IsDeactivatedIn(Epoch const & other) const
{
    ASSERT_IF(!IsValid, "Must be valid");
    return deactivationEpoch_.ToPrimaryEpoch() == other.ToPrimaryEpoch();
}

bool ReplicaDeactivationInfo::operator<=(ReplicaDeactivationInfo const & other) const
{
    return (*this < other) || (*this == other);
}

bool ReplicaDeactivationInfo::operator<(ReplicaDeactivationInfo const & other) const
{
    auto primaryEpoch = deactivationEpoch_.ToPrimaryEpoch();
    auto otherPrimaryEpoch = other.deactivationEpoch_.ToPrimaryEpoch();

    if (primaryEpoch < otherPrimaryEpoch)
    {
        return true;
    }
    else if (primaryEpoch > otherPrimaryEpoch)
    {
        return false;
    }
    else
    {
        AssertCatchupLSNMustMatch(other.catchupLSN_);
        return false;
    }
}

bool ReplicaDeactivationInfo::operator>=(ReplicaDeactivationInfo const & other) const
{
    return (*this > other) || (*this == other);
}

bool ReplicaDeactivationInfo::operator>(ReplicaDeactivationInfo const & other) const
{
    auto primaryEpoch = deactivationEpoch_.ToPrimaryEpoch();
    auto otherPrimaryEpoch = other.deactivationEpoch_.ToPrimaryEpoch();

    if (primaryEpoch > otherPrimaryEpoch)
    {
        return true;
    }
    else if (primaryEpoch < otherPrimaryEpoch)
    {
        return false;
    }
    else
    {
        AssertCatchupLSNMustMatch(other.catchupLSN_);
        return false;
    }
}

bool ReplicaDeactivationInfo::operator==(ReplicaDeactivationInfo const & other) const
{
    auto primaryEpoch = deactivationEpoch_.ToPrimaryEpoch();
    auto otherPrimaryEpoch = other.deactivationEpoch_.ToPrimaryEpoch();

    if (primaryEpoch == otherPrimaryEpoch)
    {
        if (primaryEpoch.IsInvalid())
        {
            return catchupLSN_ == other.catchupLSN_;
        }

        AssertCatchupLSNMustMatch(other.catchupLSN_);
        return true;
    }
    else
    {
        return false;
    }
}

bool ReplicaDeactivationInfo::operator!=(ReplicaDeactivationInfo const & other) const
{
    return !(*this == other);
}

string ReplicaDeactivationInfo::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "{0}:{1}";
    size_t index = 0;

    traceEvent.AddEventField<Reliability::Epoch>(format, name + ".deactivationEpoch", index);
    traceEvent.AddEventField<int64>(format, name + ".catchupLSN", index);

    return format;
}

void ReplicaDeactivationInfo::FillEventData(Common::TraceEventContext & context) const
{
    context.Write(deactivationEpoch_);
    context.Write(catchupLSN_);
}

void ReplicaDeactivationInfo::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer << deactivationEpoch_ << ":" << catchupLSN_;
}

void ReplicaDeactivationInfo::AssertCatchupLSNMustMatch(int64 otherCatchupLSN) const
{
    TESTASSERT_IF(catchupLSN_ != otherCatchupLSN && deactivationEpoch_.IsValid(), "Catchup LSN must match for same deactivation epoch");
}
