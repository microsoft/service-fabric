// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
    class RequestMessageContext::SendResponseAsyncOperation
        : public Common::AsyncOperation
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(SendResponseAsyncOperation);

    public:

        SendResponseAsyncOperation(
            Common::ErrorCode operationStatus,
            Common::ByteBufferUPtr bodyUPtr,
            RequestMessageContext & messageContext,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        SendResponseAsyncOperation(
            USHORT statusCode,
            std::wstring const& statusDescription,
            Common::ByteBufferUPtr bodyUPtr,
            RequestMessageContext & messageContext,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode End(__in Common::AsyncOperationSPtr const & asyncOperation);

    protected:

        void OnStart(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCancel();

    private:

        RequestMessageContext & messageContext_;
        Common::AsyncOperationSPtr thisSPtr_;

        Common::ErrorCode operationStatus_;
        USHORT statusCode_;
        std::wstring statusDescription_;
    };
}
