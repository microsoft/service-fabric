// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

void ProxyRequestMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
{
    fudesc_.WriteTo(w, options);
    w.WriteLine();

    localReplica_.WriteTo(w, options);

    for (Reliability::ReplicaDescription const & replica : remoteReplicas_)
    {
        replica.WriteTo(w, options);
    }

    w << service_.Name;

    w.Write("Flags: {0}", wformatString(flags_));
}

void ProxyRequestMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    RAEventSource::Events->ProxyRequestMessageBody(
        contextSequenceId,
        fudesc_,
        localReplica_,
        remoteReplicas_,
        service_.Name,
        flags_);
}
