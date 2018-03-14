// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace Client;
using namespace Reliability;
using namespace Naming;

ResolvedServicePartitionResult::ResolvedServicePartitionResult(
    Naming::ResolvedServicePartitionSPtr &rsp)
    : rsp_(rsp)
{
}

ResolvedServicePartitionResult::ResolvedServicePartitionResult(
    Naming::ResolvedServicePartitionSPtr &&rsp)
    : rsp_(move(rsp))
{
}

ErrorCode ResolvedServicePartitionResult::GetPartition(
    __inout Common::ScopedHeap & heap, 
    __out FABRIC_RESOLVED_SERVICE_PARTITION & resolvedServicePartition)
{
    rsp_->ToPublicApi(heap, resolvedServicePartition);
    return ErrorCode::Success();
}

ServiceReplicaSet const& ResolvedServicePartitionResult::GetServiceReplicaSet()
{
    return rsp_->Locations.ServiceReplicaSet;
}

ErrorCode ResolvedServicePartitionResult::CompareVersion(IResolvedServicePartitionResultPtr other, __inout LONG *result)
{
    ResolvedServicePartitionResult *otherResult = (ResolvedServicePartitionResult*)(other.get());
    if (otherResult == nullptr)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return rsp_->CompareVersion(otherResult->ResolvedServicePartition, *result);
}

ErrorCode ResolvedServicePartitionResult::GetFMVersion(__inout LONGLONG *value)
{
    if (value == NULL)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    *value = rsp_->FMVersion;
    return ErrorCode::Success();
}

ErrorCode ResolvedServicePartitionResult::GetGeneration(__inout LONGLONG *value)
{
    if (value == NULL)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    *value = rsp_->Generation.Value;
    return ErrorCode::Success();
}
