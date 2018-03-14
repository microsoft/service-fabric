// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

ReplicaDescription const* ConfigurationReplyMessageBody::GetReplica(Federation::NodeId nodeId) const
{
    for (ReplicaDescription const& desc : replicas_)
    {
        if (desc.FederationNodeId == nodeId)
        {
            return &desc;
        }
    }

    return nullptr;
}

void ConfigurationReplyMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    w.WriteLine("{0}", fudesc_);

    for (ReplicaDescription const & replica : replicas_)
    {
        w.Write("{0}", replica);
    }

    w.WriteLine("EC: {0}", errorCode_);
}

void ConfigurationReplyMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->ConfigurationReplyMessageBody(contextSequenceId, fudesc_, replicas_, errorCode_);
}
