// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;

#define CHECK_INJECT_FAILURE(a, b) \
    if (service_.ShouldFailOn(a, b)) return E_FAIL

#define CHECK_INJECT_REPORTFAULT_PERMANENT_TRANSIENT(a, b)                           \
    if (service_.ShouldFailOn(a, b, ApiFaultHelper::ReportFaultPermanent))           \
    {                                                                                   \
        service_.Partition->ReportFault(FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_PERMANENT);\
    };                                                                                  \
    if (service_.ShouldFailOn(a, b, ApiFaultHelper::ReportFaultTransient))           \
    {                                                                                   \
        service_.Partition->ReportFault(FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_TRANSIENT);\
    };                                                                                  \


SGComStatelessService::SGComStatelessService(SGStatelessService & service) :
    service_(service),
    root_(service.shared_from_this())
{
    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatelessService:::SGComStatelessService: ({0}) - ctor - service({1})",
        this,
        &service);
}
        
SGComStatelessService::~SGComStatelessService()
{
    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatelessService::~SGComStatelessService ({0}) - dtor",
        this);
}   

HRESULT STDMETHODCALLTYPE SGComStatelessService::BeginOpen( 
    IFabricStatelessServicePartition *servicePartition,
    IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    if (NULL == servicePartition || NULL == callback || NULL == context) { return E_POINTER; }

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatelessService::BeginOpen ({0}) - partition({1})",
        this,
        servicePartition);

    CHECK_INJECT_FAILURE(ApiFaultHelper::Service, L"BeginOpen");

    HRESULT hr = E_FAIL;

    if (!service_.ShouldFailOn(ApiFaultHelper::Service, L"EndOpen"))
    {
        hr = service_.OnOpen(servicePartition);
    }

    CHECK_INJECT_REPORTFAULT_PERMANENT_TRANSIENT(ApiFaultHelper::Service, L"BeginOpen")    

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    hr = operation->Initialize(hr, root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT STDMETHODCALLTYPE SGComStatelessService::EndOpen( 
    IFabricAsyncOperationContext *context,
    __out IFabricStringResult **serviceEndpoint)
{
    if (context == NULL || serviceEndpoint == NULL) { return E_POINTER; }

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatelessService::EndOpen ({0})",
        this);

    HRESULT hr = ComCompletedAsyncOperationContext::End(context);

    CHECK_INJECT_REPORTFAULT_PERMANENT_TRANSIENT(ApiFaultHelper::Service, L"EndOpen")    

    if (SUCCEEDED(hr))
    {
        ComPointer<IFabricStringResult> serviceName = make_com<ComStringResult, IFabricStringResult>(service_.ServiceName + L"%%##");
        hr = serviceName->QueryInterface(IID_IFabricStringResult, reinterpret_cast<void**>(serviceEndpoint));
        TestSession::FailTestIf(FAILED(hr), "QueryInterface");
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE SGComStatelessService::BeginClose( 
    IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    if (context == NULL) { return E_POINTER; }

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatelessService::BeginClose ({0})",
        this);

    CHECK_INJECT_FAILURE(ApiFaultHelper::Service, L"BeginClose");

    HRESULT hr = E_FAIL;
    if (!service_.ShouldFailOn(ApiFaultHelper::Service, L"EndClose"))
    {
        hr = service_.OnClose();
    }

    CHECK_INJECT_REPORTFAULT_PERMANENT_TRANSIENT(ApiFaultHelper::Service, L"BeginClose")
   
    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    hr = operation->Initialize(hr, root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT STDMETHODCALLTYPE SGComStatelessService::EndClose( 
    IFabricAsyncOperationContext *context)
{
    if (context == NULL) { return E_POINTER; }
    HRESULT hr = ComCompletedAsyncOperationContext::End(context);

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatelessService::EndClose ({0})",
        this);

    CHECK_INJECT_REPORTFAULT_PERMANENT_TRANSIENT(ApiFaultHelper::Service, L"EndClose")
    
    return hr;
}

void STDMETHODCALLTYPE SGComStatelessService::Abort()
{
    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatelessService::Abort ({0})",
        this);

    service_.OnAbort();
}

StringLiteral const SGComStatelessService::TraceSource("FabricTest.ServiceGroup.SGComStatelessService");
