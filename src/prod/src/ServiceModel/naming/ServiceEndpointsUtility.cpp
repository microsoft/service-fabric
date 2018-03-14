// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
 
using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;

using namespace Naming;

StringLiteral const TraceComponent("NamingUtility");

void ServiceEndpointsUtility::SetEndpoints(
    ServiceReplicaSet const & replicaSet,
    __in ScopedHeap & heap,
    __out ULONG & countResult,
    __out FABRIC_RESOLVED_SERVICE_ENDPOINT ** endpointsResult)
{
    if (endpointsResult == NULL) 
    { 
        wstring msg(L"NULL FABRIC_RESOLVED_SERVICE_ENDPOINT");

        Trace.WriteError(TraceComponent, "{0}", msg);

        Assert::TestAssert("{0}", msg);

        return;
    }

    auto endpointsCount = replicaSet.IsStateful
        ? (replicaSet.SecondaryLocations.size() + (replicaSet.IsPrimaryLocationValid ? 1 : 0))
        : (replicaSet.ReplicaLocations.size());

    countResult = static_cast<ULONG>(endpointsCount);

    if (endpointsCount > 0)
    {
        auto array = heap.AddArray<FABRIC_RESOLVED_SERVICE_ENDPOINT>(endpointsCount);

        size_t index = 0;
        if (replicaSet.IsStateful && replicaSet.IsPrimaryLocationValid)
        {
            array[index].Address = heap.AddString(replicaSet.PrimaryLocation);
            array[index].Role = FABRIC_SERVICE_ENDPOINT_ROLE::FABRIC_SERVICE_ROLE_STATEFUL_PRIMARY;
            ++index;
        }

        for (auto const & address : (replicaSet.IsStateful ? replicaSet.SecondaryLocations : replicaSet.ReplicaLocations))
        {
            array[index].Address = heap.AddString(address);
            array[index].Role = replicaSet.IsStateful
                ? FABRIC_SERVICE_ENDPOINT_ROLE::FABRIC_SERVICE_ROLE_STATEFUL_SECONDARY
                : FABRIC_SERVICE_ENDPOINT_ROLE::FABRIC_SERVICE_ROLE_STATELESS;
            ++index;
        }

        *endpointsResult = array.GetRawArray();
    }
    else
    {
        *endpointsResult = NULL;
    }
}

void ServiceEndpointsUtility::SetPartitionInfo(
    ServiceTableEntry const & ste,
    PartitionInfo const & partition,
    __in ScopedHeap & heap,
    __out FABRIC_SERVICE_PARTITION_INFORMATION & result)
{
    switch (partition.PartitionScheme)
    {
        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
        {
            result.Kind = FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE;
            auto int64RangePartitionInfo = heap.AddItem<FABRIC_INT64_RANGE_PARTITION_INFORMATION>();
            int64RangePartitionInfo->Id = ste.ConsistencyUnitId.Guid.AsGUID();
            int64RangePartitionInfo->LowKey = partition.LowKey;
            int64RangePartitionInfo->HighKey = partition.HighKey;
            result.Value = int64RangePartitionInfo.GetRawPointer();
        }
        break;

        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED:
        {
            result.Kind = FABRIC_SERVICE_PARTITION_KIND_NAMED;
            auto namedPartitionInfo = heap.AddItem<FABRIC_NAMED_PARTITION_INFORMATION>();
            namedPartitionInfo->Id = ste.ConsistencyUnitId.Guid.AsGUID();
            namedPartitionInfo->Name = heap.AddString(partition.PartitionName);
            result.Value = namedPartitionInfo.GetRawPointer();
        }
        break;

        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON:
        {
            result.Kind = FABRIC_SERVICE_PARTITION_KIND_SINGLETON;
            auto singletonPartitionKeyInfo = heap.AddItem<FABRIC_SINGLETON_PARTITION_INFORMATION>();
            singletonPartitionKeyInfo->Id = ste.ConsistencyUnitId.Guid.AsGUID();
            result.Value = singletonPartitionKeyInfo.GetRawPointer();
        }
        break;

        default:
            wstring msg = wformatString(
                "Unknown FABRIC_PARTITION_SCHEME {0}", 
                static_cast<unsigned int>(partition.PartitionScheme));

            Trace.WriteError(TraceComponent, "{0}", msg);

            Assert::TestAssert("{0}", msg);

            result.Kind = FABRIC_SERVICE_PARTITION_KIND_INVALID;
            result.Value = NULL;
    }
}
