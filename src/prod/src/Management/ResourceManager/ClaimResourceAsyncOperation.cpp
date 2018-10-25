// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

using namespace Management::ResourceManager;

template<typename TReceiverContext>
ClaimResourceAsyncOperation<TReceiverContext>::ClaimResourceAsyncOperation(
    Management::ResourceManager::Context & context,
    Transport::MessageUPtr requestMsg,
    TReceiverContextUPtr receiverContext,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : ClaimBasedAsyncOperation<TReceiverContext>(
        context,
        move(requestMsg),
        move(receiverContext),
        timeout,
        callback,
        parent)
{}

template<typename TReceiverContext>
void ClaimResourceAsyncOperation<TReceiverContext>::Execute(
    AsyncOperationSPtr const & thisSPtr)
{
    if (!this->ValidateRequestAndRetrieveClaim(thisSPtr))
    {
        return;
    }

    this->ExecutionContext.ResourceManager.ClaimResource(
        this->Claim,
        this->RemainingTime,
        [this, thisSPtr](AsyncOperationSPtr const & operationSPtr) 
        { 
            this->ClaimResourceCallback(thisSPtr, operationSPtr); 
        },
        thisSPtr,
        this->ActivityId);
}

template<typename TReceiverContext>
void ClaimResourceAsyncOperation<TReceiverContext>::ClaimResourceCallback(
    AsyncOperationSPtr const & thisSPtr,
    AsyncOperationSPtr const & operationSPtr)
{
    auto error = AsyncOperation::End(operationSPtr);

    if (error.IsSuccess())
    {
        ResourceMetadata metadata;

        error = this->ExecutionContext.QueryResourceMetadataFunction(this->Claim.ResourceId.ResourceName, this->ActivityId, metadata);

        if (error.IsSuccess())
        {
            this->SetReply(make_unique<ResourceMetadata>(metadata));
        }
    }

    this->Complete(thisSPtr, error);
}

template class Management::ResourceManager::ClaimResourceAsyncOperation<Transport::IpcReceiverContext>;
template class Management::ResourceManager::ClaimResourceAsyncOperation<Federation::RequestReceiverContext>;
