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

    StringLiteral const TraceComponent("UpdateServiceAtAuthority");

    UpdateServiceAtAuthorityAsyncOperation::UpdateServiceAtAuthorityAsyncOperation(
        NamingUri const & name,
        __in NamingStore & store,
        bool isUpdateComplete,
        ServiceUpdateDescription const & updateDescription,
        FabricActivityHeader const & activityHeader,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
      : AsyncOperation(callback, parent)
      , ActivityTracedComponent(store.TraceId, activityHeader)
      , name_(name)
      , store_(store)
      , isUpdateComplete_(isUpdateComplete)
      , updateDescription_(updateDescription)
      , timeoutHelper_(timeout)
    {
    }

    ErrorCode UpdateServiceAtAuthorityAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<UpdateServiceAtAuthorityAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            casted->WriteNoise(
                TraceComponent,
                "{0}: update service at authority owner complete: name = {1} complete = {2}",
                casted->TraceId,
                casted->name_,
                casted->isUpdateComplete_);
        }
        else
        {
            casted->WriteInfo(
                TraceComponent,
                "{0}: update service at authority owner failed: name = {1} complete = {2} error = {3}.",
                casted->TraceId,
                casted->name_,
                casted->isUpdateComplete_,
                casted->Error);
        }

        return casted->Error;
    }

    void UpdateServiceAtAuthorityAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->StartUpdateService(thisSPtr);
    }

    void UpdateServiceAtAuthorityAsyncOperation::StartUpdateService(AsyncOperationSPtr const & thisSPtr)
    {
        wstring const & hierarchyNameEntryType = Constants::HierarchyNameEntryType;

        TransactionSPtr txSPtr;
        auto error = store_.CreateTransaction(this->ActivityHeader, txSPtr);

        auto nameString = name_.ToString();

        HierarchyNameData nameData;
        __int64 nameSequenceNumber = -1; 

        if (error.IsSuccess())
        {
            error = store_.TryReadData(
                txSPtr,
                hierarchyNameEntryType,
                nameString,
                nameData,
                nameSequenceNumber);
        }

        if (error.IsError(ErrorCodeValue::NotFound))
        {
            error = ErrorCodeValue::NameNotFound;
        }
        else if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: Cannot read name ({1}): error = {2}",
                this->TraceId,
                name_,
                error);
        }

        if (error.IsSuccess())
        {
            if (nameData.NameState != HierarchyNameState::Created)
            {
                error = ErrorCodeValue::NameNotFound;
            }
        }

        if (error.IsSuccess())
        {
            if (isUpdateComplete_)
            {
                nameData.ServiceState = UserServiceState::Created;
            }
            else 
            {
                switch (nameData.ServiceState)
                {
                case UserServiceState::Created:
                    nameData.ServiceState = UserServiceState::Updating;
                    break;

                case UserServiceState::Updating:
                    // no-op: already in the correct state (probably from retry)
                    break;

                default:
                    error = ErrorCodeValue::UserServiceNotFound;
                }
            }
        }

        if (error.IsSuccess())
        {
            error = store_.TryWriteData<HierarchyNameData>(txSPtr, nameData, hierarchyNameEntryType, nameString, nameSequenceNumber);
        }

        if (error.IsSuccess())
        {
            wstring const & userServiceUpdateDataType = Constants::UserServiceUpdateDataType;

            if (isUpdateComplete_)
            {
                // Replication does not timeout. This means that even when an asynchronous commit
                // times out, the underlying replication is still pending and may complete any
                // time after the timeout. As long as the replication is pending, the underlying
                // database transaction is still open, so conflicting writes will still fail.
                //
                _int64 seqNumber = -1;
                error = store_.TryGetCurrentSequenceNumber(
                    txSPtr,
                    userServiceUpdateDataType,
                    nameString,
                    seqNumber);

                if (error.IsSuccess())
                {
                    error = store_.DeleteFromStore(txSPtr, userServiceUpdateDataType, nameString, seqNumber);
                }
                else if (error.IsError(ErrorCodeValue::NotFound))
                {
                    error = ErrorCodeValue::Success;
                }
            }
            else
            {
                error = store_.TryWriteData<ServiceUpdateDescription>(txSPtr, updateDescription_, userServiceUpdateDataType, nameString);
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
            this->TryComplete(thisSPtr, move(error));
        }
    }

    void UpdateServiceAtAuthorityAsyncOperation::OnCommitComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = NamingStore::EndCommit(operation);

        this->TryComplete(operation->Parent, move(error));
    }
}
