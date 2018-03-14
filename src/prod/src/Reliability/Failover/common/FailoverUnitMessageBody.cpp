// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Reliability;

void FailoverUnitMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
{ 
    fudesc_.WriteTo(w, options);
}

void FailoverUnitMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->FailoverUnitMessageBody(contextSequenceId, fudesc_);
}
