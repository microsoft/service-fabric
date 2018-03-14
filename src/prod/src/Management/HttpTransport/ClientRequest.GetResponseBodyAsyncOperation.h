// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpClient
{
    class ClientRequest::GetResponseBodyAsyncOperation
        : public Common::AllocableAsyncOperation
    {
    public:
        GetResponseBodyAsyncOperation(
            std::shared_ptr<ClientRequest> &clientRequest,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::AllocableAsyncOperation(callback, parent)
            , clientRequest_(clientRequest)
            , bufferArray_(Common::GetSFDefaultPagedKAllocator())
        {
        }

        static Common::ErrorCode End(__in Common::AsyncOperationSPtr const & asyncOperation, __out Common::ByteBufferUPtr &body);
        static Common::ErrorCode End(__in Common::AsyncOperationSPtr const & asyncOperation, __out KBuffer::SPtr &body);

    protected:

        void OnStart(Common::AsyncOperationSPtr const &thisSPtr) override;

    private:
        void StartRead(Common::AsyncOperationSPtr const &thisSPtr);
        void OnReadComplete(Common::AsyncOperationSPtr const &operation, bool);

        KBuffer::SPtr currentBodyChunk_;
        KArray<KBuffer::SPtr> bufferArray_;
        std::shared_ptr<ClientRequest> clientRequest_;
    };
}
