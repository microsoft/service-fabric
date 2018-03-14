// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

void ReplicaMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
{
    fudesc_.WriteTo(w, options);
    w.Write(" {0} ", service_.UpdateVersion);
    w.WriteLine();
    replica_.WriteTo(w, options);
    service_.WriteTo(w, options);
}

void ReplicaMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->ReplicaMessageBody(contextSequenceId, fudesc_, service_.UpdateVersion, replica_, service_.Name, service_.Type.ServiceTypeName, service_.TargetReplicaSetSize, service_.MinReplicaSetSize, service_.Instance);
}
