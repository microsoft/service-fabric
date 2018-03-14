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

    StringLiteral const TraceComponent("WriteServiceAtNameOwner");

    WriteServiceAtNameOwnerAsyncOperation::WriteServiceAtNameOwnerAsyncOperation(
        NamingUri const & name,
        __in NamingStore & store,
        bool isCreationComplete,
        bool shouldUpdatePsd,
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
      , shouldUpdatePsd_(shouldUpdatePsd)
      , psd_(psd)
      , timeoutHelper_(timeout)
      , lsn_(0)
    {
    }

    ErrorCode WriteServiceAtNameOwnerAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        FABRIC_SEQUENCE_NUMBER unusedLsn;
        return WriteServiceAtNameOwnerAsyncOperation::End(operation, unusedLsn);
    }

    ErrorCode WriteServiceAtNameOwnerAsyncOperation::End(
        AsyncOperationSPtr const & operation,
        __out FABRIC_SEQUENCE_NUMBER & result)
    {
        auto casted = AsyncOperation::End<WriteServiceAtNameOwnerAsyncOperation>(operation);
        
        if (casted->Error.IsSuccess())
        {
            casted->WriteNoise(
                TraceComponent,
                "{0}: write service at name owner complete: name = {1} complete = {2}",
                casted->TraceId,
                casted->name_,
                casted->isCreationComplete_);

            result = casted->lsn_;
        }
        else
        {
            casted->WriteInfo(
                TraceComponent,
                "{0}: write service at name owner failed: name = {1} complete = {2} error = {3}.",
                casted->TraceId,
                casted->name_,
                casted->isCreationComplete_,
                casted->Error);
        }

        return casted->Error;
    }

    void WriteServiceAtNameOwnerAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->StartWriteService(thisSPtr);
    }

    void WriteServiceAtNameOwnerAsyncOperation::StartWriteService(AsyncOperationSPtr const & thisSPtr)
    {
        TransactionSPtr txSPtr;
        auto error = store_.CreateTransaction(this->ActivityHeader, txSPtr);
        auto nameString = name_.ToString();

        NameData nameData;
        _int64 nameSequenceNumber = -1;

        if (error.IsSuccess())
        {
            error = store_.TryReadData(
                txSPtr,
                Constants::NonHierarchyNameEntryType,
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
                "{0} cannot read name {1} for create service: error = {2}",
                this->TraceId,
                nameString,
                error);
        }

        if (error.IsSuccess())
        {
            nameData.ServiceState = (isCreationComplete_ ? UserServiceState::Created : UserServiceState::Creating);

            // Tactical fix for Naming/FM consistency: allow write-through even if service already exists
            //
            error = store_.TryWriteData<NameData>(
                txSPtr,
                nameData,
                Constants::NonHierarchyNameEntryType,
                nameString,
                nameSequenceNumber);
        }

        if (error.IsSuccess() && shouldUpdatePsd_)
        {
            error = store_.TryWriteData<PartitionedServiceDescriptor>(txSPtr, psd_, Constants::UserServiceDataType, nameString);
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

    void WriteServiceAtNameOwnerAsyncOperation::OnCommitComplete(
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
