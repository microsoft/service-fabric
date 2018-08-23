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

StringLiteral const TraceComponent("ResourceManagerService");

template<typename TReceiverContext, typename TAsyncOperation>
RequestHandler<TReceiverContext> CreateHandler()
{
    return RequestHandler<TReceiverContext>(&ResourceManagerAsyncOperation<TReceiverContext>::CreateAndStart<TAsyncOperation>);
}

template<typename TReceiverContext>
void AddHandler(
    unordered_map<wstring, RequestHandler<TReceiverContext>> & map,
    wstring const & action,
    RequestHandler<TReceiverContext> const & handler)
{
    map.insert(make_pair(action, handler));
}

template<typename TReceiverContext>
shared_ptr<ResourceManagerService<TReceiverContext>> ResourceManagerService<TReceiverContext>::Create(
    Store::PartitionedReplicaId const & partitionedReplicaId,
    IReplicatedStore * replicatedStore,
    wstring const & resourceTypeName,
    QueryMetadataFunction const & queryResourceMetadataFunction,
    TimeSpan const & maxRetryDelay)
{
    return make_shared<ResourceManagerService<TReceiverContext>>(
        partitionedReplicaId,
        replicatedStore,
        resourceTypeName,
        queryResourceMetadataFunction,
        maxRetryDelay);
}

template<typename TReceiverContext>
ResourceManagerService<TReceiverContext>::ResourceManagerService(
    Store::PartitionedReplicaId const & partitionedReplicaId,
    IReplicatedStore * replicatedStore,
    wstring const & resourceTypeName,
    QueryMetadataFunction const & queryResourceMetadataFunction,
    TimeSpan const & maxRetryDelay)
    : ComponentRoot(StoreConfig::GetConfig().EnableReferenceTracking)
    , PartitionedReplicaTraceComponent(partitionedReplicaId)
    , context_(
        *this,
        partitionedReplicaId,
        replicatedStore,
        resourceTypeName,
        Constants::RequestTrackerSubComponent,
        queryResourceMetadataFunction,
        maxRetryDelay)
    , requestHandlerMap_()
{
    this->InitializeRequestHandlers();
}

template<typename TReceiverContext>
void ResourceManagerService<TReceiverContext>::InitializeRequestHandlers()
{
    unordered_map<wstring, RequestHandler<TReceiverContext>> handlerMap;

    AddHandler<TReceiverContext>(handlerMap, ResourceManagerMessage::ClaimResourceAction, CreateHandler<TReceiverContext, ClaimResourceAsyncOperation<TReceiverContext>>());
    AddHandler<TReceiverContext>(handlerMap, ResourceManagerMessage::ReleaseResourceAction, CreateHandler<TReceiverContext, ReleaseResourceAsyncOperation<TReceiverContext>>());

    this->requestHandlerMap_.swap(handlerMap);
}

template<typename TReceiverContext>
AsyncOperationSPtr ResourceManagerService<TReceiverContext>::Handle(
    __in MessageUPtr request,
    __in unique_ptr<TReceiverContext> requestContext,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto handlerRecord = this->requestHandlerMap_.find(request->Action);

    if (handlerRecord == this->requestHandlerMap_.end())
    {
        return CompletedResourceManagerAsyncOperation<TReceiverContext>::CreateAndStart(
            this->context_,
            ErrorCode(
                ErrorCodeValue::InvalidMessage,
                wformatString("Action {0} is not supported by ResourceManager. MessageId: {1}",
                    request->Action,
                    request->MessageId)),
            callback,
            parent);
    }

    return (handlerRecord->second)(
        this->context_,
        move(request),
        move(requestContext),
        timeout,
        callback,
        parent);
}

template<typename TReceiverContext>
void ResourceManagerService<TReceiverContext>::GetSupportedActions(vector<wstring> & actions)
{
    transform(
        this->requestHandlerMap_.begin(),
        this->requestHandlerMap_.end(),
        back_inserter(actions),
        [](pair<wstring, RequestHandler<TReceiverContext>> const & pair)->wstring
        {
            return wstring(pair.first);
        });
}

template<typename TReceiverContext>
AsyncOperationSPtr ResourceManagerService<TReceiverContext>::RegisterResource(
    __in wstring const & resourceName,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    ActivityId const & activityId)
{
    return this->context_.ResourceManager.RegisterResource(resourceName, timeout, callback, parent, activityId);
}

template<typename TReceiverContext>
AsyncOperationSPtr ResourceManagerService<TReceiverContext>::RegisterResources(
    __in vector<wstring> const & resourceNames,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    ActivityId const & activityId)
{
    return this->context_.ResourceManager.RegisterResources(resourceNames, timeout, callback, parent, activityId);
}


template<typename TReceiverContext>
AsyncOperationSPtr ResourceManagerService<TReceiverContext>::UnregisterResource(
    __in wstring const & resourceName,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    ActivityId const & activityId)
{
    return this->context_.ResourceManager.UnregisterResource(resourceName, timeout, callback, parent, activityId);
}

template<typename TReceiverContext>
AsyncOperationSPtr ResourceManagerService<TReceiverContext>::UnregisterResources(
    __in vector<wstring> const & resourceNames,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    ActivityId const & activityId)
{
    return this->context_.ResourceManager.UnregisterResources(resourceNames, timeout, callback, parent, activityId);
}

template class Management::ResourceManager::ResourceManagerService<Transport::IpcReceiverContext>;
template class Management::ResourceManager::ResourceManagerService<Federation::RequestReceiverContext>;
