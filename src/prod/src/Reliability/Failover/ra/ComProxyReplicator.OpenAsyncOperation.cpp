// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

HRESULT ComProxyReplicator::OpenAsyncOperation::BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
{
    return owner_.replicator_->BeginOpen(callback, context);
}

HRESULT ComProxyReplicator::OpenAsyncOperation::EndComAsyncOperation(IFabricAsyncOperationContext * context)
{
    HRESULT hr;
    ComPointer<IFabricStringResult> stringResult;

    hr = owner_.replicator_->EndOpen(context, stringResult.InitializationAddress());
    if(SUCCEEDED(hr))
    {
        if (stringResult.GetRawPointer() != nullptr)
        {
            hr = StringUtility::LpcwstrToWstring(stringResult->get_String(), true /*acceptNull*/, 0 /*minSize*/, ParameterValidator::MaxEndpointSize, replicationEndpoint_);
        }
        else
        {
            replicationEndpoint_  = L"";
        }
    }

    return hr;
}

ErrorCode ComProxyReplicator::OpenAsyncOperation::End(AsyncOperationSPtr const & asyncOperation,  __out std::wstring & replicationEndpoint)
{
    auto thisPtr = AsyncOperation::End<ComProxyReplicator::OpenAsyncOperation>(asyncOperation);

    replicationEndpoint = thisPtr->replicationEndpoint_;

    return thisPtr->Error;
}
