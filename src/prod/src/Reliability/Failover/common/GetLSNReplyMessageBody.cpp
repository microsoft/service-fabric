// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;
using namespace Reliability;

void GetLSNReplyMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
{
    ReplicaReplyMessageBody::WriteTo(w, options);

    w << " DeactivationInfo: " << deactivationInfo_;
}

void GetLSNReplyMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->GetLSNReplyMessageBody(contextSequenceId, fudesc_, replica_, deactivationInfo_, errorCode_);
}
