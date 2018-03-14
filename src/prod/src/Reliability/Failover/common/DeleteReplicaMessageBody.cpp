// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

void DeleteReplicaMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
{
    fudesc_.WriteTo(w, options);
    w.Write(" {0} ", service_.UpdateVersion);
    w.WriteLine();
    replica_.WriteTo(w, options);
    service_.WriteTo(w, options);
    w.Write(" {0} ", isForce_);
}

void DeleteReplicaMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->DeleteReplicaMessageBody(contextSequenceId, fudesc_, service_.UpdateVersion, replica_, service_, isForce_);
}
