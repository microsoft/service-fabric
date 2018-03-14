// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace Store
{
    ReplicatedStore::OnDataLossAsyncOperation::OnDataLossAsyncOperation(
        __in Api::IStoreEventHandlerPtr const & storeEventHandler,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        storeEventHandler_(storeEventHandler)
    {
        this->isStateChanged_ = false;
    }

    void ReplicatedStore::OnDataLossAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!this->storeEventHandler_.get())
        { 
            // If there is no store event handler, complete the call.
            TryComplete(thisSPtr);
            return;
        }

        this->storeEventHandler_->BeginOnDataLoss(
            [this](Common::AsyncOperationSPtr const & operation)
        {
            OnDataLossCompleted(operation);
        },
        thisSPtr);
    }

    ErrorCode ReplicatedStore::OnDataLossAsyncOperation::End(AsyncOperationSPtr const & operation, __out bool & isStateChanged)
    {
        OnDataLossAsyncOperation* onDataLossAsyncOperationPtr = AsyncOperation::End<OnDataLossAsyncOperation>(operation);

        // If storeEventHandler does not exist, isStateChanged will be false.
        if (onDataLossAsyncOperationPtr->Error.IsSuccess())
        {
            isStateChanged = onDataLossAsyncOperationPtr->isStateChanged_;
        }

        return onDataLossAsyncOperationPtr->Error;
    }

    void ReplicatedStore::OnDataLossAsyncOperation::OnDataLossCompleted(AsyncOperationSPtr const & operation)
    {
        auto error = this->storeEventHandler_->EndOnDataLoss(operation, this->isStateChanged_);
        TryComplete(operation->Parent, error);
    }
}

