// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService::ProcessDeleteNameRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessDeleteNameRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);

        ProcessDeleteNameRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            bool isExplicitRequest,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:

        DEFINE_PERF_COUNTERS ( AODeleteName )

        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&);
        void PerformRequest(Common::AsyncOperationSPtr const &);
        void OnCompleted();

    private:
        // ******************************************
        // Please refer to comments for name creation
        // ******************************************

        void OnNamedLockAcquireComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartDeleteName(Common::AsyncOperationSPtr const &);

        void DeleteNameAtAuthorityOwner(Common::AsyncOperationSPtr const &);

        void OnParentNamedLockAcquireComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void OnStoreCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
        
        void OnDeleteNameAtAuthorityOwnerDone(Common::AsyncOperationSPtr const &, Common::ErrorCode &&);

        void StartResolveNameOwner(Common::AsyncOperationSPtr const &);

        void OnResolveNameOwnerComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartRequestToNameOwner(Common::AsyncOperationSPtr const &);

        void OnRequestReplyToPeerComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void FinishDeleteName(Common::AsyncOperationSPtr const &);
        
        Common::ErrorCode PrepareDeleteCurrentName(__out TransactionSPtr & txSPtr);

        Common::ErrorCode CreateRevertDeleteTransaction(__out TransactionSPtr & txSPtr);

        Common::ErrorCode CheckNameCanBeDeleted(
            __in EnumerationSPtr & enumSPtr, 
            Common::NamingUri const & name,
            std::wstring const & nameStrig,
            HierarchyNameData const & nameData);

        void RevertTentativeDelete(Common::AsyncOperationSPtr const &);

        void OnRevertTentativeComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
        
        bool IsInitialName(std::wstring const & nameString) const;

        void OnNameNotEmpty(Common::AsyncOperationSPtr const &);

        // Tracks whether the name is deleted by an explicit or implicit request (eg delete service).
        bool isExplicitRequest_;

        // Tracks whether a NameNotFound error was encountered during
        // recursive deletion up the name path on the Authority Owner.  
        // Even if a name is not found locally on the AO, we will 
        // still try deleting up the path, but return a NameNotFound 
        // error on completion.
        //
        bool nameNotFoundOnAuthorityOwner_;

        SystemServices::SystemServiceLocation nameOwnerLocation_;

        // Keep track of locked names, to release any locks on complete, 
        // in case of failures or success
        std::vector<Common::NamingUri> lockedNames_;

        // Names that must be deleted (recursively)
        PendingNamesHierarchy tentativeNames_;

        bool tryToDeleteParent_;
    };
}
