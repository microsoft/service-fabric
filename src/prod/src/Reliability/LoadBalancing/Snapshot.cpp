// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Snapshot.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

Snapshot::Snapshot()
{
}

Snapshot::Snapshot(Snapshot && other)
    :serviceDomainSnapshot_(move(other.serviceDomainSnapshot_))
    ,createdTimeUtc_(other.createdTimeUtc_)
    ,rgDomainId_(move(other.rgDomainId_))
{
}

Snapshot::Snapshot(map<wstring, ServiceDomain::DomainData> && serviceDomainSnapshot)
    :serviceDomainSnapshot_(move(serviceDomainSnapshot))
    ,createdTimeUtc_(StopwatchTime::ToDateTime(Stopwatch::Now()))
    ,rgDomainId_()
{
}

ServiceDomain::DomainData const * Reliability::LoadBalancingComponent::Snapshot::GetRGDomainData() const
{
    if (!rgDomainId_.empty())
    {
        auto const & itDomainSnapshot = serviceDomainSnapshot_.find(rgDomainId_);
        if (itDomainSnapshot != serviceDomainSnapshot_.end())
        {
            return &(itDomainSnapshot->second);
        }
    }
    return nullptr;
}

