// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using namespace Common;

ComOperationStream::ComOperationStream(
    OperationStreamSPtr && operationStream)
    :   ComUnknownBase(),
        IFabricOperationStream2(),
        operationStream_(std::move(operationStream)),
        root_(operationStream_.get()->shared_from_this())
{
}

HRESULT ComOperationStream::BeginGetOperation(
    /* [in] */ __in IFabricAsyncOperationCallback *callback,
    /* [out, retval] */ __out IFabricAsyncOperationContext ** context)
{
    if (callback == NULL || context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    ComPointer<GetOperationOperation> getOperation = make_com<GetOperationOperation>(*(operationStream_.get()));

    HRESULT hr = getOperation->Initialize(root_, callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    ComAsyncOperationContextCPtr baseOperation;
    baseOperation.SetNoAddRef(getOperation.DetachNoRelease());
    hr = baseOperation->Start(baseOperation);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    *context = baseOperation.DetachNoRelease();
    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT ComOperationStream::EndGetOperation(
    /* [in] */ __in IFabricAsyncOperationContext *context,
    /* [out, retval] */ __out IFabricOperation ** operation)
{
    return ComUtility::OnPublicApiReturn(GetOperationOperation::End(context, operation));
}

HRESULT ComOperationStream::BeginGetBatchOperation(
    /* [in] */ __in IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ __out IFabricAsyncOperationContext ** context)
{
    UNREFERENCED_PARAMETER(callback);
    UNREFERENCED_PARAMETER(context);
    return ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

HRESULT ComOperationStream::EndGetBatchOperation(
    /* [in] */ __in IFabricAsyncOperationContext * context,
    /* [out, retval] */ __out IFabricBatchOperation ** batchOperation)
{
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(batchOperation);
    return ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

HRESULT ComOperationStream::ReportFault(
    /* [in] */ __in FABRIC_FAULT_TYPE faultType)
{
    if (faultType != FABRIC_FAULT_TYPE_TRANSIENT &&
        faultType != FABRIC_FAULT_TYPE_PERMANENT)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    ErrorCode error = operationStream_->ReportFault(faultType);

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}

} // end namespace ReplicationComponent
} // end namespace Reliability
