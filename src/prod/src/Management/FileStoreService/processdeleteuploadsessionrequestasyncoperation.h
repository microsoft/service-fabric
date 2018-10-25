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
        class ProcessDeleteUploadSessionRequestAsyncOperation 
            : public ProcessRequestAsyncOperation
        {
            DENY_COPY(ProcessDeleteUploadSessionRequestAsyncOperation)

        public:
            ProcessDeleteUploadSessionRequestAsyncOperation(
                __in RequestManager & requestManager,
                DeleteUploadSessionRequest && deleteRequest,
                Transport::IpcReceiverContextUPtr && receiverContext,
                Common::ActivityId const & activityId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual ~ProcessDeleteUploadSessionRequestAsyncOperation();

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

            class DeleteUploadSessionAsyncOperation;
            friend class DeleteUploadSessionAsyncOperation;

            Common::Guid sessionId_;
        };
    }
}
