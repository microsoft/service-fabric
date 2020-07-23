// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace HttpServer;
using namespace HttpClient;
using namespace HttpCommon;
using namespace HttpApplicationGateway;
using namespace Transport;

void GatewayRequestHandler::SendRequestToServiceAsyncOperation::OnStart(AsyncOperationSPtr const &thisSPtr)
{
    //
    // Get all headers from the client request and send them.
    //
    auto error = SetHeaders();
    if (!error.IsSuccess())
    {
        failurePhase_ = L"SetHeadersInRequest";
        TryComplete(thisSPtr, error);
        return;
    }

    requestToService_->SetVerb(verb_);

    if (requestToService_->IsSecure())
    {
        if (owner_.gateway_.GetCertificateContext())
        {
            requestToService_->SetClientCertificate(owner_.gateway_.GetCertificateContext());
        }

        // Create a KDelegate for ValidateServerCertificate static method. The first parameter can contain any data that we want to pass to the delegate.
        // Pass the validation context containing traceId and gateway config, so that the delegate can 
        // 1. log traces with the corresponding Id.
        // 2. fetch certificate validation config options
        KHttpCliRequest::ServerCertValidationHandler handler((PVOID)&validationContext_, GatewayRequestHandler::ValidateServerCertificate);
        requestToService_->SetServerCertValidationHandler(handler);
    }

    ULONG contentLength = 0;

    if (verb_ != HttpConstants::HttpGetVerb)
    {
        //
        // There should be either Content-Length or Transfer-Encoding header, if there is body.
        //
        KString::SPtr contentLengthHeaderValue;
        auto headerName = Utility::GetHeaderNameKString(
            HttpConstants::ContentLengthHeader,
            requestFromClient_->GetThisAllocator());
        auto status = requestHeaders_.Get(headerName, contentLengthHeaderValue);

        if (NT_SUCCESS(status))
        {
            // Size should be given in decimal value
            if (!StringUtility::TryFromWString<ULONG>(wstring(*contentLengthHeaderValue, contentLengthHeaderValue->Length()), contentLength))
            {
                OnError(thisSPtr, ErrorCodeValue::NotImplemented);
                return;
            }
        }
        else
        {
            KString::SPtr transferEncodingHeaderValue;

            headerName = Utility::GetHeaderNameKString
            (HttpConstants::TransferEncodingHeader,
                requestFromClient_->GetThisAllocator());
            status = requestHeaders_.Get(
                headerName,
                transferEncodingHeaderValue);

            ULONG offset;
            if (NT_SUCCESS(status) &&
                (transferEncodingHeaderValue->Search(KStringView(HttpConstants::ChunkedTransferEncoding->c_str()), offset)))
            {
                contentLength = WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH;
                chunkedEncoding_ = true;

                HttpApplicationGatewayEventSource::Trace->FoundChunkedTransferEncodingHeaderInRequest(
                    traceId_);
            }
        }
    }

    auto operation = requestToService_->BeginSendRequestHeaders(
        contentLength,
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnSendHeadersComplete(operation, false);
    },
        thisSPtr);

    OnSendHeadersComplete(operation, true);
}

void GatewayRequestHandler::SendRequestToServiceAsyncOperation::OnSendHeadersComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = requestToService_->EndSendRequestHeaders(operation, &winHttpError_);
    if (!error.IsSuccess())
    {
        failurePhase_ = L"SendRequestHeaders";
        //
        // Headers haven't been sent, so it is safe to retry the request.
        //
        safeToRetry_ = true;
        TryComplete(operation->Parent, error);
        return;
    }

    if (verb_ == *HttpConstants::HttpGetVerb)
    {
        FinishSendRequest(operation->Parent);
    }
    else if (fullRequestBody_) //Body has been read already, send it to the service.
    {
        KMemRef memRef(static_cast<ULONG>(fullRequestBody_->size()), fullRequestBody_->data(), static_cast<ULONG>(fullRequestBody_->size()));
        FinishSendRequestWithBody(operation->Parent, memRef);
    }
    else
    {
        //
        // Read the body from the client request and send it to server.
        //

        KBuffer::Create(
            owner_.gateway_.GetGatewayConfig().BodyChunkSize,
            bodyBuffer_,
            requestFromClient_->GetThisAllocator());

        ReadAndForwardBodyChunk(operation->Parent);
    }
}

void GatewayRequestHandler::SendRequestToServiceAsyncOperation::FinishSendRequestWithBody(
    AsyncOperationSPtr const &thisSPtr,
    KMemRef &fullBodyChunk)
{
    auto buffer = GetBufferForForwarding(fullBodyChunk);
    auto operation = requestToService_->BeginSendRequestChunk(
        buffer,
        true, // Last segment
        false,
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnFinishSendRequestWithBodyComplete(operation, false);
    },
        thisSPtr);

    OnFinishSendRequestWithBodyComplete(operation, true);
}

void GatewayRequestHandler::SendRequestToServiceAsyncOperation::OnFinishSendRequestWithBodyComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = requestToService_->EndSendRequestChunk(operation, &winHttpError_);
    if (!error.IsSuccess())
    {
        failurePhase_ = L"FinishSendRequestWithBody";
        TryComplete(operation->Parent, error);
        return;
    }

    TryComplete(operation->Parent, error);
}

void GatewayRequestHandler::SendRequestToServiceAsyncOperation::ReadAndForwardBodyChunk(AsyncOperationSPtr const &thisSPtr)
{
    auto buffer = GetReadBuffer();
    auto operation = requestFromClient_->BeginGetMessageBodyChunk(
        buffer,
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnReadBodyChunkComplete(operation, false);
    },
        thisSPtr);

    OnReadBodyChunkComplete(operation, true);
}

void GatewayRequestHandler::SendRequestToServiceAsyncOperation::OnReadBodyChunkComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    KMemRef currentChunk;
    auto error = requestFromClient_->EndGetMessageBodyChunk(operation, currentChunk);
    if (!error.IsSuccess())
    {
        if (error.ReadValue() == static_cast<ErrorCodeValue::Enum>(HRESULT_FROM_NT(STATUS_END_OF_FILE)))
        {
            FinishSendRequest(operation->Parent);
            return;
        }
        else
        {
            failurePhase_ = L"GetMessageBodyChunk";
            OnError(operation->Parent, error);
            return;
        }
    }

    if (currentChunk._Param != 0)
    {
        ForwardBodyChunk(operation->Parent, currentChunk);
    }
    else
    {
        FinishSendRequest(operation->Parent);
    }
}

void GatewayRequestHandler::SendRequestToServiceAsyncOperation::ForwardBodyChunk(
    AsyncOperationSPtr const &thisSPtr,
    KMemRef &currentChunk)
{
    auto buffer = GetBufferForForwarding(currentChunk);
    auto forwardOperation = requestToService_->BeginSendRequestChunk(
        buffer,
        false,
        false,
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnForwardBodyChunkComplete(operation, false);
    },
        thisSPtr);

    OnForwardBodyChunkComplete(forwardOperation, true);
}

void GatewayRequestHandler::SendRequestToServiceAsyncOperation::OnForwardBodyChunkComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = requestToService_->EndSendRequestChunk(operation, &winHttpError_);
    if (!error.IsSuccess())
    {
        failurePhase_ = L"ForwardBodyChunk";
        TryComplete(operation->Parent, error);
        return;
    }

    //
        // Read the next chunk
        //
        ReadAndForwardBodyChunk(operation->Parent);
}

void GatewayRequestHandler::SendRequestToServiceAsyncOperation::FinishSendRequest(AsyncOperationSPtr const &thisSPtr)
{
    auto buffer = GetEndOfFileBuffer();
    auto operation = requestToService_->BeginSendRequestChunk(
        buffer,
        true,
        false,
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnFinishSendRequestComplete(operation, false);
    },
        thisSPtr);

    OnFinishSendRequestComplete(operation, true);
}

void GatewayRequestHandler::SendRequestToServiceAsyncOperation::OnFinishSendRequestComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = requestToService_->EndSendRequestChunk(operation, &winHttpError_);
    if (!error.IsSuccess())
    {
        failurePhase_ = L"FinishSendRequest";
        TryComplete(operation->Parent, error);
        return;
    }

    TryComplete(operation->Parent, error);
}

ErrorCode GatewayRequestHandler::SendRequestToServiceAsyncOperation::End(
    __in AsyncOperationSPtr const & asyncOperation,
    __out ULONG *winHttpError,
    __out bool *safeToRetry,
    __out wstring & failurePhase)
{
    auto thisPtr = AsyncOperation::End<SendRequestToServiceAsyncOperation>(asyncOperation);

    if (!thisPtr->Error.IsSuccess())
    {
        *winHttpError = thisPtr->winHttpError_;
        *safeToRetry = thisPtr->safeToRetry_;
        failurePhase = thisPtr->failurePhase_;
    }

    return thisPtr->Error;
}

ErrorCode GatewayRequestHandler::SendRequestToServiceAsyncOperation::SetHeaders()
{
    KString::SPtr headers;
    ProcessHeaders(headers);

    if (headers && headers->Length() > 0)
    {
        requestToService_->SetRequestHeaders(headers);
    }

    return ErrorCodeValue::Success;
}

//
// This function shouldnt modify the requestHeaders_ member.
// We avoid creating a copy of the headers and we need the request headers incase we retry this request.
//
void GatewayRequestHandler::SendRequestToServiceAsyncOperation::ProcessHeaders(__out KString::SPtr &headers)
{
    KString::SPtr clientAddress;
    auto error = requestFromClient_->GetRemoteAddress(clientAddress);
    clientAddress->MeasureThis();

    if (!error.IsSuccess())
    {
        HttpApplicationGatewayEventSource::Trace->GetClientAddressError(
            traceId_,
            error);
    }

    owner_.ProcessHeaders(requestHeaders_, requestFromClient_->GetThisAllocator(), clientAddress, false, traceId_, headers);
}

void GatewayRequestHandler::SendRequestToServiceAsyncOperation::OnError(AsyncOperationSPtr const &thisSPtr, ErrorCode error)
{
    // TODO: This just disconnects the request to service to prevent leaks, this should also clean
    // up the connection from the client so that this happens in one place.

    auto buffer = KMemRef(0, nullptr);
    requestToService_->BeginSendRequestChunk(
        buffer,
        false,
        true, // disconnect
        [this, error](AsyncOperationSPtr const &operation)
    {
        ULONG winHttpError;
        requestToService_->EndSendRequestChunk(operation, &winHttpError);
        TryComplete(operation->Parent, error);
    },
        thisSPtr);
}

KMemRef GatewayRequestHandler::SendRequestToServiceAsyncOperation::GetReadBuffer()
{
    if (chunkedEncoding_)
    {
        return Utility::AdjustBufferForCTE(bodyBuffer_);
    }
    else
    {
        return KMemRef(bodyBuffer_->QuerySize(), bodyBuffer_->GetBuffer());
    }
}

KMemRef GatewayRequestHandler::SendRequestToServiceAsyncOperation::GetEndOfFileBuffer()
{
    if (chunkedEncoding_)
    {
        return Utility::GetCTECompletionChunk(bodyBuffer_);
    }
    else
    {
        return KMemRef(0, nullptr);
    }
}

KMemRef GatewayRequestHandler::SendRequestToServiceAsyncOperation::GetBufferForForwarding(KMemRef &memRef)
{
    if (chunkedEncoding_)
    {
        return Utility::AddCTEInfo(memRef);
    }
    else
    {
        return memRef;
    }
}

