// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService::ProcessCreateNameRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessCreateNameRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);

        ProcessCreateNameRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            bool isExplicitRequest,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:

        DEFINE_PERF_COUNTERS( AOCreateName )

        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&);
        void PerformRequest(Common::AsyncOperationSPtr const &);
        void OnCompleted();
        
    private:
        
        // ***************************************************
        //
        // Steps:
        //
        // 1) Acquire named lock for this name
        // 2) Write tentative state for the name
        //  2b) Create parent name on NameNotFound error
        // 3) Resolve the location of the name owner
        // 4) Tell the name owner to create the name
        // 5) Write completed state for the name
        // 6) Release named lock for this name
        //
        // If step 1-2 fails, we simply complete with the error
        // since there are no consistency issues between the 
        // Authority Owner (AO) and Name Owner (NO).
        // We can optimize by locally retrying on expected errors
        // (such as write conflicts), but there are no functional
        // issues with letting the client retry.
        //
        // In step 2b), CreateName will recursively create the parent names along this
        // name's path if they do not exist. Note that the parent names are
        // created one at a time starting from the earliest non-existent parent
        // name. i.e. Supposing no names are in the system and we try
        // to create "fabric:/a/b/c". We will first consistently create
        // "fabric:/a". Once that completes successfully, we will create
        // "fabric:/a/b" and so on.  Therefore, only leaf names along a path 
        // can ever be in a tentative state.
        //
        // If step 3-4 fails, we retry from step 3. We must continue
        // retrying until we know we have reached a consistent state
        // between the AO and NO or until a primary failover has 
        // occurred on this partition. Note that while retrying
        // to achieve a consistent state, we continue to hold 
        // the named lock and will not be honoring the async operation timeout
        // since we cannot allow another request to operate on this name
        // until consistency has been achieved.
        //
        // During primary recovery, we will also retry if step 2 fails since
        // the tentative state was successfully written from a previous
        // request and consistency was not achieved before failover occurred.
        //
        // If step 5 fails, we retry step 5 until we succeed or
        // a failover occurs.
        //
        // Note that in steps 2 and 5, the named lock on
        // the parent name is temporarily acquired.
        //
        // This same logical progression of steps applies to 
        // name deletion, service creation, and service deletion.
        //
        // ***************************************************

        void OnNamedLockAcquireComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartCreateName(Common::AsyncOperationSPtr const &);
       
        void WriteNameAtAuthorityOwner(Common::AsyncOperationSPtr const &);
                
        void OnParentNamedLockAcquireComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void OnStoreCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
        
        void OnWriteNameAtAuthorityOwnerDone(Common::AsyncOperationSPtr const &, Common::ErrorCode &&);

        void StartResolveNameOwner(Common::AsyncOperationSPtr const &);

        void OnResolveNameOwnerComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartRequestToNameOwner(Common::AsyncOperationSPtr const &);

        void OnRequestReplyToPeerComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void FinishCreateName(Common::AsyncOperationSPtr const &);

        Common::ErrorCode PrepareWriteCurrentName(__out TransactionSPtr & txSPtr);

        bool IsInitialName(std::wstring const & nameString) const;

        // Tracks whether the name is created by an explicit or implicit request (eg. create service).
        bool isExplicitRequest_;

        SystemServices::SystemServiceLocation nameOwnerLocation_;

        // Keep track of locked names, to release any locks on complete, 
        // in case of failures or success
        std::vector<Common::NamingUri> lockedNames_;
        
        // Tentative names that must be created
        PendingNamesHierarchy tentativeNames_;

        bool authorityNameAlreadyCompleted_;
    };
}
