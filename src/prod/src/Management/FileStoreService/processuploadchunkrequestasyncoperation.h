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
        class ProcessUploadChunkRequestAsyncOperation 
            : public ProcessRequestAsyncOperation
        {
            DENY_COPY(ProcessUploadChunkRequestAsyncOperation)

        public:
            ProcessUploadChunkRequestAsyncOperation(
                __in RequestManager & requestManager,
                UploadChunkRequest && uploadChunkMessage,
                Transport::IpcReceiverContextUPtr && receiverContext,
                Common::ActivityId const & activityId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual ~ProcessUploadChunkRequestAsyncOperation();

        protected:

            Common::AsyncOperationSPtr BeginOperation(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndOperation(
                __out Transport::MessageUPtr & reply,
                Common::AsyncOperationSPtr const & asyncOperation);

            Common::ErrorCode ValidateRequest();

        private:

            class UploadChunkAsyncOperation;
            friend class UploadChunkAsyncOperation;

            Common::Guid sessionId_;
            std::wstring stagingFullPath_;
            uint64 startPosition_;
            uint64 endPosition_;
          
        };
    }
}
