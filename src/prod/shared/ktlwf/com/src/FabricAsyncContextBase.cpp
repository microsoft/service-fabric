// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "FabricAsyncContextBase.h"
#include "Common/Common.h"

using namespace Ktl::Com;

FabricAsyncContextBase::FabricAsyncContextBase() :
    _Result(E_PENDING)
{
}

FabricAsyncContextBase::~FabricAsyncContextBase()
{
}

BOOLEAN 
FabricAsyncContextBase::Complete(
    __in HRESULT Hr)
{
    NTSTATUS status = SUCCEEDED(Hr) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

    return KAsyncContextBase::Complete(
        KAsyncContextBase::CompletingCallback(this, &FabricAsyncContextBase::OnUnsafeCompleteCalled), 
        &Hr, 
        status);
}

VOID 
FabricAsyncContextBase::OnUnsafeCompleteCalled(void* HResultPtr)
{
    HRESULT previousValue = _Result;
    ASSERT_IFNOT(E_PENDING == previousValue, "Context has been reused without calling FabricAsyncContextBase::Reuse");

    _Result = *((HRESULT*)HResultPtr);
}

BOOLEAN
FabricAsyncContextBase::Complete(
    __in ErrorCode ErrorCode)
{
    return Complete(ErrorCode.ToHResult());
}

BOOLEAN
FabricAsyncContextBase::Complete(
    __in Common::ErrorCodeValue::Enum errorCode)
{
    return Complete(ErrorCode(errorCode));
}

VOID
FabricAsyncContextBase::OnCallbackInvoked()
{
    // empty
}

VOID
FabricAsyncContextBase::OnReuse()
{
    _Result = E_PENDING;
}
