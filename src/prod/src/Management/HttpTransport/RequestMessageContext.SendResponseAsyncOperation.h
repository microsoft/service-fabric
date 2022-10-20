// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
    class RequestMessageContext::SendResponseAsyncOperation
        : public Common::AsyncOperation
    {
        DENY_COPY(SendResponseAsyncOperation);

    public:

        SendResponseAsyncOperation(
            Common::ErrorCode operationStatus,
            Common::ByteBufferUPtr bodyUPtr,
            RequestMessageContext & messageContext,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::AsyncOperation(callback, parent)
            , bodyUPtr_(std::move(bodyUPtr))
            , kBodyBuffer_()
            , operationStatus_(operationStatus)
            , messageContext_(messageContext)
            , statusCode_(0)
            , memRef_()
        {
            if (bodyUPtr_)
            {
                memRef_._Address = bodyUPtr_->data();
                memRef_._Size = static_cast<ULONG>(bodyUPtr_->size());
                memRef_._Param = static_cast<ULONG>(bodyUPtr_->size());
            }
        }

        SendResponseAsyncOperation(
            USHORT statusCode,
            std::wstring const& statusDescription,
            Common::ByteBufferUPtr bodyUPtr,
            RequestMessageContext & messageContext,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::AsyncOperation(callback, parent)
            , bodyUPtr_(std::move(bodyUPtr))
            , kBodyBuffer_()
            , statusCode_(statusCode)
            , statusDescription_(statusDescription)
            , messageContext_(messageContext)
            , memRef_()
        {
            if (bodyUPtr_)
            {
                memRef_._Address = bodyUPtr_->data();
                memRef_._Size = static_cast<ULONG>(bodyUPtr_->size());
                memRef_._Param = static_cast<ULONG>(bodyUPtr_->size());
            }
        }

        static Common::ErrorCode End(__in Common::AsyncOperationSPtr const & asyncOperation);

    protected:

        void OnStart(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCancel();

    private:
        void OnSendResponseHeadersComplete(Common::AsyncOperationSPtr const& operation, bool);
        void OnSendBodyComplete(Common::AsyncOperationSPtr const& operation, bool);

        RequestMessageContext & messageContext_;
        Common::AsyncOperationSPtr thisSPtr_;
        Common::ByteBufferUPtr bodyUPtr_;
        KBuffer::SPtr kBodyBuffer_;
        Common::ErrorCode operationStatus_;
        USHORT statusCode_;
        std::wstring statusDescription_;
        KMemRef memRef_;
    };
}
