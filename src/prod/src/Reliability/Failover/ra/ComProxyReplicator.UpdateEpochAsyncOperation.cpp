// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

ComProxyReplicator::UpdateEpochAsyncOperation::UpdateEpochAsyncOperation(
    ComProxyReplicator & owner,
    FABRIC_EPOCH const & epoch,
    Common::AsyncCallback callback,
    Common::AsyncOperationSPtr const & parent) 
    :   Common::ComProxyAsyncOperation(callback, parent, true),
        epoch_(epoch),
        owner_(owner)
    {
        // Empty
    }

HRESULT ComProxyReplicator::UpdateEpochAsyncOperation::BeginComAsyncOperation(
    IFabricAsyncOperationCallback * callback, 
    IFabricAsyncOperationContext ** context)
{
    return owner_.replicator_->BeginUpdateEpoch(
        &epoch_, 
        callback, 
        context);
}

HRESULT ComProxyReplicator::UpdateEpochAsyncOperation::EndComAsyncOperation(IFabricAsyncOperationContext * context)
{
    return owner_.replicator_->EndUpdateEpoch(context);
}

ErrorCode ComProxyReplicator::UpdateEpochAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<UpdateEpochAsyncOperation>(asyncOperation);
    return thisPtr->Error;
}
