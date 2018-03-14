// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

HRESULT ComProxyStatelessService::OpenAsyncOperation::BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
{
    return owner_.service_->BeginOpen(partition_.GetRawPointer(), callback, context);
}

HRESULT ComProxyStatelessService::OpenAsyncOperation::EndComAsyncOperation(IFabricAsyncOperationContext * context)
{
    HRESULT hr;
    ComPointer<IFabricStringResult> stringResult;

    hr = owner_.service_->EndOpen(context, stringResult.InitializationAddress());

    if(SUCCEEDED(hr))
    {
        if (stringResult.GetRawPointer() != nullptr)
        {
            hr = StringUtility::LpcwstrToWstring(stringResult->get_String(), true /*acceptNull*/, 0 /*minSize*/, ParameterValidator::MaxEndpointSize, serviceLocation_);
        }
        else
        {
            serviceLocation_ = L"";
        }
    }

    return hr;
}

ErrorCode ComProxyStatelessService::OpenAsyncOperation::End(AsyncOperationSPtr const & asyncOperation, __out std::wstring & serviceLocation)
{
    auto thisPtr = AsyncOperation::End<ComProxyStatelessService::OpenAsyncOperation>(asyncOperation);

    serviceLocation = thisPtr->serviceLocation_;

    return thisPtr->Error;
}
