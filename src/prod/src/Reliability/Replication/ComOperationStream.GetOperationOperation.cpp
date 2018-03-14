// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using namespace Common;

ComOperationStream::GetOperationOperation::GetOperationOperation(__in OperationStream & operationStream) 
    :   Common::ComAsyncOperationContext(),
        operationStream_(operationStream),
        operation_(NULL)
{
}

HRESULT ComOperationStream::GetOperationOperation::End(
    __in IFabricAsyncOperationContext * context, 
    __out IFabricOperation ** operation)
{
    if (context == NULL || operation == NULL) { return E_POINTER; }
    ComPointer<ComOperationStream::GetOperationOperation> asyncOperation(
        context, CLSID_ComOperationStream_GetOperationOperation);
    HRESULT hr = asyncOperation->ComAsyncOperationContext::End();
    if (FAILED(hr)) { return hr; }
    *operation = asyncOperation->operation_; 

    return asyncOperation->Result;
}

 HRESULT STDMETHODCALLTYPE ComOperationStream::GetOperationOperation::Initialize(
    Common::ComponentRootSPtr const & rootSPtr,
    __in_opt IFabricAsyncOperationCallback * callback)
{
    HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
    return hr;
}

void ComOperationStream::GetOperationOperation::OnStart(
    Common::AsyncOperationSPtr const & proxySPtr)
{
    auto inner = this->operationStream_.BeginGetOperation(
        [this](AsyncOperationSPtr const & asyncOperation) { this->GetOperationCallback(asyncOperation); },
        proxySPtr);

    if (inner->CompletedSynchronously)
    {
        this->FinishGetOperation(inner);
    }
}

void ComOperationStream::GetOperationOperation::GetOperationCallback(
    AsyncOperationSPtr const & asyncOperation)
{
    if (!asyncOperation->CompletedSynchronously)
    {
        this->FinishGetOperation(asyncOperation);
    }
}

void ComOperationStream::GetOperationOperation::FinishGetOperation(
    AsyncOperationSPtr const & asyncOperation)
{
    auto error = this->operationStream_.EndGetOperation(
        asyncOperation, 
        operation_);

    this->TryComplete(asyncOperation->Parent, error.ToHResult());
}

} // end namespace ReplicationComponent
} // end namespace Reliability
