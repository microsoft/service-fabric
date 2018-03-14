// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

HealthReportReplica::HealthReportReplica(const Replica & r) :
    currentRole_(r.CurrentConfigurationRole),
    previousRole_(r.PreviousConfigurationRole),
    queryStatus_(r.QueryStatus),
    replicaId_(r.ReplicaId),
    nodeId_(r.FederationNodeId)
{
}

bool HealthReportReplica::operator==(HealthReportReplica const & rhs) const
{
    if (currentRole_ != rhs.currentRole_)
    {
        return false;
    }

    if (previousRole_ != rhs.previousRole_)
    {
        return false;
    }

    if (queryStatus_ != rhs.queryStatus_)
    {
        return false;
    }

    if (replicaId_ != rhs.replicaId_)
    {
        return false;
    }

    if (nodeId_ != rhs.nodeId_)
    {
        return false;
    }

    return true;
}

bool HealthReportReplica::operator!=(HealthReportReplica const & rhs) const
{
	return !(*this == rhs);
}

wstring HealthReportReplica::GetStringRepresentation() const
{
    return wformatString("{0}/{1} {2} {3} {4}", previousRole_, currentRole_, queryStatus_, nodeId_, replicaId_);
}
