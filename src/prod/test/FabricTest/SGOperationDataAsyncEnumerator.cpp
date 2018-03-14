// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TestCommon;
using namespace FabricTest;

//
// Constructor/Destructor.
//
SGOperationDataAsyncEnumerator::SGOperationDataAsyncEnumerator(
    __in Common::ComPointer<IFabricOperationData> && operationData,
    bool failBegin, bool failEnd, bool empty)
    : operationData_(std::move(operationData))
{
    TestSession::WriteNoise(
        TraceSource, 
        "SGOperationDataAsyncEnumerator::SGOperationDataAsyncEnumerator ({0}) - ctor - Empty({1}) FailBegin({2}) FailEnd({3})",
        this,
        empty,
        failBegin,
        failEnd);

    empty_ = empty;
    failBegin_ = failBegin;
    failEnd_ = failEnd;
    
    emptyData_ = make_com<SGEmptyOperationData, IFabricOperationData>();
}

SGOperationDataAsyncEnumerator::~SGOperationDataAsyncEnumerator(void)
{
    TestSession::WriteNoise(
        TraceSource, 
        "SGOperationDataAsyncEnumerator::SGOperationDataAsyncEnumerator ({0}) - dtor",
        this);
}

//
// IFabricOperationDataStream methods.
//
HRESULT STDMETHODCALLTYPE SGOperationDataAsyncEnumerator::BeginGetNext(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context)
{
    if (NULL == callback || NULL == context) { return E_POINTER; }
    if (failBegin_)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "SGOperationDataAsyncEnumerator::BeginGetNext ({0}) - Return E_FAIL",
            this);
        return E_FAIL;
    }

    HRESULT hr = S_OK;

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    hr = operation->Initialize(ComponentRootSPtr(), callback);
    TestSession::FailTestIf(FAILED(hr), "Initialize");
    hr = ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
    
    TestSession::WriteNoise(
        TraceSource, 
        "SGOperationDataAsyncEnumerator::BeginGetNext ({0}) - context({1})",
        this,
        *context);

    return hr;
}

HRESULT STDMETHODCALLTYPE SGOperationDataAsyncEnumerator::EndGetNext( 
    __in IFabricAsyncOperationContext* context,
    __out IFabricOperationData** operationData)
{
    if (NULL == context || NULL == operationData) { return E_POINTER; }

    *operationData = emptyData_.DetachNoRelease();
    if (!*operationData)
    {
        *operationData = operationData_.DetachNoRelease();
    }
    HRESULT hr = ComCompletedAsyncOperationContext::End(context);

    if (failEnd_)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "SGOperationDataAsyncEnumerator::EndGetNext ({0}) - context({1}) - Return E_FAIL",
            this,
            context);
        return E_FAIL;
    }

    if (empty_)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "SGOperationDataAsyncEnumerator::EndGetNext ({0}) - context({1}) - Return NULL operationData",
            this,
            context);
        if (*operationData)
        {
            (*operationData)->Release();
        }
        *operationData = NULL;
    }
    else
    {
        TestSession::WriteNoise(
            TraceSource, 
            "SGOperationDataAsyncEnumerator::EndGetNext ({0}) - context({1}) operationData({2})",
            this,
            context,
            *operationData);
    }

    return hr;
}

StringLiteral const SGOperationDataAsyncEnumerator::TraceSource("FabricTest.ServiceGroup.SGOperationDataAsyncEnumerator");
