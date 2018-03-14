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

    StringLiteral const TraceComponent("RemoveServiceAtAuthority");

    RemoveServiceAtAuthorityAsyncOperation::RemoveServiceAtAuthorityAsyncOperation(
        NamingUri const & name,
        __in NamingStore & store,
        bool isDeletionComplete,
        bool isForceDelete,
        FabricActivityHeader const & activityHeader,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
      : AsyncOperation(callback, parent)
      , ActivityTracedComponent(store.TraceId, activityHeader)
      , name_(name)
      , store_(store)
      , isDeletionComplete_(isDeletionComplete)
      , isForceDelete_(isForceDelete)
      , timeoutHelper_(timeout)
    {
    }

    ErrorCode RemoveServiceAtAuthorityAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        bool temp;
        return RemoveServiceAtAuthorityAsyncOperation::End(operation, temp);
    }

    ErrorCode RemoveServiceAtAuthorityAsyncOperation::End(
        AsyncOperationSPtr const & operation,
        __out bool & isForceDelete)
    {
        auto casted = AsyncOperation::End<RemoveServiceAtAuthorityAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            casted->WriteNoise(
                TraceComponent,
                "{0}: remove service at authority owner complete: name = {1} isForceDelete = {2} complete = {3}",
                casted->TraceId,
                casted->name_,
                casted->isForceDelete_,
                casted->isDeletionComplete_);
            isForceDelete = casted->isForceDelete_;
        }
        else
        {
            casted->WriteInfo(
                TraceComponent,
                "{0}: remove service at authority owner failed: name = {1} complete = {2} error = {3}.",
                casted->TraceId,
                casted->name_,
                casted->isDeletionComplete_,
                casted->Error);
        }

        return casted->Error;
    }

    void RemoveServiceAtAuthorityAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->StartRemoveService(thisSPtr);
    }

    void RemoveServiceAtAuthorityAsyncOperation::StartRemoveService(AsyncOperationSPtr const & thisSPtr)
    {
        wstring const & dataType = Constants::HierarchyNameEntryType;

        TransactionSPtr txSPtr;
        ErrorCode error = store_.CreateTransaction(this->ActivityHeader, txSPtr);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        auto nameString = name_.ToString();

        HierarchyNameData nameData;
        __int64 nameSequenceNumber = -1;

        error = store_.TryReadData(
            txSPtr,
            dataType,
            nameString,
            nameData,
            nameSequenceNumber);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: Cannot read hierarchy name ({1}): error = {2}",
                this->TraceId,
                nameString,
                error);

            if (error.IsError(ErrorCodeValue::NotFound))
            {
                error = ErrorCodeValue::NameNotFound;
            }
        }

        if (error.IsSuccess() && nameData.ServiceState == UserServiceState::None)
        {
            WriteInfo(
                TraceComponent,
                "{0}: user service not found at authority for {1}.",
                this->TraceId,
                nameString);

            error = ErrorCodeValue::UserServiceNotFound;
        }

        if (error.IsSuccess())
        {
            if (!isDeletionComplete_)
            {
                // Recover force delete flag from potential failover
                //
                isForceDelete_ = (nameData.IsForceDelete || isForceDelete_);

                if ((nameData.ServiceState == UserServiceState::Deleting && !isForceDelete_) ||
                    (nameData.ServiceState == UserServiceState::ForceDeleting && isForceDelete_))
                {
                    // Optimization: don't re-write tentative state
                    //
                    NamingStore::CommitReadOnly(move(txSPtr));
                    this->TryComplete(thisSPtr, ErrorCodeValue::Success);
                    return;
                }
                else
                {
                    WriteInfo(
                        TraceComponent,
                        "{0}: Setting tentative flag for user service being deleted at {1} isForceDelete = {2}.",
                        this->TraceId,
                        nameString,
                        isForceDelete_);

                    nameData.ServiceState = isForceDelete_ ? UserServiceState::ForceDeleting : UserServiceState::Deleting;
                }
            }
            else
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: Removing tentative flag for user service deleted at {1}.",
                    this->TraceId,
                    nameString);
                nameData.ServiceState = UserServiceState::None;
            }
        }

        if (error.IsSuccess())
        {
            error = store_.TryWriteData<HierarchyNameData>(txSPtr, nameData, dataType, nameString, nameSequenceNumber);
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

    void RemoveServiceAtAuthorityAsyncOperation::OnCommitComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = NamingStore::EndCommit(operation);

        this->TryComplete(operation->Parent, error);
    }
}
