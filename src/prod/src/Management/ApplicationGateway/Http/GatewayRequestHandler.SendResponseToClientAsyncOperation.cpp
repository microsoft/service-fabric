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

void GatewayRequestHandler::SendResponseToClientAsyncOperation::OnStart(AsyncOperationSPtr const &thisSPtr)
{
    //
    // Get all headers from the Service response and send them.
    //
    auto error = SetHeaders();
    if (!error.IsSuccess())
    {
        failurePhase_ = L"SetResponseHeaders";
        TryComplete(thisSPtr, error);
        return;
    }

    SendHeaders(thisSPtr);
}

void GatewayRequestHandler::SendResponseToClientAsyncOperation::SendHeaders(AsyncOperationSPtr const &thisSPtr)
{
    USHORT statusCode;
    wstring statusDescription;
    requestToService_->GetResponseStatusCode(&statusCode, statusDescription);

    auto sendHeadersOperation = requestFromClient_->BeginSendResponseHeaders(
        statusCode,
        statusDescription,
        true, // We havent read the body from the service's response yet, so assuming body exists.
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnSendHeadersComplete(operation, false);
    },
        thisSPtr);

    OnSendHeadersComplete(sendHeadersOperation, true);
}

void GatewayRequestHandler::SendResponseToClientAsyncOperation::OnSendHeadersComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = requestFromClient_->EndSendResponseHeaders(operation);
    if (!error.IsSuccess())
    {
        failurePhase_ = L"SendResponseHeaders";
        OnError(operation->Parent, error);
        return;
    }

    //
    // Read and forward the body.
    //

    KBuffer::Create(
        gateway_.GetGatewayConfig().BodyChunkSize,
        bodyBuffer_,
        requestFromClient_->GetThisAllocator());

    ReadAndForwardBodyChunk(operation->Parent);
}

void GatewayRequestHandler::SendResponseToClientAsyncOperation::ReadAndForwardBodyChunk(AsyncOperationSPtr const &thisSPtr)
{
    auto buffer = GetReadBuffer();
    auto operation = requestToService_->BeginGetResponseChunk(
        buffer,
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnReadBodyChunkComplete(operation, false);
    },
        thisSPtr);

    OnReadBodyChunkComplete(operation, true);
}

void GatewayRequestHandler::SendResponseToClientAsyncOperation::OnReadBodyChunkComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    KMemRef currentChunk;
    auto error = requestToService_->EndGetResponseChunk(operation, currentChunk, &winHttpError_);
    if (!error.IsSuccess())
    {
        // Obtaining the response chunk from the service failed
        // We have already sent the HTTP headers by now so it is too late to send HTTP status code for this failure.
        // Disconnect the client gracefully because something went wrong with the requestToService (Service disconnected the connection etc)

        failurePhase_ = L"GetResponseChunkFromService";
        OnDisconnect(operation->Parent, winHttpError_);
        return;
    }

    if (currentChunk._Param == 0)
    {
        isLastSegment_ = true;
        currentChunk = GetEndOfFileBuffer();
    }
    else
    {
        currentChunk = GetBufferForForwarding(currentChunk);
    }

    auto forwardBodyOperation = requestFromClient_->BeginSendResponseChunk(
        currentChunk,
        isLastSegment_, // isLastSegment
        false, //disconnect
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnForwardBodyChunkComplete(operation, false);
    },
        operation->Parent);

    OnForwardBodyChunkComplete(forwardBodyOperation, true);
}

void GatewayRequestHandler::SendResponseToClientAsyncOperation::OnForwardBodyChunkComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = requestFromClient_->EndSendResponseChunk(operation);
    if (!error.IsSuccess())
    {
        failurePhase_ = L"SendResponseChunk";
        OnError(operation->Parent, error);
        return;
    }

    if (isLastSegment_)
    {
        //
        // This means we have indicated that we have sent the last segment.
        //
        TryComplete(operation->Parent, error);
        return;
    }

    //
    // Get and send any additional body chunks.
    //
    ReadAndForwardBodyChunk(operation->Parent);
}

ErrorCode GatewayRequestHandler::SendResponseToClientAsyncOperation::End(
    __in AsyncOperationSPtr const & asyncOperation, 
    __out ULONG *winHttpError,
    __out wstring & failurePhase)
{
    auto thisPtr = AsyncOperation::End<SendResponseToClientAsyncOperation>(asyncOperation);

    if (!thisPtr->Error.IsSuccess())
    {
        *winHttpError = thisPtr->winHttpError_;
        failurePhase = thisPtr->failurePhase_;
    }

    return thisPtr->Error;
}

ErrorCode GatewayRequestHandler::SendResponseToClientAsyncOperation::SetHeaders()
{
    KString::SPtr headers;
    auto error = requestToService_->GetAllResponseHeaders(headers);
    if (!error.IsSuccess())
    {
        return error;
    }

    KString::SPtr transferEncodingHeaderValue;
    error = requestToService_->GetResponseHeader(HttpConstants::TransferEncodingHeader, transferEncodingHeaderValue);
    if (error.IsSuccess())
    {
        ULONG offset;
        if ((transferEncodingHeaderValue->Search(KStringView(HttpConstants::ChunkedTransferEncoding->c_str()), offset)))
        {
            chunkedEncoding_ = true;

            HttpApplicationGatewayEventSource::Trace->FoundChunkedTransferEncodingHeaderInResponse(
                traceId_);
        }
    }

    auto & headersToRemove = gateway_.GetGatewayConfig().ResponseHeadersToRemove();
    // If no response headers to remove, send all the headers as is
    if (headersToRemove.empty())
    {
        requestFromClient_->SetAllResponseHeaders(headers);
    }
    else // Remove the headers listed in the RemoveServiceResponseHeaders config (default = remove Date & Server header)
    {
        // Get the list of all header-value pairs
        StringCollection headerCollection;
        wstring headerString(*headers);
        StringUtility::Split<wstring>(headerString, headerCollection, L"\r\n", true);

        // Iterate through the headers and if it is in the RemoveServiceResponseHeaders list, skip it. Otherwise add it to the response headers
        // On windows, Http.sys will add the Server, Date headers by default in the response.
        // Note: When reverse proxy goes cross platform, this might need additional logic of adding the Date and Server headers explicitly
        // if the underlying libraries do not already add them.
        for (wstring const &headerItem : headerCollection)
        {
            wstring headerName;
            wstring headerValue;

            StringUtility::SplitOnce<wstring, wchar_t>(headerItem, headerName, headerValue, L':');

            StringUtility::TrimSpaces(headerName);
            StringUtility::TrimSpaces(headerValue);

            // If header is not in response headers to remove list, add it to the response
            if (headersToRemove.find(headerName) == headersToRemove.end())
            {
                requestFromClient_->SetResponseHeader(headerName, headerValue);
            }
        }
    }

    return ErrorCodeValue::Success;
}

void GatewayRequestHandler::SendResponseToClientAsyncOperation::OnError(AsyncOperationSPtr const &thisSPtr, ErrorCode error)
{
    // TODO: THis just disconnects the request to service to prevent leaks, this should also clean
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

void GatewayRequestHandler::SendResponseToClientAsyncOperation::OnDisconnect(AsyncOperationSPtr const& thisSPtr, ULONG winHttpError)
{
    HttpApplicationGatewayEventSource::Trace->ReceiveResponseError(
        traceId_,
        failurePhase_,
        winHttpError);

    KString::SPtr clientAddress;
    requestFromClient_->GetRemoteAddress(clientAddress);

    HttpApplicationGatewayEventSource::Trace->ReceiveResponseErrorWithDetails(
        traceId_,
        requestFromClient_->GetUrl(),
        requestFromClient_->GetVerb(),
        wstring(*clientAddress),
        requestStartTime_,
        requestToService_->GetRequestUrl(),
        failurePhase_,
        winHttpError);

    auto buffer = KMemRef(0, nullptr);
    AsyncOperationSPtr operation = requestFromClient_->BeginSendResponseChunk(
        buffer,
        true, // isLastSegment
        true, // disconnect
        [this](AsyncOperationSPtr const& operation)
        {
            this->requestFromClient_->EndSendResponseChunk(operation);
            this->TryComplete(operation->Parent, ErrorCodeValue::Success);
        },
        thisSPtr);
}

KMemRef GatewayRequestHandler::SendResponseToClientAsyncOperation::GetReadBuffer()
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

KMemRef GatewayRequestHandler::SendResponseToClientAsyncOperation::GetEndOfFileBuffer()
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

KMemRef GatewayRequestHandler::SendResponseToClientAsyncOperation::GetBufferForForwarding(KMemRef &memRef)
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
