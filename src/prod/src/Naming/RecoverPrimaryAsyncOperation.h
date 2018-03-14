// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService;
    class NamingStore;

    class RecoverPrimaryAsyncOperation 
        : public Common::AsyncOperation
        , public ActivityTracedComponent<Common::TraceTaskCodes::NamingStoreService>
    {
    public:
        RecoverPrimaryAsyncOperation(
            __in StoreService & replica,
            __in NamingStore & store,
            StoreServiceProperties const & properties,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);

        virtual ~RecoverPrimaryAsyncOperation();

        static void End(Common::AsyncOperationSPtr const & operation);

    protected:
        void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        void OnCompleted() override;

    private:

        void SchedulePendingRequestCheck(Common::AsyncOperationSPtr const &, Common::TimeSpan const & delay);
        void ScheduleRecovery(Common::AsyncOperationSPtr const &, Common::TimeSpan const & delay);

        void RecoveryTimerCallback(Common::AsyncOperationSPtr const & thisSPtr);
        void InitializeRootNames(Common::AsyncOperationSPtr const & thisSPtr);
        void OnCommitComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        void LoadRecoveryOperations(Common::AsyncOperationSPtr const & thisSPtr);
        Common::ErrorCode LoadPsdCache(TransactionSPtr const &);
        Common::ErrorCode LoadAuthorityOwnerRecoveryOperations(TransactionSPtr const &);

        void StartRecoveryOperations(Common::AsyncOperationSPtr const &);
        void OnProcessingComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
        void FinishRecoveryOperation(Common::AsyncOperationSPtr const &, Common::ErrorCode const &);

        void HandleError(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const &);

        StoreService & replica_;
        NamingStore & store_;
        StoreServiceProperties const & properties_;
        Common::TimerSPtr retryTimer_;
        Common::ExclusiveLock timerLock_;
        Common::atomic_long pendingRecoveryOperationCount_;
        std::vector<Transport::MessageUPtr> pendingRecoveryRequests_;
        Common::atomic_long failedRecoveryOperationCount_;
    };

    typedef std::shared_ptr<RecoverPrimaryAsyncOperation> RecoverPrimaryAsyncOperationSPtr;
}
