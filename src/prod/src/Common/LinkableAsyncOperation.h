// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class LinkableAsyncOperation;
    typedef std::shared_ptr<LinkableAsyncOperation> LinkableAsyncOperationSPtr;

    class LinkableAsyncOperation : public AsyncOperation
    {
        DENY_COPY(LinkableAsyncOperation)

    public:
        void ResumeOutsideLock(AsyncOperationSPtr const & thisSPtr);
        virtual ~LinkableAsyncOperation();

    protected:
        LinkableAsyncOperation(AsyncOperationSPtr const & primary, AsyncCallback const & callback, AsyncOperationSPtr const & parent);
        LinkableAsyncOperation(AsyncCallback const & callback, AsyncOperationSPtr const & parent);

        __declspec(property(get=get_IsSecondary)) bool IsSecondary;

        inline bool get_IsSecondary() const
        {
            Common::AcquireExclusiveLock grab(lock_);
            return (primary_.get() != 0);
        }
        
        __declspec(property(get=get_PromotedPrimary)) AsyncOperationSPtr const & PromotedPrimary;
        AsyncOperationSPtr const & get_PromotedPrimary() const 
        { 
            Common::AcquireExclusiveLock grab(lock_);
            return promotedPrimary_; 
        }

        template<typename TLinkableAsyncOperation>
        TLinkableAsyncOperation * GetPrimary() const
        {
            Common::AcquireExclusiveLock grab(lock_);
            return dynamic_cast<TLinkableAsyncOperation *>(primary_.get());
        }

        void OnStart(AsyncOperationSPtr const & thisSPtr);
        virtual void OnCompleted();
        virtual void OnResumeOutsideLockPrimary(AsyncOperationSPtr const & thisSPtr) = 0;
                
        // Called when a new primary is promoted, before work is started on the new primary.
        // Can be used by derived classes to update pointers to the primary.
        virtual void OnNewPrimaryPromoted(AsyncOperationSPtr const & promotedPrimary);

        virtual bool IsErrorRetryable(ErrorCode const error) { return false; }

    private:
        void AddOrTryCompleteSecondary(AsyncOperationSPtr const & secondary);
        void CompleteSecondaries();
        void TryCompleteSecondary(
            AsyncOperationSPtr const & secondary,
            ErrorCode const error);
        bool TryPromoteToPrimary();
        bool TryChangePrimary(AsyncOperationSPtr const & primary);

        void CheckIsSecondaryCallerHoldsLock(std::wstring const & methodName);
        void CheckIsPrimaryCallerHoldsLock(std::wstring const & methodName);

        AsyncOperationSPtr primary_;
        // Secondary that is promoted to primary on retryable errors
        AsyncOperationSPtr promotedPrimary_;
        // Lock that protects the list of secondaries on the primary
        // and the primary pointer on the secondary
        mutable Common::ExclusiveLock lock_;
        std::unique_ptr<std::vector<AsyncOperationSPtr>> secondaries_;
    };
}
