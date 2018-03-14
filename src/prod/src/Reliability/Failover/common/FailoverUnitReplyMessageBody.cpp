// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;
using namespace Reliability;

void FailoverUnitReplyMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
{ 
    fudesc_.WriteTo(w, options);
    w.WriteLine(" EC: ");
    errorCode_.WriteTo(w, options);
}

void FailoverUnitReplyMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->FailoverUnitReplyMessageBody(contextSequenceId, fudesc_, errorCode_);
}
