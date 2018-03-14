// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Naming;

ComPropertyBatchResult::ComPropertyBatchResult(
    IPropertyBatchResultPtr const& impl)
    : ComUnknownBase()
    , impl_(impl)
{
}

HRESULT STDMETHODCALLTYPE ComPropertyBatchResult::GetProperty(
    ULONG operationIndexInRequest,
    __out IFabricPropertyValueResult ** property)
{
    if (property == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    NamePropertyResult result;
    auto error = impl_->GetProperty(operationIndexInRequest, result);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    auto comPointer = make_com<ComNamedProperty>(std::move(result));
    *property = comPointer.DetachNoRelease();
    return S_OK;
}
