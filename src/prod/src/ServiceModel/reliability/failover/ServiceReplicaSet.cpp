// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace Transport;
using namespace std;

INITIALIZE_SIZE_ESTIMATION(ServiceReplicaSet)

GlobalWString InvalidPrimaryTrace = make_global<wstring>(L"Primary=[-invalid-], ");
GlobalWString SecondariesTrace = make_global<wstring>(L"Secondaries");
GlobalWString ReplicasTrace = make_global<wstring>(L"Replicas");

ServiceReplicaSet::ServiceReplicaSet()
    : isStateful_(false),
    isPrimaryLocationValid_(false),
    primaryLocation_(),
    replicaLocations_(),
    lookupVersion_(0)
{
}

ServiceReplicaSet::ServiceReplicaSet(
    bool isStateful,
    bool isPrimaryLocationValid,
    std::wstring && primaryLocation,
    std::vector<std::wstring> && replicaLocations,
    int64 lookupVersion)
    : isStateful_(isStateful),
    isPrimaryLocationValid_(isPrimaryLocationValid),
    primaryLocation_(move(primaryLocation)),
    replicaLocations_(move(replicaLocations)),
    lookupVersion_(lookupVersion)
{
}

ServiceReplicaSet::ServiceReplicaSet(ServiceReplicaSet const & other)
    : isStateful_(other.isStateful_),
    isPrimaryLocationValid_(other.isPrimaryLocationValid_),
    primaryLocation_(other.primaryLocation_),
    replicaLocations_(other.replicaLocations_),
    lookupVersion_(other.lookupVersion_)
{
}

ServiceReplicaSet & ServiceReplicaSet::operator=(ServiceReplicaSet const & other)
{
    if (this != &other)
    {
        isStateful_ = other.isStateful_;
        isPrimaryLocationValid_ = other.isPrimaryLocationValid_;
        primaryLocation_ = other.primaryLocation_;
        replicaLocations_ = other.replicaLocations_;
        lookupVersion_ = other.lookupVersion_;
    }

    return *this;
}
        
ServiceReplicaSet::ServiceReplicaSet(ServiceReplicaSet && other)
    : isStateful_(std::move(other.isStateful_)),
        isPrimaryLocationValid_(std::move(other.isPrimaryLocationValid_)),
        primaryLocation_(std::move(other.primaryLocation_)),
        replicaLocations_(std::move(other.replicaLocations_)),
        lookupVersion_(std::move(other.lookupVersion_))
{
}

ServiceReplicaSet & ServiceReplicaSet::operator=(ServiceReplicaSet && other)
{
    if (this != &other)
    {
        isStateful_ = std::move(other.isStateful_);
        isPrimaryLocationValid_ = std::move(other.isPrimaryLocationValid_);
        primaryLocation_ = std::move(other.primaryLocation_);
        replicaLocations_ = std::move(other.replicaLocations_);
        lookupVersion_ = std::move(other.lookupVersion_);
    }

    return *this;
}

bool ServiceReplicaSet::IsEmpty() const
{
    if (IsStateful)
    {
        return !IsPrimaryLocationValid && SecondaryLocations.size() == 0;
    }
    else
    {
        return ReplicaLocations.size() == 0;
    }
}

bool ServiceReplicaSet::operator == (ServiceReplicaSet const & other) const
{
    bool equals = true;

    equals = isStateful_ == other.isStateful_;
    if (!equals) { return equals; }

    equals = isPrimaryLocationValid_ == other.isPrimaryLocationValid_;
    if (!equals) { return equals; }

    equals = primaryLocation_ == other.primaryLocation_;
    if (!equals) { return equals; }

    equals = replicaLocations_.size() == other.replicaLocations_.size();
    if (!equals) { return equals; }

    for (auto ix=0; ix<replicaLocations_.size(); ++ix)
    {
        equals = replicaLocations_[ix] == other.replicaLocations_[ix];
        if (!equals) { return equals; }
    }

    equals = lookupVersion_ == other.lookupVersion_;
    return equals;
}

bool ServiceReplicaSet::operator != (ServiceReplicaSet const & other) const
{
    return !(*this == other);
}

void ServiceReplicaSet::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    if (isStateful_)
    {
        if (isPrimaryLocationValid_)
        {
            w.WriteLine("Primary: {0}", primaryLocation_);
        }

        w.WriteLine("Secondaries:");
        for (std::wstring const& secondaryLocation : replicaLocations_)
        {
            w.WriteLine("  {0}", secondaryLocation);
        }
    }
    else
    {
        w.WriteLine("Replicas:");
        for (std::wstring const& replicaLocation : replicaLocations_)
        {
            w.WriteLine("  {0}", replicaLocation);
        }
    }
            
    w.WriteLine("Version: {0}", lookupVersion_);
}

string ServiceReplicaSet::AddField(Common::TraceEvent &traceEvent, string const& name)
{
    traceEvent.AddField<wstring>(name + ".primary");
    traceEvent.AddField<wstring>(name + ".replicaType");
    traceEvent.AddField<wstring>(name + ".replicas");
    traceEvent.AddField<int64>(name + ".version");
    return "{0}{1}=[{2}], Version={3}";
}
         
void ServiceReplicaSet::FillEventData(TraceEventContext &context) const
{
    if (isStateful_)
    {
        if (isPrimaryLocationValid_)
        {
            context.WriteCopy(wformatString("Primary=[{0}], ", primaryLocation_));
        }
        else
        {
            context.Write<wstring>(*InvalidPrimaryTrace);
        }

        context.Write<wstring>(*SecondariesTrace);
    }
    else
    {
        context.Write<wstring>(L"");
        context.Write<wstring>(*ReplicasTrace);   
    }

    wstring replicas;
    StringWriter writer(replicas);
    bool isFirst = true;
    for (auto it = replicaLocations_.begin(); it != replicaLocations_.end(); ++it)
    {
        if (!isFirst)
        {
            writer << L", ";
        }
        else
        {
            isFirst = false;
        }

        writer << L"'" << *it << L"'";
    }

    context.WriteCopy<wstring>(replicas);

    context.Write<int64>(lookupVersion_);
}

