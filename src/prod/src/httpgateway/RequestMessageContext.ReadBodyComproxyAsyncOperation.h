// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class RequestMessageContext::ReadBodyAsyncOperation
        : public Common::AsyncOperation
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(ReadBodyAsyncOperation);

    public:

        ReadBodyAsyncOperation(
            RequestMessageContext const & messageContext,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::AsyncOperation(callback, parent)
            , messageContext_(messageContext)
            , bufferArray_(GetSFDefaultPagedKAllocator())
        {
        }

        static Common::ErrorCode End(
            __in Common::AsyncOperationSPtr const & asyncOperation,
            __out Common::ByteBufferUPtr &body);

    protected:

        void OnStart(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCancel();

    private:

        void StartRead(Common::AsyncOperationSPtr const& operation);
        void OnReadComplete(Common::AsyncOperationSPtr const& operation);
        Common::ErrorCode CollateBody();

        Common::ByteBufferUPtr body_;
        RequestMessageContext const & messageContext_;
        KArray<KBuffer::SPtr> bufferArray_; 
    };
}
