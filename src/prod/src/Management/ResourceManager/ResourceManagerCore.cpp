// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Naming;
using namespace Transport;
using namespace ClientServerTransport;
using namespace Management::ResourceManager;

StringLiteral const TraceComponent("ResourceManagerCore");

AsyncOperationSPtr Result(ErrorCode const & error, AsyncCallback const & callback, AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
}

ResourceManagerCore::ResourceManagerCore(
    Store::PartitionedReplicaId const & partitionedReplicaId,
    IReplicatedStore * replicatedStore,
    wstring const & resourceTypeName)
    : ComponentRoot(StoreConfig::GetConfig().EnableReferenceTracking)
    , PartitionedReplicaTraceComponent(partitionedReplicaId)
    , partitionReplicaId_(partitionedReplicaId)
    , replicatedStore_(replicatedStore)
    , resourceTypeName_(resourceTypeName)
{
}

StoreTransaction ResourceManagerCore::CreateStoreTransaction(ActivityId const & activityId)
{
    return StoreTransaction::Create(this->replicatedStore_, this->partitionReplicaId_, activityId);
}

AsyncOperationSPtr ResourceManagerCore::RegisterResources(
    __in vector<wstring> const & resourceNames,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    ActivityId const & activityId)
{
    ErrorCode error;

    if (resourceNames.empty()
        || (resourceNames.end() != find_if(
            resourceNames.begin(),
            resourceNames.end(),
            [](wstring const & resourceName) -> bool {  return resourceName.empty(); })))
    {
        error = ErrorCode(
            ErrorCodeValue::InvalidArgument,
            L"resourceNames should contain at least one resource name and resource name cannot be empty.");

        return Result(error, callback, parent);
    }

    auto storeTx = this->CreateStoreTransaction(activityId);

    for (wstring const & resourceName : resourceNames)
    {
        ResourceDataItem resource(resourceName);

        error = storeTx.Insert(resource);

        if (!error.IsSuccess())
        {
            return Result(error, callback, parent);
        }
    }

    return StoreTransaction::BeginCommit(move(storeTx), timeout, callback, parent);
}

AsyncOperationSPtr ResourceManagerCore::RegisterResource(
    __in wstring const & resourceName,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    ActivityId const & activityId)
{
    ErrorCode error;

    if (resourceName.empty())
    {
        error = ErrorCode(ErrorCodeValue::InvalidArgument, L"resourceName cannot be empty.");

        return Result(error, callback, parent);
    }

    auto storeTx = this->CreateStoreTransaction(activityId);
    ResourceDataItem resource(resourceName);

    error = storeTx.Insert(resource);

    if (!error.IsSuccess())
    {
        return Result(error, callback, parent);
    }

    return StoreTransaction::BeginCommit(move(storeTx), timeout, callback, parent);
}

ErrorCode ResourceManagerCore::InternalUnregisterResource(
    StoreTransaction & storeTx,
    wstring const & resourceName)
{
    ResourceDataItem resource(resourceName);

    auto error = storeTx.ReadExact(resource);

    if (!error.IsSuccess())
    {
        if (error.IsError(ErrorCodeValue::Enum::StoreRecordNotFound))
        {
            error = ErrorCode::Success();
        }

        return error;
    }

    if (!resource.References.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidOperation,
            wformatString("Unable to unregister resource, {0}, as there are still consumers referencing it: {1}",
                ResourceIdentifier(this->resourceTypeName_, resource.Name),
                resource.References));
    }

    return storeTx.DeleteIfFound(resource);
}

AsyncOperationSPtr ResourceManagerCore::UnregisterResource(
    __in wstring const & resourceName,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    ActivityId const & activityId)
{
    ErrorCode error = ErrorCode::Success();

    if (resourceName.empty())
    {
        error = ErrorCode(ErrorCodeValue::InvalidArgument, L"resourceName cannot be empty.");

        return Result(error, callback, parent);
    }

    auto storeTx = this->CreateStoreTransaction(activityId);

    error = this->InternalUnregisterResource(storeTx, resourceName);

    if (!error.IsSuccess())
    {
        return Result(error, callback, parent);
    }

    return StoreTransaction::BeginCommit(move(storeTx), timeout, callback, parent);
}

AsyncOperationSPtr ResourceManagerCore::UnregisterResources(
    __in vector<wstring> const & resourceNames,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    ActivityId const & activityId)
{
    ErrorCode error = ErrorCode::Success();

    if (resourceNames.empty()
        || (resourceNames.end() != find_if(
            resourceNames.begin(),
            resourceNames.end(),
            [](wstring const & resourceName) -> bool {  return resourceName.empty(); })))
    {
        error = ErrorCode(
            ErrorCodeValue::InvalidArgument,
            L"resourceNames should contain at least one resource name and resource name cannot be empty.");

        return Result(error, callback, parent);
    }

    auto storeTx = this->CreateStoreTransaction(activityId);

    for (wstring const & resourceName : resourceNames)
    {
        error = this->InternalUnregisterResource(storeTx, resourceName);

        if (!error.IsSuccess())
        {
            return Result(error, callback, parent);
        }
    }

    return StoreTransaction::BeginCommit(move(storeTx), timeout, callback, parent);
}

AsyncOperationSPtr ResourceManagerCore::ClaimResource(
    __in Claim const & claim,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    ActivityId const & activityId)
{
    ErrorCode error;

    if (claim.IsEmpty())
    {
        error = ErrorCode(ErrorCodeValue::InvalidArgument, L"claim cannot be empty.");

        return Result(error, callback, parent);
    }

    auto storeTx = this->CreateStoreTransaction(activityId);
    ResourceDataItem resource(claim.ResourceId);

    error = storeTx.ReadExact(resource);

    if (!error.IsSuccess())
    {
        return Result(error, callback, parent);
    }

    for (const ResourceIdentifier consumerId : claim.ConsumerIds)
    {
        if (resource.References.find(consumerId) == resource.References.end()) {
            resource.References.insert(consumerId);
        }
    }

    error = storeTx.Update(resource);

    if (!error.IsSuccess())
    {
        return Result(error, callback, parent);
    }

    return StoreTransaction::BeginCommit(move(storeTx), timeout, callback, parent);
}

AsyncOperationSPtr ResourceManagerCore::ReleaseResource(
    __in Claim const & claim,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    ActivityId const & activityId)
{
    ErrorCode error;

    if (claim.IsEmpty())
    {
        error = ErrorCode(ErrorCodeValue::InvalidArgument, L"claim cannot be empty.");

        return Result(error, callback, parent);
    }

    auto storeTx = this->CreateStoreTransaction(activityId);
    ResourceDataItem resource(claim.ResourceId);

    error = storeTx.ReadExact(resource);

    if (!error.IsSuccess())
    {
        return Result(error, callback, parent);
    }

    for (ResourceIdentifier consumerId : claim.ConsumerIds)
    {
        resource.References.erase(consumerId);
    }

    error = storeTx.Update(resource);

    if (!error.IsSuccess())
    {
        return Result(error, callback, parent);
    }

    return StoreTransaction::BeginCommit(move(storeTx), timeout, callback, parent);
}