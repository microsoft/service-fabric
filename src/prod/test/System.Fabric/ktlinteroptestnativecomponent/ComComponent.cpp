// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace KtlInteropTest;

ComComponent::ComComponent()
{
}

ComComponent::~ComComponent()
{
}

HRESULT ComComponent::Create(
    __in KAllocator & allocator,
    __out ComPointer<IKtlInteropTestComponent> & component)
{
    ComComponent::SPtr created = _new(0, allocator) ComComponent();
    if (!created)
    {
        return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
    }

    NTSTATUS status = created->Status();
    if (!NT_SUCCESS(status))
    {
        return Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_NT(status));
    }

    component.SetAndAddRef(created.DownCast<IKtlInteropTestComponent>());

    return Common::ComUtility::OnPublicApiReturn(S_OK);
}

STDMETHODIMP ComComponent::BeginOperation( 
    __in HRESULT beginResult,
    __in HRESULT endResult,
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    if (nullptr == callback)
    {
       return Common::ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    if (nullptr == context)
    {
       return Common::ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    AsyncOperationContext::SPtr asyncContext = _new(0, GetThisAllocator()) AsyncOperationContext();
    if (!asyncContext)
    {
        return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
    }

    NTSTATUS status = asyncContext->Status();
    if (!NT_SUCCESS(status))
    {
        return Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_NT(status));
    }

    return Ktl::Com::AsyncCallInAdapter::CreateAndStart(
        GetThisAllocator(),
        Ktl::Move(asyncContext),
        callback,
        context,
        [beginResult, endResult](Ktl::Com::AsyncCallInAdapter&,
                                 AsyncOperationContext& operation, 
                                 KAsyncContextBase::CompletionCallback completionCallback) -> HRESULT
        {
            return operation.StartOperation(beginResult, endResult, nullptr, completionCallback).ToHResult();
        });
}

STDMETHODIMP ComComponent::EndOperation( 
    __in IFabricAsyncOperationContext *context)
{
    if (nullptr == context)
    {
       return Common::ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    AsyncOperationContext::SPtr asyncOperation;

    HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context, asyncOperation);

    return Common::ComUtility::OnPublicApiReturn(hr);
}

ComComponent::AsyncOperationContext::AsyncOperationContext() :
    _endResult(E_FAIL)
{
}

ComComponent::AsyncOperationContext::~AsyncOperationContext()
{
}

ErrorCode ComComponent::AsyncOperationContext::StartOperation(
    __in HRESULT beginResult,
    __in HRESULT endResult,
    __in_opt KAsyncContextBase* const parent,
    __in_opt CompletionCallback callback)
{
    if (SUCCEEDED(beginResult))
    {
        _endResult = endResult;
        Start(parent, callback);
    }
    
    return ErrorCode::FromHResult(beginResult);
}

void ComComponent::AsyncOperationContext::OnStart()
{
    Complete(_endResult);
}

void ComComponent::AsyncOperationContext::OnReuse()
{
    _endResult = E_FAIL;
}
