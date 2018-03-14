// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Transport;
    using namespace Store;

    StringLiteral const TraceComponent("RemoveServiceAtNameOwner");

    RemoveServiceAtNameOwnerAsyncOperation::RemoveServiceAtNameOwnerAsyncOperation(
        NamingUri const & name,
        __in NamingStore & store,
        bool isDeletionComplete,
        FabricActivityHeader const & activityHeader,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
      : AsyncOperation(callback, parent)
      , ActivityTracedComponent(store.TraceId, activityHeader)
      , name_(name)
      , store_(store)
      , isDeletionComplete_(isDeletionComplete)
      , timeoutHelper_(timeout)
      , lsn_(0)
    {
    }

    ErrorCode RemoveServiceAtNameOwnerAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        FABRIC_SEQUENCE_NUMBER unusedLsn;
        return RemoveServiceAtNameOwnerAsyncOperation::End(operation, unusedLsn);
    }

    ErrorCode RemoveServiceAtNameOwnerAsyncOperation::End(
        AsyncOperationSPtr const & operation,
        __out FABRIC_SEQUENCE_NUMBER & result)
    {
        auto casted = AsyncOperation::End<RemoveServiceAtNameOwnerAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            casted->WriteNoise(
                TraceComponent,
                "{0}: remove service at name owner complete: name = {1} complete = {2}",
                casted->TraceId,
                casted->name_,
                casted->isDeletionComplete_);

            result = casted->lsn_;
        }
        else
        {
            casted->WriteInfo(
                TraceComponent,
                "{0}: remove service at name owner failed: name = {1} complete = {2} error = {3}.",
                casted->TraceId,
                casted->name_,
                casted->isDeletionComplete_,
                casted->Error);
        }

        return casted->Error;
    }

    void RemoveServiceAtNameOwnerAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->StartRemoveService(thisSPtr);
    }

    void RemoveServiceAtNameOwnerAsyncOperation::StartRemoveService(AsyncOperationSPtr const & thisSPtr)
    {
        TransactionSPtr txSPtr;
        ErrorCode error = store_.CreateTransaction(this->ActivityHeader, txSPtr);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        auto nameString = name_.ToString();

        NameData nameData;
        _int64 nameSequenceNumber = -1;

        error = store_.TryReadData(
            txSPtr,
            Constants::NonHierarchyNameEntryType, 
            nameString,
            nameData,
            nameSequenceNumber);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} cannot read name {1} for delete service: error = {2}",
                this->TraceId,
                name_,
                error);

            if (error.IsError(ErrorCodeValue::NotFound))
            {
                error = ErrorCodeValue::NameNotFound;
            }
        }

        if (error.IsSuccess() &&  nameData.ServiceState == UserServiceState::None)
        {
            WriteInfo(
                TraceComponent,
                "{0} user service not found at name owner for {1}",
                this->TraceId,
                name_);

            error = ErrorCodeValue::UserServiceNotFound;
        }

        if (error.IsSuccess())
        {
            if (!isDeletionComplete_)
            {
                if (UserServiceState::IsDeleting(nameData.ServiceState))
                {
                    // Optimization: dont' re-write tentative state
                    NamingStore::CommitReadOnly(move(txSPtr));
                    this->TryComplete(thisSPtr, ErrorCodeValue::Success);
                    return;
                }
                else
                {
                    nameData.ServiceState = UserServiceState::Deleting;
                }
            }
            else
            {
                nameData.ServiceState = UserServiceState::None;
            }

            error = store_.TryWriteData<NameData>(
                txSPtr,
                nameData,
                Constants::NonHierarchyNameEntryType,
                nameString,
                nameSequenceNumber);
        }

        if (error.IsSuccess() && isDeletionComplete_)
        {
            wstring const & storeType = Constants::UserServiceDataType;
            _int64 psdSequenceNumber = -1;

            error = store_.TryGetCurrentSequenceNumber(txSPtr, storeType, nameString, psdSequenceNumber);

            if (error.IsSuccess())
            {
                error = store_.DeleteFromStore(txSPtr, storeType, nameString, psdSequenceNumber);
            }
            else if (error.IsError(ErrorCodeValue::NotFound))
            {
                error = ErrorCodeValue::Success;
            }
        }

        if (error.IsSuccess())
        {
            auto operation = NamingStore::BeginCommit(
                move(txSPtr),
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
                thisSPtr);
            this->OnCommitComplete(operation, true);
        }
        else
        {
            txSPtr.reset();
            this->TryComplete(thisSPtr, error);
        }
    }

    void RemoveServiceAtNameOwnerAsyncOperation::OnCommitComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = NamingStore::EndCommit(operation, lsn_);

        this->TryComplete(operation->Parent, error);
    }
}
