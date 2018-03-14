// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;

void NodeActivateRequestMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->NodeActivateRequestMessageBody(contextSequenceId, sequenceNumber_, Sender);
}

void NodeActivateRequestMessageBody::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << " " << sequenceNumber_ << " " << Sender;
}
