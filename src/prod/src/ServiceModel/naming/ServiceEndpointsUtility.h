// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceEndpointsUtility
    {
    public:
        static void SetEndpoints(
            Reliability::ServiceReplicaSet const & replicaSet,
            __in Common::ScopedHeap & heap,
            __out ULONG & countResult,
            __out FABRIC_RESOLVED_SERVICE_ENDPOINT ** endpointsResult);

        static void SetPartitionInfo(
            Reliability::ServiceTableEntry const & ste,
            PartitionInfo const & partition,
            __in Common::ScopedHeap & heap,
            __out FABRIC_SERVICE_PARTITION_INFORMATION & result);
    };
}
