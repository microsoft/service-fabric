// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Reliability;

void ConfigurationMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
{
    fudesc_.WriteTo(w, options);
    w.Write(" {0} ", serviceDesc_.UpdateVersion);
    w.WriteLine();

    for (ReplicaDescription const & replica : replicas_)
    {
        replica.WriteTo(w, options);
    }
}

void ConfigurationMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->ConfigurationMessageBody(contextSequenceId, fudesc_, serviceDesc_.UpdateVersion, replicas_);
}
