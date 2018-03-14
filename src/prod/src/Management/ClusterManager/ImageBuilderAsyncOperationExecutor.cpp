// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace Store;
using namespace ServiceModel;
using namespace Management::ClusterManager;

static StringLiteral const TraceComponent("ImageBuilderAsyncOperationExecutor");

ImageBuilderAsyncOperationExecutor::ImageBuilderAsyncOperationExecutor(
    __in ProcessRolloutContextAsyncOperationBase & owner)
    : owner_(owner)
    , imageBuilderAsyncOperation_()
    , imageBuilderAsyncOperationLock_()
    , progressCallbackEvent_()
    , canceledCalled_(false)
{
}

ImageBuilderAsyncOperationExecutor::~ImageBuilderAsyncOperationExecutor()
{
}

void ImageBuilderAsyncOperationExecutor::AddOperation(Common::AsyncOperationSPtr const & asyncOperation)
{
    bool shouldCancel = false;
    { // lock
        AcquireWriteLock lock(imageBuilderAsyncOperationLock_);

        if (!asyncOperation->IsCompleted)
        {
            if (canceledCalled_)
            {
                shouldCancel = true;
            }
            else
            {
                imageBuilderAsyncOperation_ = asyncOperation;
            }
        }
    } // endlock

    if (shouldCancel)
    {
        owner_.WriteInfo(
            TraceComponent,
            "{0}: Add image builder operation: ExternalCancel was previously called, so cancel the operation.",
            owner_.TraceId);
        asyncOperation->Cancel();
    }
}

bool ImageBuilderAsyncOperationExecutor::OnUpdateProgressDetails(
    std::wstring const & details)
{
    { // lock
        AcquireWriteLock lock(imageBuilderAsyncOperationLock_);

        if (!this->IsImageBuilderAsyncOperationPending_UnderLock() || details.empty() || progressCallbackEvent_.get() != nullptr)
        {
            return false;
        }

        progressCallbackEvent_ = make_shared<ManualResetEvent>(false);
    } // endlock
    
    return true;
}

void ImageBuilderAsyncOperationExecutor::OnUpdateProgressDetailsComplete()
{
    AcquireWriteLock lock(imageBuilderAsyncOperationLock_);

    if (progressCallbackEvent_.get() != nullptr)
    {
        progressCallbackEvent_->Set();

        progressCallbackEvent_.reset();
    }
    else
    {
        owner_.WriteWarning(TraceComponent, "{0}: OnUpdateProgressDetailsComplete: progress callback is null", owner_.TraceId);
        Assert::TestAssert("{0}: progress callback is null", owner_.TraceId);
    }
}

void ImageBuilderAsyncOperationExecutor::WaitProgressCallback()
{
    shared_ptr<ManualResetEvent> progressCallbackEventCopy;

    { // lock
        AcquireReadLock lock(imageBuilderAsyncOperationLock_);
        progressCallbackEventCopy = progressCallbackEvent_;
    } // endlock

    if (progressCallbackEventCopy.get() != nullptr)
    {
        owner_.WriteInfo(
            TraceComponent,
            "{0} wait for progress update callback",
            owner_.TraceId);

        progressCallbackEventCopy->WaitOne();
    }
}

void ImageBuilderAsyncOperationExecutor::ExternalCancel()
{
    AsyncOperationSPtr copy;

    { // lock
        AcquireWriteLock lock(imageBuilderAsyncOperationLock_);
        copy = imageBuilderAsyncOperation_;
        canceledCalled_ = true;
    } // endlock

    if (copy)
    {
        owner_.WriteInfo(
            TraceComponent,
            "{0}: ExternalCancel",
            owner_.TraceId);
        copy->Cancel();
    }
}

void ImageBuilderAsyncOperationExecutor::ResetOperation()
{
    { // lock
        AcquireWriteLock lock(imageBuilderAsyncOperationLock_);
        imageBuilderAsyncOperation_.reset();
    } // endlock
}

bool ImageBuilderAsyncOperationExecutor::IsImageBuilderAsyncOperationPending_UnderLock() const
{
    return (imageBuilderAsyncOperation_.get() != nullptr && !imageBuilderAsyncOperation_->IsCompleted);
}
