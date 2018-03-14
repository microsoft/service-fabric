// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Common
{
    INITIALIZE_COUNTER_SET(JobQueuePerfCounters)

    shared_ptr<JobQueuePerfCounters> JobQueuePerfCounters::CreateInstance(
        wstring const &jobQueueName,
        wstring const &uniqueId)
    {
        std::wstring id;
        Common::StringWriter writer(id);
        writer.Write("{0}:{1}:{2}", jobQueueName, uniqueId, SequenceNumber::GetNext());
        return JobQueuePerfCounters::CreateInstance(id);
    }
}
