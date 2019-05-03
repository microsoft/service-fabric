// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class RequestMessageContext::SendResponseAsyncOperation
        : public Common::AsyncOperation
    {
        DENY_COPY(SendResponseAsyncOperation);

    public:

        SendResponseAsyncOperation(
            Common::ErrorCode operationStatus,
            Common::ByteBufferUPtr bodyUPtr,
            RequestMessageContext const & messageContext,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::AsyncOperation(callback, parent)
            , bodyUPtr_(std::move(bodyUPtr))
            , operationStatus_(operationStatus)
            , messageContext_(messageContext)
            , statusCode_(0)
        {
        }

        SendResponseAsyncOperation(
            USHORT statusCode,
            std::wstring const& statusDescription,
            Common::ByteBufferUPtr bodyUPtr,
            RequestMessageContext const & messageContext,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::AsyncOperation(callback, parent)
            , bodyUPtr_(std::move(bodyUPtr))
            , statusCode_(statusCode)
            , statusDescription_(statusDescription)
            , messageContext_(messageContext)
        {
        }

        static Common::ErrorCode End(__in Common::AsyncOperationSPtr const & asyncOperation);

    protected:

        void OnStart(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCancel();

    private:
       
        RequestMessageContext const & messageContext_;
        Common::AsyncOperationSPtr thisSPtr_;
        Common::ByteBufferUPtr bodyUPtr_;
        Common::ErrorCode operationStatus_;
        USHORT statusCode_;
        std::wstring statusDescription_;
    };
}
