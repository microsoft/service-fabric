// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ServicePackageEntry.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Reliability::LoadBalancingComponent;
using namespace Common;

GlobalWString const ServicePackageEntry::TraceDescription = make_global<wstring>(L"[ServicePackageEntries]\r\n#servicePackageName loads");

ServicePackageEntry::ServicePackageEntry(
    std::wstring const & name,
    LoadEntry && loads,
    std::vector<std::wstring> && images)
    : name_(name),
    loads_(move(loads)),
    containerImages_(move(images))
{
}

ServicePackageEntry::ServicePackageEntry(ServicePackageEntry && other)
    : name_(move(other.name_)),
    loads_(move(other.loads_)),
    containerImages_(move(other.containerImages_))
{
}

ServicePackageEntry & ServicePackageEntry::operator=(ServicePackageEntry && other)
{
    if (this != &other)
    {
        name_ = move(other.name_);
        loads_ = move(other.loads_);
        containerImages_ = move(other.containerImages_);
    }
    return *this;
}

void ServicePackageEntry::WriteTo(Common::TextWriter & writer, Common::FormatOptions const &) const
{
    writer.Write("{0} {1} {2}", name_, loads_, containerImages_);
}

void ServicePackageEntry::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}
