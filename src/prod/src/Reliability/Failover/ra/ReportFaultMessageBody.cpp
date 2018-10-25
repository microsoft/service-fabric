// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;

void ReportFaultMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->ReportFaultMessageBody(
        contextSequenceId,
        fudesc_,
        replicaDesc_,
        faultType_,
        activityDescription_);
}
