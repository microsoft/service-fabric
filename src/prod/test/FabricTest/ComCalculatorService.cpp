// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;


void ComCalculatorService::CheckForReportFaultsAndDelays(ComPointer<IFabricStatelessServicePartition3> partition, ApiFaultHelper::ComponentName compName, std::wstring operationName)
{
    if (calculatorService_.ShouldFailOn(compName, operationName, ApiFaultHelper::ReportFaultPermanent))          
    {
        if (partition.GetRawPointer() != nullptr) 
        {
            partition->ReportFault(FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_PERMANENT);
        } 
    };                                                                                          
    if (calculatorService_.ShouldFailOn(compName, operationName, ApiFaultHelper::ReportFaultTransient))          
    {     
        if (partition.GetRawPointer() != nullptr) 
        {
            partition->ReportFault(FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_TRANSIENT);
        }
    };                                                                                          
    if (calculatorService_.ShouldFailOn(compName, operationName, ApiFaultHelper::Delay))               
    {         
        ::Sleep(static_cast<DWORD>(ApiFaultHelper::Get().GetApiDelayInterval().TotalMilliseconds()));
    }
}

ComCalculatorService::ComCalculatorService(CalculatorService & calculatorService)
    : root_(calculatorService.shared_from_this())
    , calculatorService_(calculatorService)
    , isClosedCalled_(false)
{
}

HRESULT STDMETHODCALLTYPE ComCalculatorService::BeginOpen( 
    IFabricStatelessServicePartition *servicePartition,
    IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    if (servicePartition == NULL || context == NULL) { return E_POINTER; }

    ComPointer<IFabricStatelessServicePartition3> partition;
    partition.SetAndAddRef((IFabricStatelessServicePartition3*)servicePartition);

    CheckForReportFaultsAndDelays(partition, ApiFaultHelper::Service, L"BeginOpen");

    if (calculatorService_.ShouldFailOn(ApiFaultHelper::Service, L"BeginOpen")) return E_FAIL;

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = E_FAIL;
    if (calculatorService_.ShouldFailOn(ApiFaultHelper::Service, L"EndOpen")) 
    {
        hr = operation->Initialize(E_FAIL, root_, callback);
    }
    else
    {
        ErrorCode error = calculatorService_.OnOpen(partition);
        TestSession::FailTestIfNot(error.IsSuccess(), "calculatorService_.OnOpen completed with Error {0}", error);
        hr = operation->Initialize(root_, callback);
    }

    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT STDMETHODCALLTYPE ComCalculatorService::EndOpen( 
    IFabricAsyncOperationContext *context,
    __out IFabricStringResult **serviceEndpoint)
{
    if (context == NULL || serviceEndpoint == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(calculatorService_.GetPartition(), ApiFaultHelper::Service, L"EndOpen");
    
    if (serviceEndpoint == NULL) { return E_POINTER; }
    *serviceEndpoint = NULL;
    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    if FAILED(hr) return hr;

    ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult, IFabricStringResult>(calculatorService_.ServiceLocation);
    *serviceEndpoint = stringResult.DetachNoRelease();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ComCalculatorService::BeginClose( 
    IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    if (context == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(calculatorService_.GetPartition(), ApiFaultHelper::Service, L"BeginClose");
        
    HRESULT beginResult = calculatorService_.ShouldFailOn(ApiFaultHelper::Service, L"BeginClose") ? E_FAIL : S_OK;
    HRESULT endResult = calculatorService_.ShouldFailOn(ApiFaultHelper::Service, L"EndClose") ? E_FAIL : S_OK;

    if(!isClosedCalled_)
    {
        isClosedCalled_ = true;
        ErrorCode error = calculatorService_.OnClose();
        TestSession::FailTestIfNot(error.IsSuccess(), "calculatorService_.OnClose completed with Error {0}", error);
    }

    if(FAILED(beginResult)) return E_FAIL;

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = operation->Initialize(endResult, root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}


HRESULT STDMETHODCALLTYPE ComCalculatorService::EndClose( 
    IFabricAsyncOperationContext *context)
{
    if (context == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(calculatorService_.GetPartition(), ApiFaultHelper::Service, L"EndClose");
        
    return ComCompletedAsyncOperationContext::End(context);
}

void STDMETHODCALLTYPE ComCalculatorService::Abort()
{
    CheckForReportFaultsAndDelays(calculatorService_.GetPartition(), ApiFaultHelper::Service, L"abort");
    
    calculatorService_.OnAbort();
}
