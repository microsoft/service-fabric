// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::HealthManager;

SequenceStreamId::SequenceStreamId()
    : kind_(FABRIC_HEALTH_REPORT_KIND_INVALID)
    , sourceId_()
{
}

SequenceStreamId::SequenceStreamId(
    FABRIC_HEALTH_REPORT_KIND kind,
    std::wstring const & sourceId)
    : kind_(kind)
    , sourceId_(sourceId)
{
}

SequenceStreamId::SequenceStreamId(SequenceStreamId const & other)
    : kind_(other.kind_)
    , sourceId_(other.sourceId_)
{
}

SequenceStreamId & SequenceStreamId::operator = (SequenceStreamId const & other)
{
    if (this != &other)
    {
        kind_ = other.kind_;
        sourceId_ = other.sourceId_;
    }

    return *this;
}

SequenceStreamId::SequenceStreamId(SequenceStreamId && other)
    : kind_(move(other.kind_))
    , sourceId_(move(other.sourceId_))
{
}

SequenceStreamId & SequenceStreamId::operator = (SequenceStreamId && other)
{
    if (this != &other)
    {
        kind_ = move(other.kind_);
        sourceId_ = move(other.sourceId_);
    }

    return *this;
}

bool SequenceStreamId::operator == (SequenceStreamId const & other) const
{
    return kind_ == other.kind_ && sourceId_ == other.sourceId_;
}

bool SequenceStreamId::operator != (SequenceStreamId const & other) const
{
    return !(*this == other);
}

bool SequenceStreamId::operator < (SequenceStreamId const & other) const
{
    // TODO: Temporarily reverse ordering in feature_data2 branch for density testing
    //       so that service health reports are sent/processed before partition
    //       health reports. This is to allow querying the progress of large
    //       service creation (large # partitions within a service).
    //
    if (kind_ > other.kind_)
    {
        return true;
    }
    else if (kind_ == other.kind_)
    {
        return sourceId_ < other.sourceId_;
    }
    else
    {
        return false;
    }
}

void SequenceStreamId::WriteTo(__in Common::TextWriter& writer, FormatOptions const &) const
{
    writer.Write(
        "{0}+{1}", 
        kind_,
        sourceId_);
}

void SequenceStreamId::WriteToEtw(uint16 contextSequenceId) const
{
    ServiceModelEventSource::Trace->SequenceStreamIdTrace(
        contextSequenceId,
        wformatString(kind_),
        sourceId_);
}

std::wstring SequenceStreamId::ToString() const
{
    return wformatString(*this);
}
