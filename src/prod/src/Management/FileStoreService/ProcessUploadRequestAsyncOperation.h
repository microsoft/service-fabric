// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{    
    namespace FileStoreService
    {
        class ProcessUploadRequestAsyncOperation : public ProcessRequestAsyncOperation
        {
            DENY_COPY(ProcessUploadRequestAsyncOperation)

        public:
            ProcessUploadRequestAsyncOperation(
                __in RequestManager & requestManager,
                UploadRequest && uploadMessage,                
                Transport::IpcReceiverContextUPtr && receiverContext,
                Common::ActivityId const & activityId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            virtual ~ProcessUploadRequestAsyncOperation();

        protected:
            Common::ErrorCode ValidateRequest();

            Common::AsyncOperationSPtr BeginOperation(                                
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndOperation(
                __out Transport::MessageUPtr & reply,
                Common::AsyncOperationSPtr const & asyncOperation);

            void OnRequestCompleted(Common::ErrorCode & error);

        private:
            bool shouldOverwrite_;
            std::wstring stagingFullPath_;
        };
    }
}
