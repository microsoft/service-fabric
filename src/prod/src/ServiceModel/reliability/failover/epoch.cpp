// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability;
    
Epoch const Epoch::invalidEpoch_;
int64 const Epoch::PrimaryEpochMask = 0xFFFFFFFF;

::FABRIC_EPOCH Epoch::ToPublic() const
{
    ::FABRIC_EPOCH toReturn;

    toReturn.ConfigurationNumber = ConfigurationVersion;
    toReturn.DataLossNumber = DataLossVersion;
    toReturn.Reserved = NULL;

    return toReturn;
}

ErrorCode Epoch::FromPublicApi(FABRIC_EPOCH const& fabricEpoch)
{
    ConfigurationVersion = fabricEpoch.ConfigurationNumber;
    DataLossVersion = fabricEpoch.DataLossNumber;
    return ErrorCode::Success();
}

Epoch Epoch::ToPrimaryEpoch() const
{
    return Epoch(DataLossVersion, ConfigurationVersion & ~PrimaryEpochMask);
}

bool Epoch::IsPrimaryEpochEqual(Epoch const & other) const
{
    return ToPrimaryEpoch() == other.ToPrimaryEpoch();
}

void Epoch::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("{0}:{1:x}", dataLossVersion_, configurationVersion_);
}

std::string Epoch::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    traceEvent.AddField<int64>(name + ".dataLossVersion");
    traceEvent.AddField<int64>(name + ".configVersion");
            
    return "{0}:{1:x}";
}

void Epoch::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<int64>(dataLossVersion_);
    context.Write<int64>(configurationVersion_);
}

bool Epoch::operator==(Epoch const &other) const
{
    CheckEpoch(*this);
    CheckEpoch(other);

    return 
        this->ConfigurationVersion == other.ConfigurationVersion &&
        this->DataLossVersion == other.DataLossVersion;
}

bool Epoch::IsInvalid() const
{
    return *this == Epoch::InvalidEpoch();
}

bool Epoch::IsValid() const
{
    return !IsInvalid();
}

bool Epoch::operator>(Epoch const &other) const
{
    CheckEpoch(*this);
    CheckEpoch(other);

    return 
        this->DataLossVersion > other.DataLossVersion || 
        (
            (this->DataLossVersion == other.DataLossVersion) &&
            (this->ConfigurationVersion > other.ConfigurationVersion)
        );
}

bool Epoch::operator>=(Epoch const &other) const
{
    return (*this > other) || (*this == other);
}

bool Epoch::operator<(Epoch const &other) const
{
    CheckEpoch(*this);
    CheckEpoch(other);

    return 
        this->DataLossVersion < other.DataLossVersion || 
        (
            (this->DataLossVersion == other.DataLossVersion) &&
            (this->ConfigurationVersion < other.ConfigurationVersion)
        );
}

bool Epoch::operator<=(Epoch const &other) const
{
    return (*this < other) || (*this == other);
}

bool Epoch::operator!=(Epoch const &other) const
{
    return !(*this == other);
}

void Epoch::CheckEpoch(Epoch const &toCheck)
{
    ASSERT_IF(toCheck.ConfigurationVersion == 0 && toCheck.DataLossVersion != 0, "Epoch with an invalid configuration version cannot have non-zero data loss version value.");
}
