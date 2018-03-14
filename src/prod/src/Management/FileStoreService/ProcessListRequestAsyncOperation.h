// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{    
    namespace FileStoreService
    {        
        class ProcessListRequestAsyncOperation : public ProcessRequestAsyncOperation
        {
            DENY_COPY(ProcessListRequestAsyncOperation)

        public:
            ProcessListRequestAsyncOperation(
                __in RequestManager & requestManager,
                ListRequest && listMessage,                
                Transport::IpcReceiverContextUPtr && receiverContext,
                Common::ActivityId const & activityId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            virtual ~ProcessListRequestAsyncOperation();

        protected:            
            Common::AsyncOperationSPtr BeginOperation(                                
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndOperation(
                __out Transport::MessageUPtr & reply,
                Common::AsyncOperationSPtr const & asyncOperation);      

        private:     
            Common::ErrorCode ListMetadata();
            size_t GetContinuationTokenPosition(
                std::vector<FileMetadata> const & metadataList, 
                std::wstring const & continuationToken);
            void NormalizeContinuationToken();

        private:            
            StorePagedContentInfo content_;
            std::wstring continuationToken_;
            BOOLEAN shouldIncludeDetails_;
            BOOLEAN isRecursive_;
            bool isPaging_;
        };
    }
}
