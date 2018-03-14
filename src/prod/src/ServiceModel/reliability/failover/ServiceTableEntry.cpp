// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Transport;
using namespace std;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ServiceTableEntry)

ServiceTableEntry::ServiceTableEntry()
    : consistencyUnitId_(Common::Guid::Empty()), 
    serviceName_(), 
    serviceReplicaSet_(), 
    isFound_(false)
{
}

ServiceTableEntry::ServiceTableEntry(Reliability::ConsistencyUnitId consistencyUnitId)
    : consistencyUnitId_(consistencyUnitId), 
    serviceName_(), 
    serviceReplicaSet_(), 
    isFound_(false)
{
}

ServiceTableEntry::ServiceTableEntry(
    Reliability::ConsistencyUnitId consistencyUnitId,
    std::wstring const& serviceName,
    Reliability::ServiceReplicaSet && serviceReplicaSet,
    bool isFound)
    : consistencyUnitId_(consistencyUnitId),
    serviceName_(serviceName),
    serviceReplicaSet_(std::move(serviceReplicaSet)),
    isFound_(isFound)
{
}

ServiceTableEntry::ServiceTableEntry(ServiceTableEntry const & other)
    : consistencyUnitId_(other.consistencyUnitId_),
    serviceName_(other.serviceName_),
    serviceReplicaSet_(other.serviceReplicaSet_),
    isFound_(other.isFound_)
{
}

ServiceTableEntry & ServiceTableEntry::operator=(ServiceTableEntry const & other)
{
    if (this != &other)
    {
        consistencyUnitId_ = other.consistencyUnitId_;
        serviceName_ = other.serviceName_;
        serviceReplicaSet_ = other.serviceReplicaSet_;
        isFound_ = other.isFound_;
    }

    return *this;
}

ServiceTableEntry::ServiceTableEntry(ServiceTableEntry && other)
    : consistencyUnitId_(std::move(other.consistencyUnitId_)),
        serviceName_(std::move(other.serviceName_)),
        serviceReplicaSet_(std::move(other.serviceReplicaSet_)),
        isFound_(std::move(other.isFound_))
{
}

ServiceTableEntry & ServiceTableEntry::operator=(ServiceTableEntry && other)
{
    if (this != &other)
    {
        consistencyUnitId_ = std::move(other.consistencyUnitId_);
        serviceName_ = std::move(other.serviceName_);
        serviceReplicaSet_ = std::move(other.serviceReplicaSet_);
        isFound_ = std::move(other.isFound_);
    }

    return *this;
}

bool ServiceTableEntry::operator==(ServiceTableEntry const & other) const
{
    bool equals = true;

    equals = consistencyUnitId_ == other.consistencyUnitId_;
    if (!equals) { return equals; }

    equals = serviceName_ == other.serviceName_;
    if (!equals) { return equals; }

    equals = serviceReplicaSet_ == other.serviceReplicaSet_;
    if (!equals) { return equals; }

    equals = isFound_ == other.isFound_;
    return equals;
}

bool ServiceTableEntry::operator!=(ServiceTableEntry const & other) const
{
    return !(*this == other);
}

void ServiceTableEntry::EnsureEmpty(int64 version)
{
    Reliability::ServiceReplicaSet empty(
        serviceReplicaSet_.IsStateful,
        serviceReplicaSet_.IsPrimaryLocationValid,
        L"",
        vector<wstring>(),
        serviceReplicaSet_.LookupVersion > 0 ? serviceReplicaSet_.LookupVersion : version);

    serviceReplicaSet_ = move(empty);
}

void ServiceTableEntry::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << "ConsistencyUnitId: " << consistencyUnitId_ << ", " << serviceReplicaSet_;
}

void ServiceTableEntry::WriteToEtw(uint16 contextSequenceId) const
{
    if (serviceReplicaSet_.IsStateful)
    {
        ServiceModelEventSource::Trace->StatefulServiceTableEntryTrace(
            contextSequenceId, 
            consistencyUnitId_.Guid, 
            serviceReplicaSet_.IsPrimaryLocationValid ? serviceReplicaSet_.PrimaryLocation : L"-invalid-", 
            wformatString(serviceReplicaSet_.SecondaryLocations),
            serviceReplicaSet_.LookupVersion);
    }
    else
    {
        ServiceModelEventSource::Trace->StatelessServiceTableEntryTrace(
            contextSequenceId, 
            consistencyUnitId_.Guid, 
            wformatString(serviceReplicaSet_.ReplicaLocations),
            serviceReplicaSet_.LookupVersion);
    }    
}

std::wstring ServiceTableEntry::ToString() const
{
    std::wstring product;
    Common::StringWriter writer(product);
    WriteTo(writer, Common::FormatOptions(0, false, ""));
    return product;
}
