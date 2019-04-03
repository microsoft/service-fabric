// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ServiceImplementation.h" 
#include "Util.h"

using namespace NativeServiceGroupMember;

ServiceImplementation::ServiceImplementation(wstring const & partnerUri) :
partnerUri_(partnerUri)
{
}

ServiceImplementation::~ServiceImplementation()
{
}

STDMETHODIMP ServiceImplementation::BeginOpen( 
    __in IFabricStatelessServicePartition *partition,
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    if (partition == nullptr || callback == nullptr || context == nullptr)
    {
        return E_POINTER;
    }

    try
    {
        auto hr = partition->QueryInterface(IID_IFabricServiceGroupPartition, serviceGroupPartition_.VoidInitializationAddress());
        if (FAILED(hr)) { return hr; }

        ComPointer<IFabricAsyncOperationContext> dummy = make_com<DummyAsyncContext, IFabricAsyncOperationContext>();

        hr = dummy->QueryInterface(context);
        if (FAILED(hr)) { return hr; }

        callback->Invoke(dummy.GetRawPointer());
        return hr;
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

STDMETHODIMP ServiceImplementation::EndOpen( 
    __in IFabricAsyncOperationContext *context,
    __out IFabricStringResult **serviceAddress)
{
    if (context == nullptr)
    {
        return E_POINTER;
    }

    try
    {
        ComPointer<IFabricStringResult> address = make_com<StringResult, IFabricStringResult>();
        return address->QueryInterface(serviceAddress);
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

STDMETHODIMP ServiceImplementation::BeginClose( 
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    if (callback == nullptr || context == nullptr)
    {
        return E_POINTER;
    }

    try
    {
        ComPointer<IFabricAsyncOperationContext> dummy = make_com<DummyAsyncContext, IFabricAsyncOperationContext>();

        auto hr = dummy->QueryInterface(context);
        if (FAILED(hr)) { return hr; }

        callback->Invoke(dummy.GetRawPointer());
        return hr;
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

STDMETHODIMP ServiceImplementation::EndClose( 
    __in IFabricAsyncOperationContext *context)
{
    UNREFERENCED_PARAMETER(context);
    return S_OK;
}

STDMETHODIMP_(void) ServiceImplementation::Abort()
{
}

// ISimpleAdder
STDMETHODIMP ServiceImplementation::Add( 
    __in LONG first,
    __in LONG second,
    __out LONG *result)
{
    if (serviceGroupPartition_.GetRawPointer() == nullptr)
    {
        return E_UNEXPECTED;
    }

    if (result == nullptr)
    {
        return E_POINTER;
    }

    try
    {
        //
        // negative resolve, should fail
        //

        ComPointer<ISimpleAdder> dummy;

        wstring empty;
        wstring notUrl(L"test");
        wstring notExisting(L"fabric:/invalidapplication/invalidservice#invalidmember");

        auto hr = serviceGroupPartition_->ResolveMember(empty.c_str(), IID_ISimpleAdder, dummy.VoidInitializationAddress());
        if (SUCCEEDED(hr))
        {
            return E_UNEXPECTED;
        }

        hr = serviceGroupPartition_->ResolveMember(notUrl.c_str(), IID_ISimpleAdder, dummy.VoidInitializationAddress());
        if (SUCCEEDED(hr))
        {
            return E_UNEXPECTED;
        }

        hr = serviceGroupPartition_->ResolveMember(notExisting.c_str(), IID_ISimpleAdder, dummy.VoidInitializationAddress());
        if (SUCCEEDED(hr))
        {
            return E_UNEXPECTED;
        }

        hr = serviceGroupPartition_->ResolveMember(NULL, IID_ISimpleAdder, dummy.VoidInitializationAddress());
        if (SUCCEEDED(hr))
        {
            return E_UNEXPECTED;
        }

        hr = serviceGroupPartition_->ResolveMember(partnerUri_.c_str(), IID_ISimpleAdder, NULL);
        if (SUCCEEDED(hr))
        {
            return E_UNEXPECTED;
        }

        //
        // positive resolve
        //

        ComPointer<ISimpleAdder> other;

        hr = serviceGroupPartition_->ResolveMember(partnerUri_.c_str(), IID_ISimpleAdder, other.VoidInitializationAddress());
        if (FAILED(hr)) { return hr; }

        return other->Add(first, second, result);
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

