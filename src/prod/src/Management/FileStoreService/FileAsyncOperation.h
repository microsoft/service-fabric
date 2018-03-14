// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {        
        class FileAsyncOperation 
            : public Common::AsyncOperation
            , public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::FileStoreService>
        {
            DENY_COPY(FileAsyncOperation)

        public:
            FileAsyncOperation(
                __in RequestManager & requestManager,
                bool useSimpleTx,
                std::wstring const & relativeStorePath,                 
                bool useTwoPhaseCommit,
                Common::ActivityId const & activityId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            virtual ~FileAsyncOperation();

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation);

        protected:
            __declspec(property(get=get_RequestManager)) RequestManager & RequestManagerObj;
            RequestManager & get_RequestManager() const { return this->requestManager_; }

            __declspec(property(get=get_StoreRelativePath)) std::wstring const & StoreRelativePath;
            inline std::wstring const & get_StoreRelativePath() const { return storeRelativePath_; };

            __declspec(property(get=get_UseTwoPhaseCommit)) bool UseTwoPhaseCommit;
            inline bool get_UseTwoPhaseCommit() const { return useTwoPhaseCommit_; };

            __declspec(property(get = get_NormalizedStoreRelativePath)) std::wstring NormalizedStoreRelativePath;
            inline std::wstring get_NormalizedStoreRelativePath() const
            {
#if !defined(PLATFORM_UNIX)
                std::wstring lowerCasedString = storeRelativePath_;
                Common::StringUtility::ToLower(lowerCasedString);
                return lowerCasedString + L"\\";
#else
                return storeRelativePath_ + L"/";
#endif
            };

            __declspec(property(get=get_TimeoutHelper)) Common::TimeoutHelper const & TimeoutHelperObj;
            inline Common::TimeoutHelper const & get_TimeoutHelper() const { return timeoutHelper_; };

            __declspec(property(get = get_ReplicatedStoreWrapper)) ReplicatedStoreWrapper & ReplicatedStoreWrapperObj;
            inline ReplicatedStoreWrapper & get_ReplicatedStoreWrapper() const { return *(requestManager_.ReplicaStoreWrapperUPtr); };

            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);
            virtual void OnCompleted();

            virtual Common::ErrorCode TransitionToIntermediateState(Store::StoreTransaction const & storeTx) = 0;
            virtual Common::ErrorCode TransitionToReplicatingState(Store::StoreTransaction const & storeTx) = 0;
            virtual Common::ErrorCode TransitionToCommittedState(Store::StoreTransaction const & storeTx) = 0;
            virtual Common::ErrorCode TransitionToRolledbackState(Store::StoreTransaction const & storeTx) = 0;
            
            virtual Common::AsyncOperationSPtr OnBeginFileOperation(
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent) = 0;
            virtual Common::ErrorCode OnEndFileOperation(Common::AsyncOperationSPtr const &operation) = 0;

            virtual void UndoFileOperation() = 0;

            Common::ErrorCode TryScheduleRetry(
                Common::AsyncOperationSPtr const & thisSPtr, 
                std::function<void(Common::AsyncOperationSPtr const &)> const & callback);

            static bool IsSharingViolationError(Common::ErrorCode const& error);

        private:                    
            void StartTransitionToIntermediateState(Common::AsyncOperationSPtr const & thisSPtr);
            void StartTransitionToReplicatingState(Common::AsyncOperationSPtr const & thisSPtr);
            void StartTransitionToCommittedState(Common::AsyncOperationSPtr const & thisSPtr);

            void OnFileOperationCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void OnCommitToIntermediateStateCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void OnCommitToReplicatingStateCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void OnCommitToCommittedStateCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void OnCommitToRolledbackStateCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously, Common::ErrorCode const & originalError);
            void Rollback(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & originalError);                        

        private:            
            RequestManager & requestManager_;
            bool const useSimpleTx_;
            std::wstring const storeRelativePath_;
            bool const useTwoPhaseCommit_;
            Common::TimeoutHelper timeoutHelper_;
            Common::TimerSPtr retryTimer_;
            Common::ExclusiveLock timerLock_;
        };        
    }
}
