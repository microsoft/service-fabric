// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Client
{
    class ResolvedServicePartitionResult
        : public Api::IResolvedServicePartitionResult
        , public Common::ComponentRoot
    {
        DENY_COPY(ResolvedServicePartitionResult)
    public:

        ResolvedServicePartitionResult(__in Naming::ResolvedServicePartitionSPtr &rsp);
        ResolvedServicePartitionResult(__in Naming::ResolvedServicePartitionSPtr &&rsp);

        __declspec(property(get=get_RSP)) Naming::ResolvedServicePartitionSPtr const & ResolvedServicePartition;
        Naming::ResolvedServicePartitionSPtr const & get_RSP() const { return rsp_; }

        Common::ErrorCode GetPartition(
            __inout Common::ScopedHeap & heap, 
            __out FABRIC_RESOLVED_SERVICE_PARTITION & resolvedServicePartition);

        Reliability::ServiceReplicaSet const& GetServiceReplicaSet();
        Common::ErrorCode CompareVersion(Api::IResolvedServicePartitionResultPtr other, __inout LONG *result);
        Common::ErrorCode GetFMVersion(__inout LONGLONG *value);
        Common::ErrorCode GetGeneration(__inout LONGLONG *value);

    private:
        Naming::ResolvedServicePartitionSPtr rsp_;
    };
}
