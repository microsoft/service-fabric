// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{    
    namespace FileStoreService
    {        
        class ProcessInternalListRequestAsyncOperation : public ProcessRequestAsyncOperation
        {
            DENY_COPY(ProcessInternalListRequestAsyncOperation)

        public:
            ProcessInternalListRequestAsyncOperation(
                __in RequestManager & requestManager,
                ListRequest && listMessage,                
                Transport::IpcReceiverContextUPtr && receiverContext,
                Common::ActivityId const & activityId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            virtual ~ProcessInternalListRequestAsyncOperation();

        protected: 
            Common::ErrorCode ValidateRequest();

            Common::AsyncOperationSPtr BeginOperation(                                
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndOperation(
                __out Transport::MessageUPtr & reply,
                Common::AsyncOperationSPtr const & asyncOperation);      

        private:
            Common::ErrorCode ListMetadata();

        private:            
            bool isPresent_;
            FileState::Enum state_;
            StoreFileVersion currentVersion_;            
        };
    }
}
