// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpClient;
using namespace HttpCommon;

StringLiteral const TraceType("HttpRequest.GetResponseBodyAsyncOperation");

void ClientRequest::GetResponseBodyAsyncOperation::OnStart(AsyncOperationSPtr const &thisSPtr)
{
    StartRead(thisSPtr);
}

void ClientRequest::GetResponseBodyAsyncOperation::StartRead(AsyncOperationSPtr const &thisSPtr)
{
    auto status = KBuffer::Create(
        HttpConstants::DefaultEntityBodyChunkSize,
        currentBodyChunk_,
        GetSFDefaultPagedKAllocator(),
        HttpConstants::AllocationTag);

    if (!NT_SUCCESS(status))
    {
        TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
        return;
    }

    KMemRef body(currentBodyChunk_->QuerySize(), currentBodyChunk_->GetBuffer());
    auto operation = clientRequest_->BeginGetResponseChunk(
        body,
        [this](AsyncOperationSPtr const &operation)
        {
            this->OnReadComplete(operation, false);
        },
        thisSPtr);

    OnReadComplete(operation, true);
}

void ClientRequest::GetResponseBodyAsyncOperation::OnReadComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    KMemRef memRef;
    ULONG winHttpError;
    auto error = clientRequest_->EndGetResponseChunk(operation, memRef, &winHttpError);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            "GetResponseChunk failed for Url {0} with error : {1}, winHttpError : {2}",
            clientRequest_->requestUri_,
            error,
            winHttpError);

        TryComplete(operation->Parent, error);
        return;
    }

    if (memRef._Param == 0)
    {
        //
        // Reached EOF
        //
        TryComplete(operation->Parent, ErrorCodeValue::Success);
        return;
    }

    // Trim the buffer to the exact size read.
    currentBodyChunk_->SetSize(memRef._Param, TRUE);
    bufferArray_.Append(move(currentBodyChunk_));

    if (bufferArray_.Count() * HttpConstants::DefaultEntityBodyChunkSize >=
        HttpConstants::MaxEntityBodySize) // TODO: This should be changed to settings.xml
    {
        WriteWarning(
            TraceType,
            "Response body too large for url {0}",
            clientRequest_->requestUri_);

        TryComplete(operation->Parent, ErrorCodeValue::MessageTooLarge);
        return;
    }

    StartRead(operation->Parent);
}

ErrorCode ClientRequest::GetResponseBodyAsyncOperation::End(AsyncOperationSPtr const &asyncOperation, ByteBufferUPtr &body)
{
    auto thisPtr = AsyncOperation::End<GetResponseBodyAsyncOperation>(asyncOperation);

    if (!thisPtr->Error.IsSuccess())
    {
        return thisPtr->Error;
    }

    ULONG bufferSize = 0;
    for (ULONG i = 0; i < thisPtr->bufferArray_.Count(); i++)
    {
        auto hr = ULongAdd(bufferSize, thisPtr->bufferArray_[i]->QuerySize(), &bufferSize);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
    }

    body = make_unique<ByteBuffer>();
    body->resize(bufferSize);

    auto bufferPointer = (*body).data();
    for (ULONG i = 0; i < thisPtr->bufferArray_.Count(); i++)
    {
        memcpy_s(bufferPointer, thisPtr->bufferArray_[i]->QuerySize(), thisPtr->bufferArray_[i]->GetBuffer(), thisPtr->bufferArray_[i]->QuerySize());
        bufferPointer += thisPtr->bufferArray_[i]->QuerySize();
    }

    return thisPtr->Error;
}

ErrorCode ClientRequest::GetResponseBodyAsyncOperation::End(AsyncOperationSPtr const &asyncOperation, KBuffer::SPtr &body)
{
    auto thisPtr = AsyncOperation::End<GetResponseBodyAsyncOperation>(asyncOperation);

    if (!thisPtr->Error.IsSuccess())
    {
        return thisPtr->Error;
    }

    ULONG bufferSize = 0;
    for (ULONG i = 0; i < thisPtr->bufferArray_.Count(); i++)
    {
        auto hr = ULongAdd(bufferSize, thisPtr->bufferArray_[i]->QuerySize(), &bufferSize);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
    }

    auto status = KBuffer::Create(
        bufferSize, 
        body, 
        GetSFDefaultPagedKAllocator(),
        HttpConstants::AllocationTag);

    if (!NT_SUCCESS(status))
    {
        return ErrorCode::FromNtStatus(status);
    }

    auto bufferPointer = (BYTE *)body->GetBuffer();
    for (ULONG i = 0; i < thisPtr->bufferArray_.Count(); i++)
    {
        memcpy_s(bufferPointer, thisPtr->bufferArray_[i]->QuerySize(), thisPtr->bufferArray_[i]->GetBuffer(), thisPtr->bufferArray_[i]->QuerySize());
        bufferPointer += thisPtr->bufferArray_[i]->QuerySize();
    }

    return thisPtr->Error;
}
