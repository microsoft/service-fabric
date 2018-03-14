// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

void ServiceTableUpdateMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->ServiceTableUpdateMessageBody(
        contextSequenceId,
        generation_,
        serviceTableEntries_.size(),
        versionRangeCollection_,
        endVersion_,
        isFromFMM_);
}
