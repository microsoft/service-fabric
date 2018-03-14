// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Common
{
    LinkableAsyncOperation::LinkableAsyncOperation(
        AsyncOperationSPtr const & primary, 
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
          primary_(primary),
          secondaries_(),
          lock_()
    {
    }

    LinkableAsyncOperation::LinkableAsyncOperation(AsyncCallback const & callback, AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
          primary_(),
          secondaries_(Common::make_unique<vector<AsyncOperationSPtr>>()),
          lock_()
    {
    }

    LinkableAsyncOperation::~LinkableAsyncOperation()
    {
    }

    void LinkableAsyncOperation::OnStart(AsyncOperationSPtr const &)
    {
        // This method typically will be called under the lock from the container of the linkable async operation.
        // We should not be completing under the lock and hence the after calling CreateAndStart on the Linkable
        // async operations, ResumeOutsideLock should be called to continue the workflow.
        // This also ensures that Cancel can run in parallel with the ResumeOutsideLock code without any issues.
    }

    void LinkableAsyncOperation::ResumeOutsideLock(AsyncOperationSPtr const & thisSPtr)
    {
        // Note that, if set, the primary_ can't be reset before / in parallel with this call;
        // It can only be reset if the secondary is promoted to primary.
        AsyncOperationSPtr primaryCopy = primary_;
        if (primaryCopy)
        {
            auto primary = AsyncOperation::Get<LinkableAsyncOperation>(primaryCopy);
            primary->AddOrTryCompleteSecondary(thisSPtr);
        }
        else
        {
            OnResumeOutsideLockPrimary(thisSPtr);
        }
    }

    void LinkableAsyncOperation::OnCompleted()
    {
        AsyncOperation::OnCompleted();
        if (!IsSecondary)
        {
            // Post completion of the secondaries on a different thread
            auto thisSPtr = this->shared_from_this();
            Threadpool::Post([thisSPtr, this]() { this->CompleteSecondaries(); });
        }
    }

    void LinkableAsyncOperation::CompleteSecondaries()
    {
        unique_ptr<vector<AsyncOperationSPtr>> secondaries = nullptr;

        // beginlock
        {
            AcquireExclusiveLock grab(lock_);
            CheckIsPrimaryCallerHoldsLock(L"CompleteSecondaries");

            // exchange unique pointers
            secondaries.swap(secondaries_);
        }

        // complete all secondaries
        for (auto iter = secondaries->begin(); iter != secondaries->end(); ++iter)
        {
            TryCompleteSecondary(*iter, Error);
        }
    }

    // Should not be called under the outside holder lock
    void LinkableAsyncOperation::TryCompleteSecondary(
        AsyncOperationSPtr const & secondary,
        ErrorCode const error)
    {
        if (error.IsSuccess())
        {
            secondary->TryComplete(secondary, error);
            return;
        }

        auto linkableSecondary = AsyncOperation::Get<LinkableAsyncOperation>(secondary);
        LinkableAsyncOperation * linkableNewPrimary = NULL;
        AsyncOperationSPtr newPrimarySPtr = nullptr;
        
        if (linkableSecondary->IsErrorRetryable(error))
        {
            // Executed on PRIMARY only:
            // Check promoted primary under the lock, determine whether the secondary 
            // will be promoted to primary or will be linked to the already promoted primary
            AcquireExclusiveLock grab(lock_);
            CheckIsPrimaryCallerHoldsLock(L"TryCompleteSecondary");
            if (promotedPrimary_)
            {
                if (!linkableSecondary->TryChangePrimary(promotedPrimary_))
                {
                    // The secondary was already completed, nothing to do
                    return;
                }

                linkableNewPrimary = AsyncOperation::Get<LinkableAsyncOperation>(promotedPrimary_);
            }
            else
            {
                if (!linkableSecondary->TryPromoteToPrimary())
                {
                    // The secondary was already completed, nothing to do
                    return;
                }
                
                promotedPrimary_ = secondary;
                newPrimarySPtr = secondary;
                linkableNewPrimary = linkableSecondary;
            }
        }
        else
        {
            secondary->TryComplete(secondary, error);
            return;
        }

        if (newPrimarySPtr)
        {
            OnNewPrimaryPromoted(newPrimarySPtr);
            linkableNewPrimary->OnResumeOutsideLockPrimary(newPrimarySPtr);
        }
        else 
        {
            linkableNewPrimary->AddOrTryCompleteSecondary(secondary);
        }
    }

    void LinkableAsyncOperation::OnNewPrimaryPromoted(AsyncOperationSPtr const &)
    {
        // Do nothing, only needed in derived classes that support retryable errors
    }

    bool LinkableAsyncOperation::TryPromoteToPrimary()
    {
        AcquireExclusiveLock grab(lock_);
        CheckIsSecondaryCallerHoldsLock(L"PromoteToPrimary");

        if (this->InternalIsCompleted)
        {
            return false;
        }

        primary_.reset();
        secondaries_ = move(Common::make_unique<vector<AsyncOperationSPtr>>());
        return true;
    }

    bool LinkableAsyncOperation::TryChangePrimary(AsyncOperationSPtr const & primary)
    {
        AcquireExclusiveLock grab(lock_);
        CheckIsSecondaryCallerHoldsLock(L"ChangePrimary");
        
        if (this->InternalIsCompleted)
        {
            return false;
        }

        primary_ = primary;
        return true;
    }

    void LinkableAsyncOperation::AddOrTryCompleteSecondary(AsyncOperationSPtr const & secondary)
    {
        bool added = false;
        
        // beginlock
        {
            AcquireExclusiveLock grab(lock_);
            CheckIsPrimaryCallerHoldsLock(L"AddOrTryCompleteSecondary");
     
            if (secondaries_.get() != 0)
            {
                secondaries_->push_back(secondary);
                added = true;
            }
        }

        if (!added)
        {
            // The primary has already completed. Find and link to the new
            // promoted primary if the error is retryable. May result in 
            // this secondary becoming the new primary.
            //
            // TryCompleteSecondary() will change the primary_ member variable on this
            // secondary to point to the new (promoted) primary, so we need to ensure that the
            // old (completing/completed) primary stays alive throughout the duration of the 
            // TryCompleteSecondary() call.
            //    
            this->TryCompleteSecondary(secondary, this->Error);
        }
    }

    void LinkableAsyncOperation::CheckIsSecondaryCallerHoldsLock(wstring const & methodName)
    {
        if (primary_.get() == 0)
        {
            Assert::CodingError("{0}: Method should be called on the secondary only", methodName);
        }
    }

    void LinkableAsyncOperation::CheckIsPrimaryCallerHoldsLock(wstring const & methodName)
    {
        if (primary_.get() != 0)
        {
            Assert::CodingError("{0}: Method should be called on the primary only", methodName);
        }
    }
}
