// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::RepairManager;

StringLiteral const TraceComponent("HealthCheckStoreData");

HealthCheckStoreData::HealthCheckStoreData()
    : StoreData()
    , lastErrorAt_(Common::DateTime::Now())
    , lastHealthyAt_(Common::DateTime::Zero)
{
}

HealthCheckStoreData::HealthCheckStoreData(HealthCheckStoreData && other)
    : StoreData(move(other))
    , lastErrorAt_(other.lastErrorAt_)
    , lastHealthyAt_(other.lastHealthyAt_)
{
}

HealthCheckStoreData const & HealthCheckStoreData::operator = (HealthCheckStoreData const & other)
{
    if (this == &other) { return *this; }

    StoreData::operator = (other);

    lastErrorAt_ = other.lastErrorAt_;
    lastHealthyAt_ = other.lastHealthyAt_;

    return *this;
}

HealthCheckStoreData::~HealthCheckStoreData()
{
}

std::wstring const & HealthCheckStoreData::get_Type() const
{
    return Constants::StoreType_HealthCheckStoreData;
}

std::wstring HealthCheckStoreData::ConstructKey() const
{
    // No need of partition/replica specific info in the key. Each replica has a separate 
    // database, so we can simply store a constant key since it is unique for a replica.
    return Constants::StoreKey_HealthCheckStoreData;    
}

void HealthCheckStoreData::put_LastErrorAt(Common::DateTime const & newTime)
{
    lastErrorAt_ = newTime;
}

void HealthCheckStoreData::put_LastHealthyAt(Common::DateTime const & newTime)
{
    lastHealthyAt_ = newTime;
}

void HealthCheckStoreData::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("HealthCheckStoreData[LastErrorAt: {0}, LastHealthyAt: {1}]", lastErrorAt_, lastHealthyAt_);
}
