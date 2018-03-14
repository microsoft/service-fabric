// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "AsyncCallInAdapter.h"
#include "Common/Common.h"

using namespace Ktl::Com;

//** Common KTL-centric COM (IFabricAsyncOperationContext) adapter implementation
AsyncCallInAdapterBase::AsyncCallInAdapterBase() 
    :   _Completed(FALSE)
{
}

AsyncCallInAdapterBase::~AsyncCallInAdapterBase()
{
}

BOOLEAN STDMETHODCALLTYPE 
AsyncCallInAdapterBase::IsCompleted()
{
    return _Completed;
}

BOOLEAN STDMETHODCALLTYPE 
AsyncCallInAdapterBase::CompletedSynchronously()
{
    return FALSE;
}

HRESULT STDMETHODCALLTYPE 
AsyncCallInAdapterBase::get_Callback(__out IFabricAsyncOperationCallback** Callback)
{
    if (nullptr == Callback)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<IFabricAsyncOperationCallback> copy = _OuterAsyncCallback;
    *Callback = copy.DetachNoRelease();

    return Common::ComUtility::OnPublicApiReturn(S_OK);
}

VOID 
AsyncCallInAdapterBase::Execute()
{
    _OuterAsyncCallback->Invoke(static_cast<IFabricAsyncOperationContext*>(this));
    _OuterAsyncCallback.Release();
    OnExecuted();
    KSharedBase::Release();
}

VOID 
AsyncCallInAdapterBase::ProcessOperationCompleted()
{
    K_LOCK_BLOCK(_ThisLock)
    {
        ASSERT_IF(TRUE == _Completed, "an operation cannot be completed more than once");
        _Completed = TRUE;

        if (_WaitHandle)
        {
            //
            // Unblock synchronous waiters
            //
            _WaitHandle->SetEvent();
        }
    }

    if (_OuterAsyncCallback)
    {
        // Invoke user callback on system thread pool. The operation is completed from a KTL worker thread. 
        // This thread must not call into user code.
        GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
    }
    else
    {
        OnExecuted();
        KSharedBase::Release();
    }
}

HRESULT 
AsyncCallInAdapterBase::WaitCompleted()
{
    K_LOCK_BLOCK(_ThisLock)
    {
        if (_Completed)
        {
            return S_OK;
        }

        ASSERT_IFNOT(nullptr == _WaitHandle, "multiple waiters for the same operation");

        _WaitHandle = _new (COMMON_ALLOCATION_TAG, GetThisAllocator()) KEvent();
        if (!_WaitHandle)
        {
            return E_OUTOFMEMORY;
        }

        NTSTATUS status = _WaitHandle->Status();
        if (!NT_SUCCESS(status))
        {
            return KComUtility::ToHRESULT(status);
        }
    }

    if (_WaitHandle)
    {
        _WaitHandle->WaitUntilSet();
    }

    ASSERT_IFNOT(TRUE == _Completed, "operation is not complete after event is set");

    //
    // Always return success, even if the underlying operation failed.
    // The call to wait is wrapped inside End<> which returns the proper error value.
    //
    return S_OK;
}



//** Common KAsyncContextBase and FabricAsyncContextBase operation adapter implementation
AsyncCallInAdapter::AsyncCallInAdapter()
{
}

AsyncCallInAdapter::~AsyncCallInAdapter()
{
}

NTSTATUS 
AsyncCallInAdapter::Create(__in KAllocator& Allocator, __out AsyncCallInAdapter::SPtr& Adapter)
{
    Adapter = _new(COMMON_ALLOCATION_TAG, Allocator) AsyncCallInAdapter();
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
AsyncCallInAdapter::Cancel()
{
    ASSERT_IF(nullptr == _InnerOperation.RawPtr(), "operation has not been attached");

    _InnerOperation->Cancel();
    return Common::ComUtility::OnPublicApiReturn(S_OK);
}

VOID 
AsyncCallInAdapter::OnOperationCompleted(
    __in KAsyncContextBase * const Parent,
    __in KAsyncContextBase& Context)
{
    UNREFERENCED_PARAMETER(Parent);

    if (_InnerOperation == nullptr)
    {
        // Allow for anonymous completion contexts 
        _InnerOperation = &Context;
    }
    else
    {
        KAssert(_InnerOperation.RawPtr() == &Context);
    }

    ProcessOperationCompleted();
}

VOID 
AsyncCallInAdapter::OnExecuted()
{
    if (_InnerOpIsFabricAsyncContextBase)
    {
        // todo: may need to be scheduled on the KTL thread
        static_cast<FabricAsyncContextBase&>(*_InnerOperation).OnCallbackInvoked();
    }
}

