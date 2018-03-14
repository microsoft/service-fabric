// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;

void NodeUpgradeReplyMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->NodeUpgradeReplyMessageBody(contextSequenceId, applicationId_, instanceId_, replicas_);
}
