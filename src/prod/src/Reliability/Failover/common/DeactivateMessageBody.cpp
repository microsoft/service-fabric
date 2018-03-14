// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Reliability;

void DeactivateMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
{
    FailoverUnitDescription.WriteTo(w, options);

    w.Write(" {0} ", ServiceDescription.UpdateVersion);
    w.WriteLine();

    for (ReplicaDescription const & replica : ReplicaDescriptions)
    {
        replica.WriteTo(w, options);
    }

    w.Write("{0} {1}", isForce_ ? L"1" : L"0", deactivationInfo_);
}

void DeactivateMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->DeactivateMessageBody(
        contextSequenceId, 
        FailoverUnitDescription, 
        ServiceDescription.UpdateVersion, 
        ReplicaDescriptions, 
        isForce_,
        deactivationInfo_);
}
