// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{    
    namespace FileStoreService
    {
        class ProcessRequestAsyncOperation 
            : public Common::AsyncOperation
            , public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::FileStoreService>
        {
            DENY_COPY(ProcessRequestAsyncOperation)

        protected:
            typedef enum 
            {
                Upload,
                Delete,
                List,
                InternalList,
                Copy,
                ListUploadSession,
                DeleteUploadSession,
                UploadChunk,
                CommitUploadSession,
                CreateUploadSession,
                CheckExistence,
                UploadChunkContent
            }OperationKind;

        public:
            ProcessRequestAsyncOperation(
                RequestManager & requestManager /*parent is RequestManager*/,
                std::wstring const & storeRelativePath,
                OperationKind const operationKind,
                Transport::IpcReceiverContextUPtr && receiverContext,
                Common::ActivityId const & activityId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            virtual ~ProcessRequestAsyncOperation();

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & operation,
                __out Transport::IpcReceiverContextUPtr & receiverContext,
                __out Transport::MessageUPtr & reply,
                __out Common::ActivityId & activityId);

            virtual void WriteTrace(__in Common::ErrorCode const &) { };

            bool IsCommitUploadSessionOperation() const { return operationKind_ == OperationKind::CommitUploadSession; }

            bool IsUploadChunkOperation() const { return operationKind_ == OperationKind::UploadChunk; }

            bool IsUploadChunkContentOperation() const { return operationKind_ == OperationKind::UploadChunkContent; }

            bool IsCreateUploadSessionOperation() const { return operationKind_ == OperationKind::CreateUploadSession; }

            bool IsDeleteUploadSessionOperation() const { return operationKind_ == OperationKind::DeleteUploadSession; }

        protected:
            __declspec(property(get=get_RequestManager)) RequestManager & RequestManagerObj;
            RequestManager & get_RequestManager() const { return this->requestManager_; }

            __declspec(property(get = get_ReplicatedStoreWrapper)) ReplicatedStoreWrapper & ReplicatedStoreWrapperObj;
            inline ReplicatedStoreWrapper & get_ReplicatedStoreWrapper() const { return *(this->requestManager_.ReplicaStoreWrapperUPtr); };

            __declspec(property(get=get_StoreRelativePath)) std::wstring const & StoreRelativePath;
            inline std::wstring const & get_StoreRelativePath() const { return storeRelativePath_; };

            __declspec(property(get = get_NormalizedStoreRelativePath)) std::wstring NormalizedStoreRelativePath;
            inline std::wstring get_NormalizedStoreRelativePath() const
            {
                if (storeRelativePath_.empty())
                {
                    return storeRelativePath_;
                }
                else
                {
#if !defined(PLATFORM_UNIX)
                    std::wstring lowerCasedString = storeRelativePath_;
                    Common::StringUtility::ToLower(lowerCasedString);
                    return lowerCasedString + L"\\";
#else
                    return storeRelativePath_ + L"/";
#endif
                }
            };

            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

            virtual Common::ErrorCode ValidateRequest() { return Common::ErrorCodeValue::Success; }

            virtual Common::AsyncOperationSPtr BeginOperation(
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) = 0;

            virtual Common::ErrorCode EndOperation(
                __out Transport::MessageUPtr & reply,
                Common::AsyncOperationSPtr const & asyncOperation) = 0;

            virtual void OnRequestCompleted(__inout  Common::ErrorCode & error);

            void ConvertObjectClosedErrorCode(__inout Common::ErrorCode & error);

            Store::StoreTransaction CreateTransaction();

            Common::TimeoutHelper timeoutHelper_;

        protected:
            virtual bool DeleteIfMetadataNotInStableState(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            void OnOperationCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void CompleteOperation(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode & errorCode);
            void NormalizeStoreRelativePath();

            static bool IsWriteOperation(OperationKind operationKind);
            virtual Common::ErrorCode DeleteMetadata(Store::StoreTransaction const & storeTx, FileStoreService::FileMetadata const & metadata);
            virtual void OnDeleteMetadataCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously, FileStoreService::FileMetadata const & metadata);

        private:
            RequestManager & requestManager_;
            std::wstring storeRelativePath_;
            OperationKind const operationKind_;
            Transport::IpcReceiverContextUPtr receiverContext_;
            Transport::MessageUPtr reply_;
            Common::ManualResetEvent metadataDeletionCompleted_;
        };
    }
}
