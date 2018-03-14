// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;
    
Epoch const Epoch::InvalidEpochValue = Epoch(-1,-1);
Epoch const Epoch::ZeroEpochValue = Epoch(0,0);

Epoch::Epoch() 
    : configurationVersion_(0)
    , dataLossVersion_(0)
{
}

Epoch::Epoch(
    __in LONG64 dataLossVersion,
    __in LONG64 configurationVersion)
    : dataLossVersion_(dataLossVersion)
    , configurationVersion_(configurationVersion)
{
}

Epoch::Epoch(
    __in FABRIC_EPOCH epoch)
    : dataLossVersion_(epoch.DataLossNumber)
    , configurationVersion_(epoch.ConfigurationNumber)
{
}

Epoch const & Epoch::InvalidEpoch() 
{ 
    return InvalidEpochValue;
}

Epoch const & Epoch::ZeroEpoch()
{
    return ZeroEpochValue;
}

LONG64 const Epoch::get_ConfigurationVersion() const 
{ 
    return configurationVersion_; 
}

void Epoch::set_ConfigurationVersion(LONG64 value) 
{ 
    configurationVersion_ = value; 
}

LONG64 const Epoch::get_DataLossVersion() const 
{ 
    return dataLossVersion_; 
}

void Epoch::set_DataLossVersion(LONG64 value) 
{ 
    dataLossVersion_ = value; 
}

void Epoch::WriteTo(
    __in Common::TextWriter& w, 
    __in Common::FormatOptions const &) const
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

FABRIC_EPOCH Epoch::GetFabricEpoch() const
{
    FABRIC_EPOCH epoch;
    epoch.DataLossNumber = dataLossVersion_;
    epoch.ConfigurationNumber = configurationVersion_;
    epoch.Reserved = nullptr;

    return epoch;
}

bool Epoch::operator==(Epoch const &other) const
{
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
