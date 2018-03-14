// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
#if !defined(PLATFORM_UNIX)
#include "Management/HttpTransport/HttpTransport.Client.h"
#include "Management/HttpTransport/HttpTransport.Server.h"
#include "Management/HttpTransport/HttpClient.h"
using namespace HttpClient;
using namespace HttpCommon;
#endif

class TelemetryClient::SendEventsAsyncOperation : public AsyncOperation
{
    DENY_COPY(SendEventsAsyncOperation)

public:
    SendEventsAsyncOperation(
        __in TelemetryClient & owner,
        std::vector<TelemetryEvent> && events,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        events_(move(events)),
        timeoutHelper_(timeout)
    {
    }

    virtual ~SendEventsAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<SendEventsAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
#if defined(PLATFORM_UNIX)
        // Telemetry client does not work on Linux for now.
        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::NotImplemented));
        return;
#else
        HttpClientImpl::CreateHttpClient(
            L"TelemetryEventSender",
            this->owner_.Root,
            httpClient_);

        pendingOperationCount_.store((uint64)events_.size());

        for (auto & event : events_)
        {
            SendEvent(thisSPtr, move(event));
        }
#endif
    }
#if !defined(PLATFORM_UNIX)
    void SendEvent(AsyncOperationSPtr const & thisSPtr, TelemetryEvent && event)
    {
        IHttpClientRequestSPtr clientRequest;
        auto timeout = timeoutHelper_.GetRemainingTime();

        auto error = httpClient_->CreateHttpRequest(
            owner_.telemetryUri_,
            timeout,
            timeout,
            timeout,
            clientRequest);

        if (!error.IsSuccess())
        {
            DecrementAndCheckPendingOperations(thisSPtr, error);
            return;
        }
        ByteBufferUPtr requestBody = nullptr;

        Envelope message = Envelope::CreateTelemetryMessage(move(event), owner_.instrumentationKey_);
        JsonHelper::Serialize(message, requestBody);

        string jsonData;

        for (auto a : *requestBody)
        {
            jsonData += (char)a;
        }

        clientRequest->SetVerb(*HttpConstants::HttpPostVerb);
        clientRequest->SetRequestHeader(*HttpConstants::ContentTypeHeader, L"application/json");
        auto operation = clientRequest->BeginSendRequest(
            move(requestBody),
            [clientRequest, this](AsyncOperationSPtr const &operation)
        {
            this->OnRequestCompleted(clientRequest, operation, false);
        },
            thisSPtr);
        OnRequestCompleted(clientRequest, operation, true);
    }

    void OnRequestCompleted(
        IHttpClientRequestSPtr const & clientRequest,
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ULONG winHttpError;
        auto error = clientRequest->EndSendRequest(operation, &winHttpError);
        if (!error.IsSuccess())
        {
            DecrementAndCheckPendingOperations(operation->Parent, error);
        }
        GetHttpResponse(clientRequest, operation->Parent);
    }

    void GetHttpResponse(IHttpClientRequestSPtr const & clientRequest, AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = clientRequest->BeginGetResponseBody(
            [this, clientRequest](AsyncOperationSPtr const &operation)
        {
            this->OnGetHttpResponseCompleted(clientRequest, operation, false);
        },
            thisSPtr);
        this->OnGetHttpResponseCompleted(clientRequest, operation, true);
    }

    void OnGetHttpResponseCompleted(IHttpClientRequestSPtr const & clientRequest, AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ByteBufferUPtr body;
        auto error = clientRequest->EndGetResponseBody(operation, body);
        USHORT statusCode;
        wstring description;
        clientRequest->GetResponseStatusCode(&statusCode, description);
        auto thisSPtr = operation->Parent;
        if (!error.IsSuccess())
        {
            DecrementAndCheckPendingOperations(thisSPtr, error);
        }
        if (statusCode != 200)
        {
            DecrementAndCheckPendingOperations(thisSPtr, ErrorCodeValue::InvalidState);
        }
        DecrementAndCheckPendingOperations(thisSPtr, ErrorCodeValue::Success);
    }
    void DecrementAndCheckPendingOperations(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode const & error)
    {
        if (!error.IsSuccess())
        {
            AcquireExclusiveLock lock(lock_);
            lastError_.ReadValue();
            lastError_ = error;
        }
        if (pendingOperationCount_.load() == 0)
        {
            Assert::CodingError("Pending operation count is already 0");
        }
        uint64 pendingOperationCount = --pendingOperationCount_;

        if (pendingOperationCount == 0)
        {
            TryComplete(thisSPtr, lastError_);
        }
    }
#endif

private:
#if !defined(PLATFORM_UNIX)
    HttpClient::IHttpClientSPtr httpClient_;
#endif
    TelemetryClient & owner_;
    TimeoutHelper timeoutHelper_;
    vector<TelemetryEvent> events_;
    atomic_uint64 pendingOperationCount_;
    Common::RwLock lock_;
    ErrorCode lastError_;
};


TelemetryClient::TelemetryClient(std::wstring const & instrumentationKey, std::wstring const & telemetryUri, ComponentRoot const & root)
    : RootedObject(root),
    instrumentationKey_(instrumentationKey),
    telemetryUri_(telemetryUri)
{
}

Common::AsyncOperationSPtr TelemetryClient::BeginReportTelemetry(
    std::vector<TelemetryEvent> && events,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<SendEventsAsyncOperation>(
        *this,
        move(events),
        timeout,
        callback,
        parent);
}

Common::ErrorCode TelemetryClient::EndReportTelemetry(
    Common::AsyncOperationSPtr const & operation)
{
    return SendEventsAsyncOperation::End(operation);
}
