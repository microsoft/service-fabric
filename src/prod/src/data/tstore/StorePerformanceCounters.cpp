// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::TStore;
INITIALIZE_COUNTER_SET(StorePerformanceCounters)

StorePerformanceCountersSPtr StorePerformanceCounters::CreateInstance(
    __in Common::Guid const& partitionId,
    __in FABRIC_REPLICA_ID const& replicaId,
    __in FABRIC_STATE_PROVIDER_ID const& stateProviderId)
{
    std::wstring id;
    Common::StringWriter writer(id);
    writer.Write(
        "{0}:{1}:{2}",
        partitionId,
        replicaId,
        stateProviderId);
    return CreateInstance(id);
}
