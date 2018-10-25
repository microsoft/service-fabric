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

    StringLiteral const TraceComponent("ProcessCreateName");

    StoreService::ProcessCreateNameRequestAsyncOperation::ProcessCreateNameRequestAsyncOperation(
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
        , isExplicitRequest_(true)
        , nameOwnerLocation_()
        , lockedNames_()
        , tentativeNames_()
        , authorityNameAlreadyCompleted_(false)
    {
    }

    StoreService::ProcessCreateNameRequestAsyncOperation::ProcessCreateNameRequestAsyncOperation(
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
        , isExplicitRequest_(isExplicitRequest)
        , nameOwnerLocation_()
        , lockedNames_()
        , tentativeNames_()
        , authorityNameAlreadyCompleted_(false)
    {
    }

    ErrorCode StoreService::ProcessCreateNameRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        if (this->TryGetNameFromRequest(request))
        {
            this->ProcessRecoveryHeader(move(request));
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }

    void StoreService::ProcessCreateNameRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {
        // Reply will always be an empty body
        this->Reply = NamingMessage::GetPeerNameOperationReply();

        tentativeNames_.AddTentativeName(this->Name, this->NameString);

        if (isExplicitRequest_)
        {
            auto inner = this->Store.BeginAcquireNamedLock(
                this->Name,
                this->TraceId,
                this->GetRemainingTime(),
                [this] (AsyncOperationSPtr const & operation) { this->OnNamedLockAcquireComplete(operation, false); },
                thisSPtr);
            this->OnNamedLockAcquireComplete(inner, true);
        }
        else
        {
            this->StartCreateName(thisSPtr);
        }
    }

    void StoreService::ProcessCreateNameRequestAsyncOperation::OnNamedLockAcquireComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != operation->CompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = this->Store.EndAcquireNamedLock(this->Name, this->TraceId, operation);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            lockedNames_.push_back(this->Name);

            if (this->TryAcceptRequestAtAuthorityOwner(thisSPtr))
            {
                this->StartCreateName(thisSPtr);
            }
        }
    }

    void StoreService::ProcessCreateNameRequestAsyncOperation::StartCreateName(AsyncOperationSPtr const & thisSPtr)
    {
        this->Properties.Trace.AOStartCreateName(
            this->TraceId,
            tentativeNames_.CurrentNameString,
            tentativeNames_.IsCurrentNameCompletePending);

        // Acquire parent lock
        auto inner = this->Store.BeginAcquireNamedLock(
            tentativeNames_.CurrentParentName, 
            this->TraceId,
            this->GetRemainingTime(),
            [this] (AsyncOperationSPtr const & operation) { this->OnParentNamedLockAcquireComplete(operation, false); },
            thisSPtr);
        this->OnParentNamedLockAcquireComplete(inner, true);
    }

    void StoreService::ProcessCreateNameRequestAsyncOperation::OnParentNamedLockAcquireComplete(
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
            WriteNameAtAuthorityOwner(thisSPtr);
        }
        else if (tentativeNames_.IsCurrentNameCompletePending || this->IsLocalRetryable(error))
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error), 
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartCreateName(thisSPtr); });
        }
        else
        {
            this->CompleteOrRecoverPrimary(thisSPtr, move(error), 
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartCreateName(thisSPtr); });
        }
    }

    void StoreService::ProcessCreateNameRequestAsyncOperation::WriteNameAtAuthorityOwner(AsyncOperationSPtr const & thisSPtr)
    {
        TransactionSPtr txSPtr;
        ErrorCode error = this->PrepareWriteCurrentName(txSPtr);
        
        if (error.IsError(ErrorCodeValue::NameNotFound))
        {
            // Parent name not found, mark it as tentative and process it.
            // Note that at this point the parent named lock is still held.
            tentativeNames_.MarkParentAsTentative();
            this->StartCreateName(thisSPtr);
        }
        else if (error.IsSuccess())
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
            // Certain errors are considered success, so we can go to next step
            if (error.IsError(ErrorCodeValue::UpdatePending))
            {
                // Skip committing to database, the value is already there
                error = ErrorCode(ErrorCodeValue::Success);
            }
            else if (error.IsError(ErrorCodeValue::NameAlreadyExists))
            {
                if (this->IsPrimaryRecovery || tentativeNames_.IsCurrentNameCompletePending)
                {
                    error = ErrorCode(ErrorCodeValue::Success);
                }
                else
                {
                    authorityNameAlreadyCompleted_ = true;
                }
            }

            txSPtr.reset();
            this->OnWriteNameAtAuthorityOwnerDone(thisSPtr, move(error));
        }
    }
      
    void StoreService::ProcessCreateNameRequestAsyncOperation::OnStoreCommitComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = NamingStore::EndCommit(operation);

        this->OnWriteNameAtAuthorityOwnerDone(thisSPtr, move(error));
    }

    void StoreService::ProcessCreateNameRequestAsyncOperation::OnWriteNameAtAuthorityOwnerDone(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode && error)
    {
        // Release parent lock, for both error and success cases.
        this->Store.ReleaseNamedLock(tentativeNames_.CurrentParentName, this->TraceId, thisSPtr);
        lockedNames_.pop_back();

        // Do not expect the NO name to missing if the authority name is completed,
        // but avoid create service getting stuck if this ever happens - ensure that the NO name
        // also exists.
        //
        if (error.IsSuccess() || error.IsError(ErrorCodeValue::NameAlreadyExists))
        {
             WriteNoise(
                TraceComponent,
                "{0} write name at authority owner complete: name = {1} complete = {2}",
                this->TraceId,
                tentativeNames_.CurrentNameString,
                tentativeNames_.IsCurrentNameCompletePending);

            if (tentativeNames_.IsCurrentNameCompletePending)
            {
                // The name is complete, remove it from the list of tentative names and go back on the recursive chain
                this->FinishCreateName(thisSPtr);
            }
            else
            {
                // Go to name owner to update information
                this->StartResolveNameOwner(thisSPtr);
            }
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} write name at authority owner failed: name = {1} complete = {2}, error = {3}",
                this->TraceId,
                tentativeNames_.CurrentNameString,
                tentativeNames_.IsCurrentNameCompletePending,
                error);

            if (tentativeNames_.IsCurrentNameCompletePending || this->IsLocalRetryable(error))
            {
                // Must continue trying to achieve consistency as long as tentative data was written
                // or the local error does not guarantee that the data will not be written
                //
                this->CompleteOrScheduleRetry(thisSPtr, move(error), 
                    [this](AsyncOperationSPtr const & thisSPtr) { this->StartCreateName(thisSPtr); });
            }
            else 
            {
                // Cannot release the named lock until we have achieved consistency during
                // primary recovery since releasing the lock will allow user requests to
                // act on this inconsistent name.
                //
                this->CompleteOrRecoverPrimary(thisSPtr, move(error), 
                    [this](AsyncOperationSPtr const & thisSPtr) { this->StartCreateName(thisSPtr); });
            }
        }
    }
       
    void StoreService::ProcessCreateNameRequestAsyncOperation::StartResolveNameOwner(AsyncOperationSPtr const & thisSPtr)
    {
        auto inner = this->BeginResolveNameLocation(
            tentativeNames_.CurrentName,
            [this](AsyncOperationSPtr const & operation) { this->OnResolveNameOwnerComplete(operation, false); },
            thisSPtr);
        this->OnResolveNameOwnerComplete(inner, true);
    }

    void StoreService::ProcessCreateNameRequestAsyncOperation::OnResolveNameOwnerComplete(
        AsyncOperationSPtr const & operation,
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

    void StoreService::ProcessCreateNameRequestAsyncOperation::StartRequestToNameOwner(AsyncOperationSPtr const & thisSPtr)
    {
        auto message = NamingMessage::GetInnerNameCreate(tentativeNames_.CurrentName); 
        message->Headers.Replace(nameOwnerLocation_.CreateFilterHeader());

        auto inner = this->BeginRequestReplyToPeer(
            std::move(message),
            nameOwnerLocation_.NodeInstance,
            [this](AsyncOperationSPtr const & asyncOperation) { this->OnRequestReplyToPeerComplete(asyncOperation, false); },
            thisSPtr);

        this->Properties.Trace.AOInnerCreateNameRequestSendComplete(
            this->TraceId, 
            this->Node.Id.ToString(), 
            this->Node.InstanceId, 
            tentativeNames_.CurrentNameString, 
            nameOwnerLocation_.NodeInstance.Id.ToString(), 
            nameOwnerLocation_.NodeInstance.InstanceId);

        this->OnRequestReplyToPeerComplete(inner, true);
    }

    void StoreService::ProcessCreateNameRequestAsyncOperation::OnRequestReplyToPeerComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != operation->CompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        MessageUPtr unusedReply;
        ErrorCode error = this->EndRequestReplyToPeer(operation, unusedReply);

        // The request may have been processed by the Name Owner.
        // From this point on, we must attempt to make the Authority Owner
        // and Name Owner consistent if possible.
        //

        if (error.IsSuccess() || error.IsError(ErrorCodeValue::NameAlreadyExists))
        {
            if (authorityNameAlreadyCompleted_)
            {
                this->TryComplete(thisSPtr, ErrorCodeValue::NameAlreadyExists);
            }
            else
            {
                this->Properties.Trace.AOInnerCreateNameReplyReceiveComplete(
                    this->TraceId, 
                    this->Node.Id.ToString(), 
                    this->Node.InstanceId, 
                    tentativeNames_.CurrentNameString);

                // Write "complete" name
                tentativeNames_.SetCurrentNameCompletePending();

                this->StartCreateName(thisSPtr);
            }
        }
        else
        {
            this->CompleteOrScheduleRetry(thisSPtr, 
                move(error), 
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartResolveNameOwner(thisSPtr); });
        }
    }

    void StoreService::ProcessCreateNameRequestAsyncOperation::FinishCreateName(AsyncOperationSPtr const & thisSPtr)
    {
        if (tentativeNames_.CompleteNameAndTryMoveNext())
        {
            // There are more names to be created
            // The current node was already put as tentative, so release lock and go to name owner
            this->Store.ReleaseNamedLock(tentativeNames_.CurrentParentName, this->TraceId, thisSPtr);
            lockedNames_.pop_back();
                    
            this->StartResolveNameOwner(thisSPtr);
        }
        else
        {
            // All names have been created, done.

            // On peer communication failure, StoreService::ProcessRequestAsyncOperation
            // will automatically set a failure reply.
            //
            // Set the success reply again here since we may have retried on such a failure earlier.
            //
            this->Reply = NamingMessage::GetPeerNameOperationReply();   
            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        }
    }
        
    void StoreService::ProcessCreateNameRequestAsyncOperation::OnCompleted()
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

        this->Properties.Trace.AOCreateNameComplete(
            this->TraceId, 
            this->Node.Id.ToString(), 
            this->Node.InstanceId, 
            this->NameString,
            this->Error);
    }

    Common::ErrorCode StoreService::ProcessCreateNameRequestAsyncOperation::PrepareWriteCurrentName(__out TransactionSPtr & outTxSPtr)
    {
        TransactionSPtr txSPtr;
        ErrorCode error = this->Store.CreateTransaction(this->ActivityHeader, txSPtr);

        if (!error.IsSuccess())
        {
            return error;
        }

        wstring const & dataType = Constants::HierarchyNameEntryType;
        wstring const & nameString = tentativeNames_.CurrentNameString;
        wstring const & parentNameString = tentativeNames_.CurrentParentNameString;

        HierarchyNameData parentNameData;
        _int64 parentNameSequenceNumber = -1;

        // All hierarchy names must be created by extending a parent name in the hierarchy. 
        // Doing this has the effect of serializing subname and property writes but 
        // is necessary to be consistent with delete.
        //
        error = Store.TryReadData(
            txSPtr,
            dataType,
            parentNameString,
            parentNameData,
            parentNameSequenceNumber);

        if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteNoise(TraceComponent, "{0}: parent name {1} doesn't exist", this->TraceId, parentNameString);
            return ErrorCodeValue::NameNotFound;
        }
        else if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} cannot read parent name {1} to create hierarchy name {2}: error = {3}",
                this->TraceId,
                parentNameString,
                nameString,
                error);
            return error;
        }
        
        // Only the first name is written as explicit, and that if the original request was passed in as explicit
        bool isExplicit = isExplicitRequest_ && this->IsInitialName(nameString);

        WriteNoise(
            TraceComponent,
            "{0} AO create write transaction: name = {1}, explicit = {2}, complete = {3}",
            this->TraceId,
            nameString,
            isExplicit,
            tentativeNames_.IsCurrentNameCompletePending);
            
        // If the name already exists but wasn't completed successfully,
        // then overwrite with the new name state.
        //
        HierarchyNameData nameData(
            HierarchyNameState::Invalid, 
            UserServiceState::None, 
            DateTime::Now().Ticks); // base store version acts as instance id across deletes

        _int64 nameSequenceNumber = -1;
        error = Store.TryReadData(txSPtr, dataType, nameString, nameData, nameSequenceNumber);
       
        if (error.IsSuccess())
        {
            // If the name was previously created implicitly 
            // and this name is explicitly created, replace the flag
            // On recovery, keep the original explicit flag
            bool shouldUpdate = false;
            if (isExplicit && (!nameData.IsExplicit) && (!this->IsPrimaryRecovery))
            {
                nameData.IsExplicit = true;
                shouldUpdate = true;
            }

            if (nameData.NameState == HierarchyNameState::Created)
            {
                WriteInfo(
                    TraceComponent,
                    "{0} hierarchy name {1} already exists ({2}): user service = {3}, explicit={4}, shouldUpdate={5}", 
                    this->TraceId,
                    nameString,
                    nameData.NameState,
                    nameData.ServiceState,
                    nameData.IsExplicit, 
                    shouldUpdate);

                if (shouldUpdate && !tentativeNames_.IsCurrentNameCompletePending)
                {
                    // The name already exists, but it was created implicitly.
                    // Replace the state to be explicit and do not return error.
                    // Change the state to mark that this name is completed (not tentative),
                    // so no trip to name owner should be executed after the transaction is written.
                    tentativeNames_.SetCurrentNameCompletePending();
                }
                else
                {
                    return ErrorCodeValue::NameAlreadyExists;
                }
            }
            // During primary recovery: don't need to re-write tentative state if the explicitly create flag didn't change
            else if (!tentativeNames_.IsCurrentNameCompletePending
                && nameData.NameState == HierarchyNameState::Creating 
                && !shouldUpdate)
            {
                // Return an error to indicate that committing the transaction is this case
                // is not necessary.
                return ErrorCode(ErrorCodeValue::UpdatePending);
            }
        }
        else if (error.IsError(ErrorCodeValue::NotFound))
        {
            nameData.IsExplicit = isExplicit;

            error = ErrorCodeValue::Success;
        }
        
        if (error.IsSuccess())
        {
            // Preserve service state and subnames version number
            //
            nameData.NameState = (tentativeNames_.IsCurrentNameCompletePending ? HierarchyNameState::Created : HierarchyNameState::Creating); 
            
            error = Store.TryWriteData<HierarchyNameData>(txSPtr, nameData, dataType, nameString, nameSequenceNumber);
        }

        if (!error.IsSuccess())
        {
            return error;
        }

        if (tentativeNames_.IsCurrentNameCompletePending)
        {
            parentNameData.IncrementSubnamesVersion();
            
            // Update the parent name sequence number
            error = Store.TryWriteData<HierarchyNameData>(
                txSPtr,
                parentNameData,
                dataType,
                parentNameString,
                parentNameSequenceNumber);

            if (!error.IsSuccess())
            {
                return error;
            }

            // If there are any children that triggered put on the parent, write the first one in this transaction
            // with tentative state
            wstring childNameString;
            if (tentativeNames_.TryGetCurrentChildNameString(childNameString))
            {
                HierarchyNameData childNameData(
                    HierarchyNameState::Creating, 
                    UserServiceState::None, 
                    DateTime::Now().Ticks);
                childNameData.IsExplicit = isExplicitRequest_ && this->IsInitialName(childNameString);
                WriteNoise(
                    TraceComponent,
                    "{0} write tentative child {1} explicit={2}", 
                    this->TraceId,
                    childNameString,
                    childNameData.IsExplicit);
                error = Store.TryWriteData<HierarchyNameData>(txSPtr, childNameData, dataType, childNameString);
            }
        }

        if (error.IsSuccess())
        {
            outTxSPtr = move(txSPtr);
        }

        return error;
    }

    bool StoreService::ProcessCreateNameRequestAsyncOperation::IsInitialName(std::wstring const & nameString) const 
    { 
        return this->NameString == nameString; 
    }
}
