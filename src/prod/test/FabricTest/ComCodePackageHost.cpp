// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;

HRESULT STDMETHODCALLTYPE ComCodePackageHost::BeginActivate( 
    LPCWSTR codePackageId,
    IFabricCodePackageActivationContext * activationContext,
    IFabricRuntime * fabricRuntime,
    IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    if (activationContext == NULL || fabricRuntime == NULL ||context == NULL) { return E_POINTER; }

    ComPointer<Hosting2::ComCodePackageActivationContext> codePackageActivationContext;
    codePackageActivationContext.SetAndAddRef(static_cast<Hosting2::ComCodePackageActivationContext*>(activationContext));
    ComPointer<IFabricRuntime> runtime;
    runtime.SetAndAddRef(fabricRuntime);
    wstring instanceId = wformatString(DateTime::Now().Ticks);

    TestCodePackageContext testCodePackageContext(
        FABRICHOSTSESSION.NodeId, 
        codePackageActivationContext->Test_CodePackageActivationContextObj.CodePackageInstanceId, 
        instanceId,
        L"1.0",// Dont support upgrade yet for high density host.
        TestMultiPackageHostContext(FABRICHOSTSESSION.NodeId, hostId_));

    FABRICHOSTSESSION.Dispatcher.ActivateCodePackage(
        codePackageId,
        codePackageActivationContext,
        runtime,
        testCodePackageContext);

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();

    HRESULT hr = operation->Initialize(
        ComponentRootSPtr(),
        callback);
    if (FAILED(hr)) { return hr; }

    ComAsyncOperationContextCPtr baseOperation;
    baseOperation.SetNoAddRef(operation.DetachNoRelease());
    hr = baseOperation->Start(baseOperation);
    if (FAILED(hr)) { return hr; }

    *context = baseOperation.DetachNoRelease();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ComCodePackageHost::EndActivate( 
    IFabricAsyncOperationContext *context)
{
    if (context == NULL) { return E_POINTER; }

    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    return hr;
}

HRESULT STDMETHODCALLTYPE ComCodePackageHost::BeginDeactivate( 
    LPCWSTR codePackageId,
    IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    if (context == NULL) { return E_POINTER; }

    FABRICHOSTSESSION.Dispatcher.DeactivateCodePackage(codePackageId);

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();

    HRESULT hr = operation->Initialize(
        ComponentRootSPtr(),
        callback);
    if (FAILED(hr)) { return hr; }

    ComAsyncOperationContextCPtr baseOperation;
    baseOperation.SetNoAddRef(operation.DetachNoRelease());
    hr = baseOperation->Start(baseOperation);
    if (FAILED(hr)) { return hr; }

    *context = baseOperation.DetachNoRelease();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ComCodePackageHost::EndDeactivate( 
    IFabricAsyncOperationContext *context)
{
    if (context == NULL) { return E_POINTER; }

    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    return hr;
}
