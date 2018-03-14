// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Reliability;

void ActivateMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
{
    FailoverUnitDescription.WriteTo(w, options);

    w.Write(" {0} ", ServiceDescription.UpdateVersion);
    w.WriteLine();

    for (ReplicaDescription const & replica : ReplicaDescriptions)
    {
        replica.WriteTo(w, options);
    }

    w.Write("{0}", deactivationInfo_);
}

void ActivateMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->ActivateMessageBody(
        contextSequenceId,
        FailoverUnitDescription,
        ServiceDescription.UpdateVersion,
        ReplicaDescriptions,
        deactivationInfo_);
}
