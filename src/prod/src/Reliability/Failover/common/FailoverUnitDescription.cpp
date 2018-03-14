// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;
using namespace Reliability;

FailoverUnitDescription::FailoverUnitDescription()
    : ccEpoch_(),
      pcEpoch_(),
      targetReplicaSetSize_(0),
      minReplicaSetSize_(0)      
{
}

FailoverUnitDescription::FailoverUnitDescription(
    Reliability::FailoverUnitId failoverUnitId,
    Reliability::ConsistencyUnitDescription const& consistencyUnitDescription,
    Reliability::Epoch ccEpoch, 
    Reliability::Epoch pcEpoch)
    : failoverUnitId_(failoverUnitId),
      consistencyUnitDescription_(consistencyUnitDescription),
      ccEpoch_(ccEpoch), 
      pcEpoch_(pcEpoch),
      targetReplicaSetSize_(0),
      minReplicaSetSize_(0)
{
}

FailoverUnitDescription & FailoverUnitDescription::operator = (FailoverUnitDescription const& other)
{
    if (this != &other) 
    {
        failoverUnitId_ = other.failoverUnitId_;
        consistencyUnitDescription_ = other.consistencyUnitDescription_;
        ccEpoch_ = other.ccEpoch_;
        pcEpoch_ = other.pcEpoch_;
        targetReplicaSetSize_ = other.targetReplicaSetSize_;
        minReplicaSetSize_ = other.minReplicaSetSize_;
    }

    return *this;
}

bool FailoverUnitDescription::IsReservedCUID() const
{
    return ConsistencyUnitId::IsReserved(consistencyUnitDescription_.ConsistencyUnitId.Guid);
}

void FailoverUnitDescription::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{ 
    w.Write(
        "{0} {1}/{2}",
        failoverUnitId_, 
        pcEpoch_,
        ccEpoch_);
}

std::string FailoverUnitDescription::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    std::string format = "{0} {1}/{2}";

    size_t index = 0;
    traceEvent.AddEventField<Guid>(format, name + ".id", index);
    traceEvent.AddEventField<Epoch>(format, name + ".pcEpoch", index);
    traceEvent.AddEventField<Epoch>(format, name + ".ccEpoch", index);
            
    return format;
}

void FailoverUnitDescription::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<Guid>(failoverUnitId_.Guid);
    context.Write<Epoch>(pcEpoch_);
    context.Write<Epoch>(ccEpoch_);
}
