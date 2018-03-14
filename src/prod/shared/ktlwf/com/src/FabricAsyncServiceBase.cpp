// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "FabricAsyncServiceBase.h"
#include "Common/Common.h"
#include "KComUtility.h"

using namespace Ktl::Com;


//*** FabricAsyncServiceBase implementation
FabricAsyncServiceBase::FabricAsyncServiceBase()
    :   _OpenResult(E_PENDING),
        _CloseResult(E_PENDING)
{
}

FabricAsyncServiceBase::~FabricAsyncServiceBase()
{
}

VOID
FabricAsyncServiceBase::OnServiceReuse()
{
    _OpenResult = E_PENDING;
    _CloseResult = E_PENDING;
}

BOOLEAN 
FabricAsyncServiceBase::CompleteOpen(__in HRESULT Hr)
{
    return __super::CompleteOpen((SUCCEEDED(Hr) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL), &Hr);
}

BOOLEAN 
FabricAsyncServiceBase::CompleteOpen(__in ErrorCode Ec)
{
    return CompleteOpen(Ec.ToHResult());
}

BOOLEAN 
FabricAsyncServiceBase::CompleteOpen(__in Common::ErrorCodeValue::Enum Ec)
{
    return CompleteOpen(ErrorCode(Ec));
}

BOOLEAN 
FabricAsyncServiceBase::CompleteClose(__in HRESULT Hr)
{
    return __super::CompleteClose((SUCCEEDED(Hr) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL), &Hr);
}

BOOLEAN 
FabricAsyncServiceBase::CompleteClose(__in ErrorCode Ec)
{
    return CompleteClose(Ec.ToHResult());
}

BOOLEAN 
FabricAsyncServiceBase::CompleteClose(__in Common::ErrorCodeValue::Enum Ec)
{
    return CompleteClose(ErrorCode(Ec));
}

VOID 
FabricAsyncServiceBase::OnUnsafeOpenCompleteCalled(void* HResultPtr)
{
    HRESULT previousValue = _OpenResult;
    ASSERT_IFNOT(E_PENDING == previousValue, "Context has been reused without calling FabricAsyncServiceBase::Reuse");
    ASSERT_IFNOT(HResultPtr != nullptr, "Derivation did not call a FabricAsyncServiceBase::CompleteOpen() method");

    _OpenResult = *((HRESULT*)HResultPtr);
}

VOID 
FabricAsyncServiceBase::OnUnsafeCloseCompleteCalled(void* HResultPtr)
{
    HRESULT previousValue = _CloseResult;
    ASSERT_IFNOT(E_PENDING == previousValue, "Context has been reused without calling FabricAsyncServiceBase::Reuse");
    ASSERT_IFNOT(HResultPtr != nullptr, "Derivation did not call a FabricAsyncServiceBase::CompleteClose() method");

    _CloseResult = *((HRESULT*)HResultPtr);
}



//*** ServiceOpenCloseAdapter implementation
ServiceOpenCloseAdapter::ServiceOpenCloseAdapter()
{
}

ServiceOpenCloseAdapter::ServiceOpenCloseAdapter(__in KAsyncServiceBase& Service)
    :   _Service(&Service)
{
}

ServiceOpenCloseAdapter::~ServiceOpenCloseAdapter()
{
}

NTSTATUS 
ServiceOpenCloseAdapter::Create(
    __in KAllocator& Allocator, 
    __in KAsyncServiceBase& Service, 
    __out ServiceOpenCloseAdapter::SPtr& Adapter)
{
    Adapter = _new(COMMON_ALLOCATION_TAG, Allocator) ServiceOpenCloseAdapter(Service);
    if (!Adapter)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Adapter->Status();
    if (!NT_SUCCESS(status))
    {
        Adapter = nullptr;
        return status;
    }

    return STATUS_SUCCESS;
}

HRESULT STDMETHODCALLTYPE 
ServiceOpenCloseAdapter::Cancel()
{
    return Common::ComUtility::OnPublicApiReturn(S_OK);
}

VOID 
ServiceOpenCloseAdapter::OnOperationCompleted(
    __in KAsyncContextBase *const Parent,
    __in KAsyncServiceBase& Service,
    __in NTSTATUS OpStatus)
{
    UNREFERENCED_PARAMETER(Parent);
    UNREFERENCED_PARAMETER(Service);
    UNREFERENCED_PARAMETER(OpStatus);

    ProcessOperationCompleted();
}

