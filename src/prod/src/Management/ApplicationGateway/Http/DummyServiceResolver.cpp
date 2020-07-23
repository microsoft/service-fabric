// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Client;
using namespace Naming;
using namespace HttpApplicationGateway;
using namespace Reliability;

AsyncOperationSPtr DummyServiceResolver::BeginOpen(
    TimeSpan &timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    UNREFERENCED_PARAMETER(timeout);
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        ErrorCodeValue::Success, 
        callback, 
        parent);
}

ErrorCode DummyServiceResolver::EndOpen(AsyncOperationSPtr const &operation)
{
    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr DummyServiceResolver::BeginPrefixResolveServicePartition(
    ParsedGatewayUriSPtr const &parsedUri,
    IResolvedServicePartitionResultPtr &prevResult,
    wstring const &traceId,
    KAllocator &allocator,
    TimeSpan &timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    UNREFERENCED_PARAMETER(parsedUri);
    UNREFERENCED_PARAMETER(prevResult);
    UNREFERENCED_PARAMETER(traceId);
    UNREFERENCED_PARAMETER(allocator);
    UNREFERENCED_PARAMETER(timeout);

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        ErrorCodeValue::Success,
        callback,
        parent);
}

ErrorCode DummyServiceResolver::EndResolveServicePartition(
    AsyncOperationSPtr const &operation,
    IResolvedServicePartitionResultPtr &prevResult)
{
    UNREFERENCED_PARAMETER(prevResult);
    return CompletedAsyncOperation::End(operation);
}

ErrorCode DummyServiceResolver::GetReplicaEndpoint(
    IResolvedServicePartitionResultPtr const &rspResult,
    TargetReplicaSelector::Enum,
    __out wstring &replicaLocation)
{
    UNREFERENCED_PARAMETER(rspResult);

    replicaLocation = HttpApplicationGatewayConfig::GetConfig().TargetResolutionAddress;
    return ErrorCodeValue::Success;
}

wstring DummyServiceResolver::GetServiceName(IResolvedServicePartitionResultPtr const &rspResult)
{
    UNREFERENCED_PARAMETER(rspResult);

    return L"";
}
