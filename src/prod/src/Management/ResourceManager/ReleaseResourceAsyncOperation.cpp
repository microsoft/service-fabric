// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

using namespace Management::ResourceManager;

template<typename TReceiverContext>
ReleaseResourceAsyncOperation<TReceiverContext>::ReleaseResourceAsyncOperation(
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
void ReleaseResourceAsyncOperation<TReceiverContext>::Execute(AsyncOperationSPtr const & thisSPtr)
{
    if (!this->ValidateRequestAndRetrieveClaim(thisSPtr))
    {
        return;
    }

    this->ExecutionContext.ResourceManager.ReleaseResource(
        this->Claim,
        this->RemainingTime,
        [this, thisSPtr](AsyncOperationSPtr const & operationSPtr)
        {
            this->ReleaseResourceCallback(thisSPtr, operationSPtr);
        },
        thisSPtr,
        this->ActivityId);
}

template<typename TReceiverContext>
void ReleaseResourceAsyncOperation<TReceiverContext>::ReleaseResourceCallback(
    AsyncOperationSPtr const & thisSPtr,
    AsyncOperationSPtr const & operationSPtr)
{
    auto error = AsyncOperation::End(operationSPtr);
    this->Complete(thisSPtr, error);
}

template class Management::ResourceManager::ReleaseResourceAsyncOperation<Transport::IpcReceiverContext>;
template class Management::ResourceManager::ReleaseResourceAsyncOperation<Federation::RequestReceiverContext>;
