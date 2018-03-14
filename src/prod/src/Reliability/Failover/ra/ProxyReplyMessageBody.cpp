// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

void ProxyReplyMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
{
    fudesc_.WriteTo(w, options);
    w.WriteLine();
    localReplica_.WriteTo(w, options);

    for (Reliability::ReplicaDescription const & replica : remoteReplicas_)
    {
        replica.WriteTo(w, options);
    }
 
    w.Write("State: {0} EC: {1}", flags_, proxyErrorCode_);
}

void ProxyReplyMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    RAEventSource::Events->ProxyReplyMessageBody(
        contextSequenceId,
        fudesc_,
        localReplica_,
        remoteReplicas_,        
        wformatString(flags_),
        proxyErrorCode_);
}
