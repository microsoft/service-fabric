// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace TXRStatefulServiceBase;

StringLiteral const TraceComponent("TestClientRequestHandler");

TestClientRequestHandler::TestClientRequestHandler(
    __in HttpRequestProcessCallback const & postRequestCallback)
    : postRequestCallback_(postRequestCallback)
{
}

TestClientRequestHandlerSPtr TestClientRequestHandler::Create(
    __in HttpRequestProcessCallback const & postRequestCallback)
{
    return shared_ptr<TestClientRequestHandler>(new TestClientRequestHandler(postRequestCallback));
}

class TestClientRequestHandler::HandleRequestAsyncOperation
    : public AsyncOperation
{
public:
    HandleRequestAsyncOperation(
        __in TestClientRequestHandler & owner,
        __in HttpServer::IRequestMessageContextUPtr request,
        __in AsyncCallback const& callback,
        __in AsyncOperationSPtr const& parent)
        : AsyncOperation(callback, parent)
        , request_(move(request))
        , owner_(owner)
    {
    }

    static ErrorCode HandleRequestAsyncOperation::End(AsyncOperationSPtr const &operation)
    {
        HandleRequestAsyncOperation * op = AsyncOperation::End<HandleRequestAsyncOperation>(operation);
        return op->Error;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const &thisSPtr)
    {
        if (request_->GetVerb() != *HttpCommon::HttpConstants::HttpPostVerb &&
            request_->GetVerb() != *HttpCommon::HttpConstants::HttpGetVerb)
        {
            ByteBufferUPtr emptyBody;

            Trace.WriteError(
                TraceComponent,
                "Test Server only expects GET or POST Http requests. Received {0}",
                request_->GetVerb());

            OnSendResponse(thisSPtr, ErrorCodeValue::InvalidArgument, move(emptyBody));
            return;
        }

        request_->BeginGetMessageBody(
            [this](AsyncOperationSPtr const &operation)
            {
                OnGetMessageBodyComplete(operation);
            },
            thisSPtr);
    }

    void OnGetMessageBodyComplete(AsyncOperationSPtr const &operation)
    {
        ByteBufferUPtr body;
        ErrorCode error = request_->EndGetMessageBody(operation, body);

        if (!error.IsSuccess())
        {
            Trace.WriteError(
                TraceComponent,
                "Test Server Get Body Failed with error {0}",
                error);

            ByteBufferUPtr emptyBody;
            OnSendResponse(operation->Parent, ErrorCodeValue::InvalidArgument, move(emptyBody));
            return;
        }

        ByteBufferUPtr responseBody;
        if (request_->GetVerb() == *HttpCommon::HttpConstants::HttpPostVerb)
        {
            error = owner_.postRequestCallback_(move(body), responseBody);
        }
        else
        {
            Assert::CodingError(
                "Only post requests supported in test client request handler. Got verb {0}", 
                request_->GetVerb());
        }

        Trace.WriteInfo(
            TraceComponent,
            "Test Server Processing Callback Returned error {0}",
            error);

        if (!error.IsSuccess())
        {
            ByteBufferUPtr emptyBody;
            OnSendResponse(operation->Parent, error, move(emptyBody));
            return;
        }

        SendResponse(operation->Parent, move(responseBody));
    }

    void SendResponse(AsyncOperationSPtr const& thisSPtr, ByteBufferUPtr && bodyBytes)
    {
        request_->SetResponseHeader(HttpCommon::HttpConstants::ContentTypeHeader, L"application/json");
        OnSendResponse(thisSPtr, ErrorCodeValue::Success, move(bodyBytes));
    }

    void OnSendResponse(AsyncOperationSPtr const &thisSPtr, ErrorCode error, ByteBufferUPtr body)
    {
        request_->BeginSendResponse(
            error,
            move(body),
            [error, this](AsyncOperationSPtr const &operation)
            {
                ErrorCode error1 = ErrorCode::FirstError(error, request_->EndSendResponse(operation));

                if (!error1.IsSuccess())
                {
                    printf("EndSendResponse failed with %d\n", error.ReadValue());
                }

                TryComplete(operation->Parent, error1);
            },
            thisSPtr);
    }

private:
    TestClientRequestHandler & owner_;
    HttpServer::IRequestMessageContextUPtr request_;
};

AsyncOperationSPtr TestClientRequestHandler::BeginProcessRequest(
    __in HttpServer::IRequestMessageContextUPtr request,
    __in AsyncCallback const& callback,
    __in AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<HandleRequestAsyncOperation>(*this, move(request), callback, parent);
}

ErrorCode TestClientRequestHandler::EndProcessRequest(__in AsyncOperationSPtr const& operation)
{
    return HandleRequestAsyncOperation::End(operation);
}
