// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    CompletedAsyncOperation::CompletedAsyncOperation(ErrorCode const & error, AsyncCallback const & callback, AsyncOperationSPtr const& parent)
        : AsyncOperation(callback, parent),
          completionError_(error)
    {
    }

    CompletedAsyncOperation::CompletedAsyncOperation(AsyncCallback const & callback, AsyncOperationSPtr const& parent)
        : AsyncOperation(callback, parent),
        completionError_(ErrorCode())
    {
    }

    CompletedAsyncOperation::~CompletedAsyncOperation(void)
    {
    }

    void CompletedAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TryComplete(thisSPtr, completionError_);
    }

    ErrorCode CompletedAsyncOperation::End(AsyncOperationSPtr const& asyncOperation)
    {
        return AsyncOperation::End<CompletedAsyncOperation>(asyncOperation)->Error;
    }
}
