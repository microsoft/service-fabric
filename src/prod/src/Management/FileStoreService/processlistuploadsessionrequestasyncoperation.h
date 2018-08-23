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
        class ProcessListUploadSessionRequestAsyncOperation
            : public ProcessRequestAsyncOperation
        {
            DENY_COPY(ProcessListUploadSessionRequestAsyncOperation)

        public:
            ProcessListUploadSessionRequestAsyncOperation(
                __in RequestManager & requestManager,
                UploadSessionRequest && request,
                Transport::IpcReceiverContextUPtr && receiverContext,
                Common::ActivityId const & activityId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual ~ProcessListUploadSessionRequestAsyncOperation();

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
            class ListUploadSessionBySessionIdAsyncOperation;
            friend class ListUploadSessionBySessionIdAsyncOperation;

            class ListUploadSessionByRelativePathAsyncOperation;
            friend class ListUploadSessionByRelativePathAsyncOperation;

            std::vector<Management::FileStoreService::UploadSessionInfo> uploadSessions_;
            std::wstring storeRelativePath_;
            Common::Guid sessionId_;
        };
    }
}
