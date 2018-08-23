// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Store;

using namespace Management::ResourceManager;

StringLiteral const TraceComponent("ResourceManagerAsyncOperation");

/* ResourceManagerAsyncOperation */

template<typename TReceiverContext>
ResourceManagerAsyncOperation<TReceiverContext>::ResourceManagerAsyncOperation(
	Management::ResourceManager::Context & executionContext,
	MessageUPtr requestMsg,
	TReceiverContextUPtr receiverContext,
	TimeSpan const & timeout,
	AsyncCallback const & callback,
	AsyncOperationSPtr const & parent)
	: AutoRetryAsyncOperation<TReceiverContext>(
		executionContext.MaxRetryDelay,
		move(requestMsg), 
		move(receiverContext), 
		timeout, 
		callback, 
		parent)
	, ReplicaActivityTraceComponent<TraceTaskCodes::ResourceManager>(
		executionContext.PartitionedReplicaId,
		this->ActivityId)
	, executionContext_(executionContext)
{
}

template class Management::ResourceManager::ResourceManagerAsyncOperation<Transport::IpcReceiverContext>;
template class Management::ResourceManager::ResourceManagerAsyncOperation<Federation::RequestReceiverContext>;

/* CompletedResourceManagerAsyncOperation */

template<typename TReceiverContext>
CompletedResourceManagerAsyncOperation<TReceiverContext>::CompletedResourceManagerAsyncOperation(
	__in Management::ResourceManager::Context & executionContext,
	__in ErrorCode const & error,
	AsyncCallback const & callback,
	AsyncOperationSPtr const & parent)
	: ResourceManagerAsyncOperation<TReceiverContext>(
		executionContext,
		nullptr,
		nullptr,
		Common::TimeSpan::Zero,
		callback,
		parent)
	, completionError_(error)
{
}

template<typename TReceiverContext>
AsyncOperationSPtr CompletedResourceManagerAsyncOperation<TReceiverContext>::CreateAndStart(
	__in Management::ResourceManager::Context & context,
	__in ErrorCode const & error,
	AsyncCallback const & callback,
	AsyncOperationSPtr const & parent)
{
	return AsyncOperation::CreateAndStart<CompletedResourceManagerAsyncOperation<TReceiverContext>>(
		context,
		error,
		callback,
		parent);
}

template<typename TReceiverContext>
void CompletedResourceManagerAsyncOperation<TReceiverContext>::Execute(AsyncOperationSPtr const & thisSPtr)
{
	this->Complete(thisSPtr, this->completionError_);
}

template class Management::ResourceManager::CompletedResourceManagerAsyncOperation<Transport::IpcReceiverContext>;
template class Management::ResourceManager::CompletedResourceManagerAsyncOperation<Federation::RequestReceiverContext>;
