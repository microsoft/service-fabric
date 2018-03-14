// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;

NodeDeactivateReplyMessageBody::NodeDeactivateReplyMessageBody()
{
}

NodeDeactivateReplyMessageBody::NodeDeactivateReplyMessageBody(int64 sequenceNumber)
    : sequenceNumber_(sequenceNumber)
{
}

void NodeDeactivateReplyMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->NodeDeactivateReplyMessageBody(contextSequenceId, sequenceNumber_);
}

void NodeDeactivateReplyMessageBody::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.WriteLine(sequenceNumber_);
}
