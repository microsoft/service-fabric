// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ServiceTypeDescription.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

GlobalWString const ServiceTypeDescription::FormatHeader = make_global<wstring>(L"ServiceTypeName BlockList");

ServiceTypeDescription::ServiceTypeDescription(
    wstring && name, 
    set<Federation::NodeId> && blockList)
    : name_(move(name)),
      blockList_(move(blockList))
{
}

ServiceTypeDescription::ServiceTypeDescription(ServiceTypeDescription const & other)
    : name_(other.name_),
    blockList_(other.blockList_)
{
}

ServiceTypeDescription::ServiceTypeDescription(ServiceTypeDescription && other)
    : name_(move(other.name_)),
    blockList_(move(other.blockList_))
{
}

ServiceTypeDescription & ServiceTypeDescription::operator = (ServiceTypeDescription && other)
{
    if (this != &other)
    {
        name_ = move(other.name_);
        blockList_ = move(other.blockList_);
    }

    return *this;
}

bool ServiceTypeDescription::operator == (ServiceTypeDescription const & other ) const
{
    // it is only used when we update the service description
    ASSERT_IFNOT(name_ == other.name_, "Comparison between different two different service types");

    return (blockList_ == other.blockList_);
}

bool ServiceTypeDescription::operator != (ServiceTypeDescription const & other ) const
{
    return !(*this == other);
}

bool ServiceTypeDescription::IsInBlockList(Federation::NodeId node) const
{
    return blockList_.find(node) != blockList_.end();
}

void ServiceTypeDescription::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} blockList:{1}", name_, vector<Federation::NodeId>(blockList_.begin(), blockList_.end()));
}

void ServiceTypeDescription::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}
