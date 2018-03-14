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

    StringLiteral const TraceComponent("WriteServiceAtAuthority");

    WriteServiceAtAuthorityAsyncOperation::WriteServiceAtAuthorityAsyncOperation(
        NamingUri const & name,
        __in NamingStore & store,
        bool isCreationComplete,
        PartitionedServiceDescriptor const & psd,
        FabricActivityHeader const & activityHeader,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
      : AsyncOperation(callback, parent)
      , ActivityTracedComponent(store.TraceId, activityHeader)
      , name_(name)
      , store_(store)
      , isCreationComplete_(isCreationComplete)
      , psd_(psd)
      , timeoutHelper_(timeout)
    {
    }

    ErrorCode WriteServiceAtAuthorityAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<WriteServiceAtAuthorityAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            casted->WriteNoise(
                TraceComponent,
                "{0}: write service at authority owner complete: name = {1} complete = {2}",
                casted->TraceId,
                casted->name_,
                casted->isCreationComplete_);
        }
        else if (casted->Error.IsError(ErrorCodeValue::NameNotFound))
        {
            // NameNotFound is a common case for recursively created names, optimize Info-level tracing here
            //
            casted->WriteNoise(
                TraceComponent,
                "{0}: write service at authority owner failed due to missing name = {1} complete = {2}",
                casted->TraceId,
                casted->name_,
                casted->isCreationComplete_);
        }
        else
        {
            casted->WriteInfo(
                TraceComponent,
                "{0}: write service at authority owner failed: name = {1} complete = {2} error = {3}.",
                casted->TraceId,
                casted->name_,
                casted->isCreationComplete_,
                casted->Error);
        }

        return casted->Error;
    }

    void WriteServiceAtAuthorityAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->StartWriteService(thisSPtr);
    }

    void WriteServiceAtAuthorityAsyncOperation::StartWriteService(AsyncOperationSPtr const & thisSPtr)
    {
        wstring const & dataType = Constants::HierarchyNameEntryType;

        TransactionSPtr txSPtr;
        auto error = store_.CreateTransaction(this->ActivityHeader, txSPtr);
        auto nameString = name_.ToString();

        HierarchyNameData nameData;
        __int64 nameSequenceNumber = -1; 

        if (error.IsSuccess())
        {
            error = store_.TryReadData(
                txSPtr,
                dataType,
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
            // Allow duplicate create service requests through the system to ensure consistency of user service
            // state between Naming/FM until Naming Service can support dataloss
            //
            nameData.ServiceState = (isCreationComplete_ ? UserServiceState::Created : UserServiceState::Creating);

            error = store_.TryWriteData<HierarchyNameData>(txSPtr, nameData, dataType, nameString, nameSequenceNumber);
        }

        if (error.IsSuccess())
        {
            wstring const & psdDataType = Constants::UserServiceRecoveryDataType;

            if (isCreationComplete_)
            {
                // Replication does not timeout. This means that even when an asynchronous commit
                // times out, the underlying replication is still pending and may complete any
                // time after the timeout. As long as the replication is pending, the underlying
                // database transaction is still open, so conflicting writes will still fail.
                //
                _int64 psdSequenceNumber = -1;
                error = store_.TryGetCurrentSequenceNumber(
                    txSPtr,
                    psdDataType,
                    nameString,
                    psdSequenceNumber);

                if (error.IsSuccess())
                {
                    error = store_.DeleteFromStore(txSPtr, psdDataType, nameString, psdSequenceNumber);
                }
                else if (error.IsError(ErrorCodeValue::NotFound))
                {
                    error = ErrorCodeValue::Success;
                }
            }
            else
            {
                error = store_.TryWriteData<PartitionedServiceDescriptor>(txSPtr, psd_, psdDataType, nameString);
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

    void WriteServiceAtAuthorityAsyncOperation::OnCommitComplete(
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

