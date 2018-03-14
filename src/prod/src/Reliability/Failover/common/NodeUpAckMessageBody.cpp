// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

void NodeUpAckMessageBody::WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
{
    w.Write("NodeVersionInstance = {0}. IsNodeActivated = {1}. ActivationSequenceNumber = {2}", fabricVersionInstance_, isNodeActivated_, nodeActivationSequenceNumber_);
    

    for (UpgradeDescription const & upgrade : upgrades_)
    {
        w.Write(" {0}\r\n", upgrade);
    }
}

void NodeUpAckMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->NodeUpAckMessageBody(
        contextSequenceId,
        isNodeActivated_,
        nodeActivationSequenceNumber_,
        fabricVersionInstance_,
        upgrades_);                
}
