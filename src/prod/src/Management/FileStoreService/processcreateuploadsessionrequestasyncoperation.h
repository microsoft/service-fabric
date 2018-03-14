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
        class ProcessCreateUploadSessionRequestAsyncOperation 
            : public ProcessRequestAsyncOperation
        {
            DENY_COPY(ProcessCreateUploadSessionRequestAsyncOperation)

        public:
            ProcessCreateUploadSessionRequestAsyncOperation(
                __in RequestManager & requestManager,
                CreateUploadSessionRequest && createMessage,
                Transport::IpcReceiverContextUPtr && receiverContext,
                Common::ActivityId const & activityId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual ~ProcessCreateUploadSessionRequestAsyncOperation();

        protected:

            Common::AsyncOperationSPtr BeginOperation(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndOperation(
                __out Transport::MessageUPtr & reply,
                Common::AsyncOperationSPtr const & asyncOperation);

            Common::ErrorCode ValidateRequest();

        private:

            class CreateUploadSessionAsyncOperation;

            std::wstring storeRelativePath_;
            Common::Guid sessionId_;
            uint64 fileSize_;
        };
    }
}
