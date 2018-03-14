// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

void RAReplicaMessageBody::WriteTo(Common::TextWriter& writer, Common::FormatOptions const & options) const
{
    ReplicaMessageBody::WriteTo(writer, options);

    writer << " " << deactivationInfo_;
}

void RAReplicaMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->RAReplicaMessageBody(contextSequenceId, fudesc_, service_.UpdateVersion, replica_, service_, deactivationInfo_);
}
