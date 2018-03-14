// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ServicePackageNode.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ServicePackageNode::ServicePackageNode()
    : sharedServiceRegularReplicaCount_(0),
    sharedServiceDisappearReplicaCount_(0),
    exclusiveServiceTotalReplicaCount_(0)
{
}

ServicePackageNode::ServicePackageNode(ServicePackageNode && other)
    : sharedServiceRegularReplicaCount_(other.sharedServiceRegularReplicaCount_),
    sharedServiceDisappearReplicaCount_(other.sharedServiceDisappearReplicaCount_),
    exclusiveServiceTotalReplicaCount_(other.exclusiveServiceTotalReplicaCount_)
{
}

ServicePackageNode & ServicePackageNode::operator = (ServicePackageNode && other)
{
    sharedServiceRegularReplicaCount_ = other.sharedServiceRegularReplicaCount_;
    sharedServiceDisappearReplicaCount_ = other.sharedServiceDisappearReplicaCount_;
    exclusiveServiceTotalReplicaCount_ = other.exclusiveServiceTotalReplicaCount_;
    return *this;
}

void ServicePackageNode::WriteTo(TextWriter& writer, FormatOptions const &) const
{
    writer.Write("shared regular {0}, shared disappear {1} exclusive total {2}", sharedServiceRegularReplicaCount_, sharedServiceDisappearReplicaCount_, exclusiveServiceTotalReplicaCount_);
}

void ServicePackageNode::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

bool ServicePackageNode::HasAnyReplica() const
{
    return sharedServiceRegularReplicaCount_ > 0 || sharedServiceDisappearReplicaCount_ > 0 || exclusiveServiceTotalReplicaCount_ > 0;
}

int ServicePackageNode::GetSharedServiceReplicaCount() const
{
    return sharedServiceRegularReplicaCount_ + sharedServiceDisappearReplicaCount_;
}

void ServicePackageNode::MergeServicePackages(ServicePackageNode const & other)
{
    this->SharedServiceRegularReplicaCount += other.SharedServiceRegularReplicaCount;
    this->SharedServiceDisappearReplicaCount += other.SharedServiceDisappearReplicaCount;
    this->ExclusiveServiceTotalReplicaCount += other.ExclusiveServiceTotalReplicaCount;
}
