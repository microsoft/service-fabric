// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RGStatistics.h"
#include "ServicePackageDescription.h"
#include "NodeDescription.h"
#include "ServiceDescription.h"
#include "Snapshot.h"
#include "PLBConfig.h"
#include "PLBEventSource.h"
#include "PLBStatistics.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

void PLBStatistics::AddServicePackage(ServicePackageDescription const& sp)
{
    rgStatistics_.AddServicePackage(sp);
}

void PLBStatistics::RemoveServicePackage(ServicePackageDescription const& sp)
{
    rgStatistics_.RemoveServicePackage(sp);
}

void PLBStatistics::UpdateServicePackage(ServicePackageDescription const& oldSp, ServicePackageDescription const& newSp)
{
    rgStatistics_.UpdateServicePackage(oldSp, newSp);
}

void PLBStatistics::AddNode(NodeDescription const & node)
{
    rgStatistics_.AddNode(node);
}

void PLBStatistics::RemoveNode(NodeDescription const & node)
{
    rgStatistics_.RemoveNode(node);
}

void PLBStatistics::UpdateNode(NodeDescription const& node1, NodeDescription const& node2)
{
    rgStatistics_.UpdateNode(node1, node2);
}

void PLBStatistics::AddService(ServiceDescription const & service)
{
    rgStatistics_.AddService(service);
    autoScaleStatistics_.AddService(service);
}

void PLBStatistics::DeleteService(ServiceDescription const & service)
{
    rgStatistics_.DeleteService(service);
    autoScaleStatistics_.DeleteService(service);
}

void PLBStatistics::UpdateService(ServiceDescription const & service1, ServiceDescription const & service2)
{
    rgStatistics_.UpdateService(service1, service2);
    autoScaleStatistics_.UpdateService(service1, service2);
}

void PLBStatistics::Update(Snapshot const &snapshot)
{
    rgStatistics_.Update(snapshot);
    defragStatistics_.Update(snapshot);
}

void PLBStatistics::TraceStatistics(PLBEventSource const& trace)
{
    trace.ResourceGovernanceStatistics(rgStatistics_);
    trace.AutoScalingStatistics(autoScaleStatistics_);
    trace.DefragmentationStatistics(defragStatistics_);
}
