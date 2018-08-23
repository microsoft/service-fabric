// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace Transport;
    using namespace Federation;
    using namespace Reliability;
    using namespace std;
    using namespace Store;

    StringLiteral const TraceComponent("ProcessDeleteName");

    StoreService::ProcessDeleteNameRequestAsyncOperation::ProcessDeleteNameRequestAsyncOperation(
        MessageUPtr && request,
        __in NamingStore & namingStore,
        __in StoreServiceProperties & properties,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ProcessRequestAsyncOperation(
            std::move(request),
            namingStore,
            properties,
            timeout,
            callback,
            root)
        , nameOwnerLocation_()
        , isExplicitRequest_(true)
        , nameNotFoundOnAuthorityOwner_(false)
        , lockedNames_()
        , tentativeNames_()
        , tryToDeleteParent_(false)
    {
    }

    StoreService::ProcessDeleteNameRequestAsyncOperation::ProcessDeleteNameRequestAsyncOperation(
        MessageUPtr && request,
        __in NamingStore & namingStore,
        __in StoreServiceProperties & properties,
        bool isExplicitRequest,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ProcessRequestAsyncOperation(
            std::move(request),
            namingStore,
            properties,
            timeout,
            callback,
            root)
        , nameOwnerLocation_()
        , isExplicitRequest_(isExplicitRequest)
        , nameNotFoundOnAuthorityOwner_(false)
        , lockedNames_()
        , tentativeNames_()
        , tryToDeleteParent_(false)
    {
    }

    ErrorCode StoreService::ProcessDeleteNameRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        if (this->TryGetNameFromRequest(request))
        {
            DeleteNameHeader header;
            if (request->Headers.TryReadFirst(header))
            {
                // For backwards compatibility with V1 Naming Service replicas,
                // the CM will send an delete name request to clean-up names.
                // This delete name request is only meant to be processed by
                // V1 NS replicas. V2 replicas should treat it like an implicit
                // delete name request.
                //
                isExplicitRequest_ = header.IsExplicit;
            }

            this->ProcessRecoveryHeader(move(request));
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }

    void StoreService::ProcessDeleteNameRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {
        // Reply will always be an empty body
        this->Reply = NamingMessage::GetPeerNameOperationReply();

        tentativeNames_.AddTentativeName(this->Name, this->NameString);

        if (isExplicitRequest_)
        {
            auto inner = Store.BeginAcquireNamedLock(
                this->Name, 
                this->TraceId,
                GetRemainingTime(),
                [this] (AsyncOperationSPtr const & queueOperation) { this->OnNamedLockAcquireComplete(queueOperation, false); },
                thisSPtr);
            this->OnNamedLockAcquireComplete(inner, true);
        }
        else
        {
            this->StartDeleteName(thisSPtr);
        }
    }

    void StoreService::ProcessDeleteNameRequestAsyncOperation::OnNamedLockAcquireComplete(
        AsyncOperationSPtr const & queueOperation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != queueOperation->CompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = queueOperation->Parent;

        ErrorCode error = Store.EndAcquireNamedLock(this->Name, this->TraceId, queueOperation);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            lockedNames_.push_back(this->Name);
            
            if (this->TryAcceptRequestAtAuthorityOwner(thisSPtr))
            {
                this->StartDeleteName(thisSPtr);
            }
        }
    }

    void StoreService::ProcessDeleteNameRequestAsyncOperation::StartDeleteName(AsyncOperationSPtr const & thisSPtr)
    {
        this->Properties.Trace.AOStartDeleteName(
            this->TraceId,
            tentativeNames_.CurrentNameString,
            tentativeNames_.IsCurrentNameCompletePending,
            isExplicitRequest_);
                
        // Acquire parent lock
        auto inner = this->Store.BeginAcquireNamedLock(
            tentativeNames_.CurrentParentName, 
            this->TraceId,
            this->GetRemainingTime(),
            [this] (AsyncOperationSPtr const & operation) { this->OnParentNamedLockAcquireComplete(operation, false); },
            thisSPtr);
        this->OnParentNamedLockAcquireComplete(inner, true);
    }

    void StoreService::ProcessDeleteNameRequestAsyncOperation::OnParentNamedLockAcquireComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != operation->CompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = this->Store.EndAcquireNamedLock(tentativeNames_.CurrentParentName, this->TraceId, operation);
         
        if (error.IsSuccess())
        {
            lockedNames_.push_back(tentativeNames_.CurrentParentName);
            this->DeleteNameAtAuthorityOwner(thisSPtr);
        }
        else if (tentativeNames_.IsCurrentNameCompletePending || this->IsLocalRetryable(error))
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error), 
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartDeleteName(thisSPtr); });
        }
        else
        {
            this->CompleteOrRecoverPrimary(thisSPtr, move(error), 
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartDeleteName(thisSPtr); });
        }
    }
    
    void StoreService::ProcessDeleteNameRequestAsyncOperation::DeleteNameAtAuthorityOwner(AsyncOperationSPtr const & thisSPtr)
    {
        TransactionSPtr txSPtr;
        ErrorCode error = this->PrepareDeleteCurrentName(txSPtr);
        
        if (error.IsSuccess())
        {
            auto inner = NamingStore::BeginCommit(
                move(txSPtr),
                this->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnStoreCommitComplete(operation, false); },
                thisSPtr);
            this->OnStoreCommitComplete(inner, true);
        }
        else
        {
            // Certain errors are considered success, so we can skip commit and go to next step
            if (error.IsError(ErrorCodeValue::UpdatePending))
            {
                error = ErrorCode(ErrorCodeValue::Success);
            }
            else if (error.IsError(ErrorCodeValue::NameNotFound))
            {
                if (tentativeNames_.IsCurrentNameCompletePending || this->IsPrimaryRecovery)
                {
                    error = ErrorCode(ErrorCodeValue::Success);
                }
            }

            txSPtr.reset();

            // Retry / recover on errors or go to next step
            this->OnDeleteNameAtAuthorityOwnerDone(thisSPtr, move(error));
        }
    }

    void StoreService::ProcessDeleteNameRequestAsyncOperation::OnStoreCommitComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = NamingStore::EndCommit(operation);

        this->OnDeleteNameAtAuthorityOwnerDone(thisSPtr, move(error));
    }

    void StoreService::ProcessDeleteNameRequestAsyncOperation::OnDeleteNameAtAuthorityOwnerDone(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode && error)
    {
        if (error.IsSuccess())
        {
            WriteNoise(
                TraceComponent,
                "{0} delete name at authority owner complete: name = {1} complete = {2}",
                this->TraceId,
                tentativeNames_.CurrentNameString,
                tentativeNames_.IsCurrentNameCompletePending);

            if (tentativeNames_.IsCurrentNameCompletePending)
            {
                // The name is complete, remove it from the list of tentative names and go back on the recursive chain
                // Note that the parent name lock is not released here: 
                // if need to delete parent, the lock must be held.
                // Otherwise, the async operation is done and we will release the lock in OnComplete.
                this->FinishDeleteName(thisSPtr);
            }
            else
            {
                // Release parent lock
                this->Store.ReleaseNamedLock(lockedNames_.back(), this->TraceId, thisSPtr);
                lockedNames_.pop_back();

                // Go to name owner to update information
                this->StartResolveNameOwner(thisSPtr);
            }
        }
        else
        {
            // Release parent lock, it will be re-acquired on retry if needed
            this->Store.ReleaseNamedLock(lockedNames_.back(), this->TraceId, thisSPtr);
            lockedNames_.pop_back();

             WriteInfo(
                TraceComponent,
                "{0} delete name at authority owner failed: name = {1} complete = {2}, error = {3}",
                this->TraceId,
                tentativeNames_.CurrentNameString,
                tentativeNames_.IsCurrentNameCompletePending,
                error);

            if (error.IsError(ErrorCodeValue::NameNotEmpty))
            {
                this->OnNameNotEmpty(thisSPtr);
            }
            else if (tentativeNames_.IsCurrentNameCompletePending || this->IsLocalRetryable(error))
            {
                this->CompleteOrScheduleRetry(thisSPtr, move(error), 
                    [this](AsyncOperationSPtr const & thisSPtr) { this->StartDeleteName(thisSPtr); });
            }
            else
            {
                if (error.IsError(ErrorCodeValue::NameNotFound))
                {
                    if (!this->IsPrimaryRecovery)
                    {
                        nameNotFoundOnAuthorityOwner_ = true;
                    }

                    // Like recursive name creation, recursive name deletion is 
                    // eventually consistent (AO/NO consistency) but not atomic.
                    // Continue recursive deletion until we encounter a name that
                    // exists, but can't be deleted (non-empty name).
                    //
                    tryToDeleteParent_ = true;
                    this->FinishDeleteName(thisSPtr);
                }
                else
                {
                    // Cannot release the named lock until we have achieved consistency during
                    // primary recovery since releasing the lock will allow user requests to
                    // act on this inconsistent name.
                    //
                    this->CompleteOrRecoverPrimary(thisSPtr, move(error), 
                        [this](AsyncOperationSPtr const & thisSPtr) { this->StartDeleteName(thisSPtr); });
                }
            }
        }
    }
 
    void StoreService::ProcessDeleteNameRequestAsyncOperation::StartResolveNameOwner(AsyncOperationSPtr const & thisSPtr)
    {
        auto inner = this->BeginResolveNameLocation(
            tentativeNames_.CurrentName,
            [this](AsyncOperationSPtr const & asyncOperation) { this->OnResolveNameOwnerComplete(asyncOperation, false); },
            thisSPtr);
        this->OnResolveNameOwnerComplete(inner, true);
    }

    void StoreService::ProcessDeleteNameRequestAsyncOperation::OnResolveNameOwnerComplete(
        Common::AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != operation->CompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = this->EndResolveNameLocation(operation, nameOwnerLocation_);

        if (!error.IsSuccess())
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error), 
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartResolveNameOwner(thisSPtr); });
        }
        else
        {
            this->StartRequestToNameOwner(thisSPtr);
        }
    }

    void StoreService::ProcessDeleteNameRequestAsyncOperation::StartRequestToNameOwner(AsyncOperationSPtr const & thisSPtr)
    {
        auto message = NamingMessage::GetInnerNameDelete(tentativeNames_.CurrentName); 
        message->Headers.Replace(nameOwnerLocation_.CreateFilterHeader());

        auto inner = this->BeginRequestReplyToPeer(
            std::move(message),
            nameOwnerLocation_.NodeInstance,
            [this](AsyncOperationSPtr const & asyncOperation) { this->OnRequestReplyToPeerComplete(asyncOperation, false); },
            thisSPtr);

        this->Properties.Trace.AOInnerDeleteNameRequestSendComplete(
            this->TraceId, 
            this->Node.Id.ToString(), 
            this->Node.InstanceId, 
            tentativeNames_.CurrentNameString, 
            nameOwnerLocation_.NodeInstance.Id.ToString(), 
            nameOwnerLocation_.NodeInstance.InstanceId);

        this->OnRequestReplyToPeerComplete(inner, true);
    }

    void StoreService::ProcessDeleteNameRequestAsyncOperation::OnRequestReplyToPeerComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        MessageUPtr unusedReply;
        ErrorCode error = this->EndRequestReplyToPeer(operation, unusedReply);

        if (error.IsError(ErrorCodeValue::NameNotEmpty))
        {
            // Special-case for name deletion:
            //
            // Properties are created directly at the Name Owner 
            // while names are deleted starting at the Authority Owner.
            // If a property is written at the Name Owner before deletion occurs, 
            // then revert the tentative delete state on this hierarchy name.
            //
            this->RevertTentativeDelete(thisSPtr);
        }
        else if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NameNotFound))
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error), 
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartResolveNameOwner(thisSPtr); });
        }
        else
        {
            this->Properties.Trace.AOInnerDeleteNameReplyReceiveComplete(
                this->TraceId, 
                this->Node.Id.ToString(), 
                this->Node.InstanceId, 
                tentativeNames_.CurrentNameString);

            // Update delete name as "complete"
            tentativeNames_.SetCurrentNameCompletePending();
            this->StartDeleteName(thisSPtr);
        }
    }

    void StoreService::ProcessDeleteNameRequestAsyncOperation::FinishDeleteName(AsyncOperationSPtr const & thisSPtr)
    {
        if (tryToDeleteParent_ && tentativeNames_.CurrentParentName != NamingUri::RootNamingUri)
        {
            // First complete the current name, because the work is done
            // then move to the parent
            tentativeNames_.PromoteParentAsTentative();
        
            // Continue up on the hierarchy, to delete all parents that are empty and not explicitly created
            this->StartDeleteName(thisSPtr);
        }
        else
        {
            // All names have been deleted, done.

            // On peer communication failure, StoreService::ProcessRequestAsyncOperation
            // will automatically set a failure reply.
            //
            // Set the success reply again here since we may have retried on such a failure earlier.
            //
            this->Reply = NamingMessage::GetPeerNameOperationReply();   
            this->TryComplete(
                thisSPtr, 
                (nameNotFoundOnAuthorityOwner_ ? ErrorCodeValue::NameNotFound : ErrorCodeValue::Success));
        }
    }

    void StoreService::ProcessDeleteNameRequestAsyncOperation::RevertTentativeDelete(AsyncOperationSPtr const & thisSPtr)
    {
        TransactionSPtr txSPtr;
        auto error = this->CreateRevertDeleteTransaction(txSPtr);
        
        if (error.IsSuccess())
        {
            auto operation = NamingStore::BeginCommit(
                move(txSPtr),
                this->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnRevertTentativeComplete(operation, false); },
                thisSPtr);
            this->OnRevertTentativeComplete(operation, true);
        }
        else
        {
            txSPtr.reset();
            this->CompleteOrScheduleRetry(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->RevertTentativeDelete(thisSPtr); });
        }
    }

    void StoreService::ProcessDeleteNameRequestAsyncOperation::OnRevertTentativeComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = NamingStore::EndCommit(operation);

        // Pending replication from a previous asynchronous commit timeout
        // could have committed after the timeout
        //
        if (error.IsError(ErrorCodeValue::NameAlreadyExists))
        {
            error = ErrorCodeValue::Success;
        }

        if (!error.IsSuccess())
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->RevertTentativeDelete(thisSPtr); });
        }
        else
        {
            this->OnNameNotEmpty(thisSPtr);
        }
    }
    
    void StoreService::ProcessDeleteNameRequestAsyncOperation::OnCompleted()
    {
        StoreService::ProcessRequestAsyncOperation::OnCompleted();

         if (!lockedNames_.empty())
        {
            auto root = shared_from_this();
            for (auto it = lockedNames_.begin(); it != lockedNames_.end(); ++it)
            {
                this->Store.ReleaseNamedLock(*it, this->TraceId, root);
            }

            lockedNames_.clear();
        }

        this->Properties.Trace.AODeleteNameComplete(
            this->TraceId, 
            this->Node.Id.ToString(), 
            this->Node.InstanceId, 
            this->NameString);
    }
        
    ErrorCode StoreService::ProcessDeleteNameRequestAsyncOperation::PrepareDeleteCurrentName(__out TransactionSPtr & outTxSPtr)
    {
        TransactionSPtr txSPtr;
        ErrorCode error = this->Store.CreateTransaction(this->ActivityHeader, txSPtr);

        if (!error.IsSuccess())
        {
            return error;
        }
        
        wstring const & dataType = Constants::HierarchyNameEntryType;
        wstring const & nameString = tentativeNames_.CurrentNameString;
        
        // Can't delete a name that doesn't exist
        _int64 nameSequenceNumber = -1;
        EnumerationSPtr enumSPtr(nullptr);
        error = this->Store.TryGetCurrentSequenceNumber(
            txSPtr,
            dataType,
            nameString,
            nameSequenceNumber,
            enumSPtr);

        if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteNoise(TraceComponent, "{0}: name {1} doesn't exist", this->TraceId, nameString);
            return ErrorCodeValue::NameNotFound;
        }

        HierarchyNameData nameData;
        if (error.IsSuccess())
        {
            error = this->Store.ReadCurrentData<HierarchyNameData>(enumSPtr, nameData);
        }

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} error reading hierarchy name {1}: error = {2}",
                this->TraceId,
                nameString,
                error);
            return error;
        }

        // Can't delete a name with an existing service description
        // or subnames
        //
        error = CheckNameCanBeDeleted(enumSPtr, tentativeNames_.CurrentName, nameString, nameData);
        if (!error.IsSuccess())
        {
            return error;
        }
        
        WriteNoise(
            TraceComponent,
            "{0} AO create delete transaction: name = {1}, complete = {2}",
            this->TraceId,
            nameString,
            tentativeNames_.IsCurrentNameCompletePending);

        if (tentativeNames_.IsCurrentNameCompletePending)
        {
            error = this->Store.DeleteFromStore(
                txSPtr, 
                dataType,
                nameString,
                nameSequenceNumber);
        }
        else if (nameData.NameState == HierarchyNameState::Deleting)
        {
            // Optimization: don't re-write tentative state (just make sure the LSN we return is correct)
            return ErrorCode(ErrorCodeValue::UpdatePending);
        }
        else
        {
            // set tentative state
            nameData.NameState = HierarchyNameState::Deleting;
            error = this->Store.TryWriteData<HierarchyNameData>(txSPtr, nameData, dataType, nameString);
        }

        if (!error.IsSuccess())
        {
            return error;
        }
                
        // All hierarchy name deletions increment the sequence number on the parent name in the 
        // hierarchy to track transactional consistency
        //            
        if (tentativeNames_.IsCurrentNameCompletePending)
        {
            _int64 parentNameSequenceNumber = -1;
            wstring const & parentNameString = tentativeNames_.CurrentParentNameString;
            error = this->Store.TryGetCurrentSequenceNumber(
                txSPtr,
                dataType,
                parentNameString,
                parentNameSequenceNumber,
                enumSPtr);

            HierarchyNameData parentNameData;
            if (error.IsSuccess())
            {
                error = this->Store.ReadCurrentData<HierarchyNameData>(enumSPtr, parentNameData);
            }

            if (!error.IsSuccess())
            {
                return error;
            }
            
            parentNameData.IncrementSubnamesVersion();
            error = this->Store.TryWriteData<HierarchyNameData>(
                txSPtr,
                parentNameData,
                dataType,
                parentNameString,
                parentNameSequenceNumber);

            // Check whether parent has service or children: if not empty, stop recursive delete.
            // Otherwise, remember the parent as the next name that should be deleted.
            // Do not mark it as tentative yet, because that should be done under its parent lock
            if (error.IsSuccess())
            {
                ErrorCode checkParentError = CheckNameCanBeDeleted(
                    enumSPtr, 
                    tentativeNames_.CurrentParentName, 
                    tentativeNames_.CurrentParentNameString, 
                    parentNameData);

                // If the parent is not empty, do not add it in the list of next to be processed names
                // This makes the recursion chain to stop prematurely
                if (checkParentError.IsSuccess() || !checkParentError.IsError(ErrorCodeValue::NameNotEmpty))
                {
                    tryToDeleteParent_ = true;
                }
                else
                {
                    tryToDeleteParent_ = false;
                }
            }
        }

        if (error.IsSuccess())
        {
            outTxSPtr = move(txSPtr);
        }

        return error;
    }

    ErrorCode StoreService::ProcessDeleteNameRequestAsyncOperation::CreateRevertDeleteTransaction(__out TransactionSPtr & txSPtr)
    {
        ErrorCode error = this->Store.CreateTransaction(this->ActivityHeader, txSPtr);

        if (!error.IsSuccess())
        {
            return error;
        }

        EnumerationSPtr enumSPtr = nullptr;

        _int64 nameSequenceNumber = -1;
        wstring const & dataType = Constants::HierarchyNameEntryType;
        wstring const & nameString = tentativeNames_.CurrentNameString;
        error = Store.TryGetCurrentSequenceNumber(txSPtr, dataType, nameString, nameSequenceNumber, enumSPtr);

        HierarchyNameData nameData(
            HierarchyNameState::Invalid, 
            UserServiceState::None, 
            DateTime::Now().Ticks); // base store version acts as instance id across deletes
        if (error.IsSuccess())
        {
            error = Store.ReadCurrentData<HierarchyNameData>(enumSPtr, nameData);
        }
        
        if (error.IsSuccess())
        {
            // Preserve service state and subnames version number
            // as well as the flags (explicit/implicit) 
            //
            nameData.NameState = HierarchyNameState::Created; 
            error = Store.TryWriteData<HierarchyNameData>(txSPtr, nameData, dataType, nameString, nameSequenceNumber);
        }

        return error;
    }

    ErrorCode StoreService::ProcessDeleteNameRequestAsyncOperation::CheckNameCanBeDeleted(
        __in EnumerationSPtr & enumSPtr, 
        NamingUri const & name,
        wstring const & nameString,
        HierarchyNameData const & nameData)
    {
        if (nameData.ServiceState != UserServiceState::None)
        {
            WriteInfo(
                TraceComponent,
                "{0} cannot delete hierarchy name {1} since it still has a service",
                this->TraceId,
                nameString);
            return ErrorCodeValue::NameNotEmpty;
        }

        // If not the original name, check that it's not explicitly set.
        // If the name was explicitly created and this is the original name, 
        // only delete if this is an explicit request (eg. not coming from a delete service)
        if (nameData.IsExplicit)
        {
            bool isInitialName = IsInitialName(nameString);
            if (!isInitialName || !isExplicitRequest_)
            {
                WriteInfo(
                    TraceComponent,
                    "{0} cannot delete hierarchy name {1} since it was explicitly created",
                    this->TraceId,
                    nameString);
                return ErrorCodeValue::NameNotEmpty;
            }
        }

        ErrorCode error;
        while ((error = enumSPtr->MoveNext()).IsSuccess())
        {
            wstring currentName;
            error = enumSPtr->CurrentKey(currentName);

            if (!error.IsSuccess())
            {
                break;
            }

            NamingUri subNameCandidate;
            if (NamingUri::TryParse(currentName, subNameCandidate))
            {
                if (name.IsPrefixOf(subNameCandidate))
                {
                    WriteInfo(
                        TraceComponent,
                        "{0} cannot delete hierarchy name {1} since it still has subname {2}", 
                        this->TraceId,
                        tentativeNames_.CurrentNameString,
                        subNameCandidate);
                    return ErrorCodeValue::NameNotEmpty;
                }
                else if (StringUtility::StartsWith(currentName, nameString))
                {
                    // The list of subnames may not be contiguous in the database
                    continue;
                }
                else
                {
                    break;
                }
            }
        } // while subnames

        if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            return ErrorCodeValue::Success;
        }

        return error;
    }

    bool StoreService::ProcessDeleteNameRequestAsyncOperation::IsInitialName(std::wstring const & nameString) const 
    { 
        return this->NameString == nameString; 
    }

    void StoreService::ProcessDeleteNameRequestAsyncOperation::OnNameNotEmpty(AsyncOperationSPtr const & thisSPtr)
    {
        // Can't retry on this error, complete execution.
        // Only expose the error if the name is the original one. Otherwise, the original name
        // was already deleted, so consider success
        if (isExplicitRequest_ && this->IsInitialName(tentativeNames_.CurrentNameString))
        {
            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::NameNotEmpty));
        }
        else
        {
            this->TryComplete(
                thisSPtr, 
                (nameNotFoundOnAuthorityOwner_ ? ErrorCodeValue::NameNotFound : ErrorCodeValue::Success));
        }
    }
}
