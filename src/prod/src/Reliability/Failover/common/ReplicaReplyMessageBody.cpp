// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;
using namespace Reliability;

void ReplicaReplyMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
{
    fudesc_.WriteTo(w, options);
    w.Write(" ");
    replica_.WriteTo(w, options);
    w.Write(" EC: ");
    errorCode_.WriteTo(w, options);
}

void ReplicaReplyMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->ReplicaReplyMessageBody(contextSequenceId, fudesc_, replica_, errorCode_);
}
