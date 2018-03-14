// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;

ComProxyReplicator::OnDataLossAsyncOperation::OnDataLossAsyncOperation(
    ComProxyReplicator & owner,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) 
    :   ComProxyAsyncOperation(callback, parent, true),
        owner_(owner),
        isStateChanged_(false)
    {
    }

HRESULT ComProxyReplicator::OnDataLossAsyncOperation::BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
{
    return owner_.primary_->BeginOnDataLoss(callback, context);
}

HRESULT ComProxyReplicator::OnDataLossAsyncOperation::EndComAsyncOperation(IFabricAsyncOperationContext * context)
{
    BOOLEAN isStateChanged;

    HRESULT hr = owner_.primary_->EndOnDataLoss(context, &isStateChanged);

    if(SUCCEEDED(hr))
    {
        isStateChanged_ = isStateChanged ? true : false;
    }

    return hr;
}

ErrorCode ComProxyReplicator::OnDataLossAsyncOperation::End(AsyncOperationSPtr const & asyncOperation, __out bool & isStateChanged)
{
    auto thisPtr = AsyncOperation::End<OnDataLossAsyncOperation>(asyncOperation);
    isStateChanged = thisPtr->isStateChanged_;
    return thisPtr->Error;
}
