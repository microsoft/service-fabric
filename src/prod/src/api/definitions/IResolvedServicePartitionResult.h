// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IResolvedServicePartitionResult
    {
    public:
        ~IResolvedServicePartitionResult() {};

        virtual Common::ErrorCode GetPartition(
            __inout Common::ScopedHeap & heap, 
            __out FABRIC_RESOLVED_SERVICE_PARTITION & resolvedServicePartition) = 0;

        virtual Reliability::ServiceReplicaSet const& GetServiceReplicaSet() = 0;
        virtual Common::ErrorCode CompareVersion(IResolvedServicePartitionResultPtr other, LONG *result) = 0;
        virtual Common::ErrorCode GetFMVersion(LONGLONG *value) = 0;
        virtual Common::ErrorCode GetGeneration(LONGLONG *value) = 0;
    };
}
