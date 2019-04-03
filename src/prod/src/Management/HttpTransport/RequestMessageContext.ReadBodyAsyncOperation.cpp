// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpServer;
using namespace HttpCommon;

StringLiteral const HttpRequestReadBody("HttpRequestReadBody");

void RequestMessageContext::ReadBodyAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    StartRead(thisSPtr);
}

void RequestMessageContext::ReadBodyAsyncOperation::StartRead(AsyncOperationSPtr const& thisSPtr)
{
    auto status = KBuffer::Create(
        HttpConstants::DefaultEntityBodyChunkSize,
        currentBodyChunk_,
        messageContext_.GetThisAllocator(),
        HttpConstants::AllocationTag);

    if (!NT_SUCCESS(status))
    {
        TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
        return;
    }

    KMemRef body(currentBodyChunk_->QuerySize(), currentBodyChunk_->GetBuffer());
    messageContext_.BeginGetMessageBodyChunk(
        body,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnReadComplete(operation);
        },
        thisSPtr);
}

void RequestMessageContext::ReadBodyAsyncOperation::OnReadComplete(AsyncOperationSPtr const& operation)
{
    KMemRef memRef;
    auto error = messageContext_.EndGetMessageBodyChunk(operation, memRef);

    if (error.IsSuccess())
    {
        //
        // If we have met the max buffer size, and still have not seen EOF, then do not post a new read.
        // Fail this read.
        //
        if (bufferArray_.Count() * HttpConstants::DefaultEntityBodyChunkSize >=
            HttpConstants::MaxEntityBodySize) // TODO: This should be changed to settings.xml
        {
            WriteWarning(
                HttpRequestReadBody,
                "Entity body too large for url {0}",
                messageContext_.GetUrl());

            TryComplete(operation->Parent, ErrorCodeValue::MessageTooLarge);
        }
        else
        {
            // Trim the buffer to the exact size read.
            currentBodyChunk_->SetSize(memRef._Param, TRUE);

            bufferArray_.Append(move(currentBodyChunk_));
            StartRead(operation->Parent);
        }
    }
    else if (error.ReadValue() == static_cast<ErrorCodeValue::Enum>(HRESULT_FROM_NT(STATUS_END_OF_FILE)))
    {
        error = CollateBody();
        TryComplete(operation->Parent, error);
    }
    else
    {
        WriteWarning(
            HttpRequestReadBody,
            "Read failed failed with for url {0} with {1}",
            messageContext_.GetUrl(),
            error);

        TryComplete(operation->Parent, error);
    }
}

Common::ErrorCode RequestMessageContext::ReadBodyAsyncOperation::CollateBody()
{
    //
    // Depending on how many buffers are there in the buffer array, allocate the body bytebuffer
    // and start copying them over.
    //
    ULONG bufferSize = 0;
    for (ULONG i = 0; i < bufferArray_.Count(); i++)
    {
        auto hr = ULongAdd(bufferSize, bufferArray_[i]->QuerySize(), &bufferSize);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
    }

    body_ = make_unique<ByteBuffer>();
    body_->resize(bufferSize);

    auto bufferPointer = (*body_).data();
    for (ULONG i = 0; i < bufferArray_.Count(); i++)
    {
        memcpy_s(bufferPointer, bufferArray_[i]->QuerySize(), bufferArray_[i]->GetBuffer(), bufferArray_[i]->QuerySize());
        bufferPointer += bufferArray_[i]->QuerySize();
    }

    return ErrorCodeValue::Success;
}

void RequestMessageContext::ReadBodyAsyncOperation::OnCancel()
{
}

ErrorCode RequestMessageContext::ReadBodyAsyncOperation::End(
    __in Common::AsyncOperationSPtr const & asyncOperation,
    __out Common::ByteBufferUPtr &body)
{
    auto operation = AsyncOperation::End<ReadBodyAsyncOperation>(asyncOperation);

    if (operation->Error.IsSuccess())
    {
        body = move(operation->body_);
    }
    return operation->Error;
}
