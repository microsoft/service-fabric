// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

INITIALIZE_COUNTER_SET(REPerformanceCounters)

REPerformanceCountersSPtr REPerformanceCounters::CreateInstance(
    std::wstring const & partitionId,
    FABRIC_REPLICA_ID const & replicaId)
{
    std::wstring id;
    Common::StringWriter writer(id);
    writer.Write("{0}:{1}:{2}", partitionId, replicaId, Common::SequenceNumber::GetNext());
    return CreateInstance(id);
}

} // end namespace ReplicationComponent
} // end namespace Reliability
