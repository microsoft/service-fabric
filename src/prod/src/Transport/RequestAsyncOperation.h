// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class RequestAsyncOperation;
    typedef std::shared_ptr<RequestAsyncOperation> RequestAsyncOperationSPtr;

    class RequestTable;

    class RequestAsyncOperation : public Common::TimedAsyncOperation
    {
        DENY_COPY(RequestAsyncOperation)

    public:

        RequestAsyncOperation(RequestTable & owner, Transport::MessageId const & messageId, Common::TimeSpan timeout, Common::AsyncCallback const & callback, Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operationSPtr, Transport::MessageUPtr & reply);

        __declspec (property(get=get_MessageId)) Transport::MessageId const & MessageId;

        Transport::MessageId const & get_MessageId() const { return this->messageId_; }

        void SetReply(MessageUPtr && reply);

    protected:

        void OnTimeout(Common::AsyncOperationSPtr const & operationSPtr);
        void OnCancel();

    private:

        RequestTable & owner_;
        Transport::MessageId messageId_;
        Transport::MessageUPtr reply_;
    };
}
