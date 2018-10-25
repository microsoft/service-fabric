// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class RequestManager;
        class ProcessCommitUploadSessionRequestAsyncOperation
            : public ProcessRequestAsyncOperation
        {
            DENY_COPY(ProcessCommitUploadSessionRequestAsyncOperation)

        public:
            ProcessCommitUploadSessionRequestAsyncOperation(
                __in RequestManager & requestManager,
                UploadSessionRequest && request,
                Transport::IpcReceiverContextUPtr && receiverContext,
                Common::ActivityId const & activityId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual ~ProcessCommitUploadSessionRequestAsyncOperation();

            __declspec(property(get = get_SessionId)) Common::Guid const & SessionId;
            Common::Guid const & get_SessionId() const { return sessionId_; }

            virtual void WriteTrace(__in Common::ErrorCode const &error) override;

        protected:

            Common::AsyncOperationSPtr BeginOperation(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndOperation(
                __out Transport::MessageUPtr & reply,
                Common::AsyncOperationSPtr const & asyncOperation);

            Common::ErrorCode ValidateRequest();

        private:

            class CommitUploadSessionAsyncOperation;

            Common::Guid sessionId_;
        };
    }
}
