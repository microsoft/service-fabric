// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpClient
{
    class ClientRequest::SendRequestAsyncOperation
        : public Common::AllocableAsyncOperation
    {
    public:
        SendRequestAsyncOperation(
            Common::ByteBufferUPtr bodyUPtr,
            std::shared_ptr<ClientRequest> &clientRequest,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::AllocableAsyncOperation(callback, parent)
            , bodyUPtr_(std::move(bodyUPtr))
            , kBufferBody_()
            , clientRequest_(clientRequest)
            , winHttpError_(0)
            , memRef_()
        {
            if (bodyUPtr_)
            {
                memRef_._Address = bodyUPtr_->data();
                memRef_._Size = static_cast<ULONG>(bodyUPtr_->size());
                memRef_._Param = static_cast<ULONG>(bodyUPtr_->size());
            }
        }

        SendRequestAsyncOperation(
            KBuffer::SPtr &&kBufferBody,
            std::shared_ptr<ClientRequest> &clientRequest,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::AllocableAsyncOperation(callback, parent)
            , bodyUPtr_()
            , kBufferBody_(std::move(kBufferBody))
            , clientRequest_(clientRequest)
            , winHttpError_(0)
            , memRef_()
        {
            if (kBufferBody_)
            {
                memRef_._Address = kBufferBody_->GetBuffer();
                memRef_._Size = kBufferBody_->QuerySize();
                memRef_._Param = kBufferBody_->QuerySize();
            }
        }

        static Common::ErrorCode End(
            __in Common::AsyncOperationSPtr const & asyncOperation,
            __out ULONG *winHttpError);

    protected:

        void OnStart(Common::AsyncOperationSPtr const& thisSPtr) override;

    private:

        void OnSendHeadersComplete(Common::AsyncOperationSPtr const &operation, bool);
        void OnSendBodyComplete(Common::AsyncOperationSPtr const &operation, bool);

        Common::AsyncOperationSPtr thisSPtr_;
        Common::ByteBufferUPtr bodyUPtr_;
        KBuffer::SPtr kBufferBody_;
        std::shared_ptr<ClientRequest> clientRequest_;
        ULONG winHttpError_;
        KMemRef memRef_;
    };
}
