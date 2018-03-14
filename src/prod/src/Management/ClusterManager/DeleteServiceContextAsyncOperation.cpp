// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Management/DnsService/include/DnsPropertyValue.h"

using namespace Common;
using namespace std;
using namespace Store;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("DeleteServiceContextAsyncOperation");

DeleteServiceContextAsyncOperation::DeleteServiceContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in ServiceContext & context,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ProcessRolloutContextAsyncOperationBase(
        rolloutManager,
        context.ReplicaActivityId,
        timeout,
        callback, 
        root)
    , context_(context)
{
}

void DeleteServiceContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();

    auto appContext = make_unique<ApplicationContext>(context_.ApplicationName);
    ErrorCode error = this->Replica.GetApplicationContextForServiceProcessing(storeTx, *appContext);

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();
    }
    else
    {
        storeTx.Rollback();

        // DeleteApplication will have deleted this service already
        if (context_.Error.IsError(ErrorCodeValue::ApplicationNotFound))
        {
            error = ErrorCodeValue::Success;
        }

        this->TryComplete(thisSPtr, error);
        return;
    }

    if (context_.IsCommitPending)
    {
        this->FinishDelete(thisSPtr);
        return;
    }

    error = this->Replica.ScheduleNamingAsyncWork(
        [this, thisSPtr](AsyncCallback const & jobQueueCallback)
        {
            return this->BeginDeleteService(thisSPtr, jobQueueCallback);
        },
        [this](AsyncOperationSPtr const & operation) { this->EndDeleteService(operation); });
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
    }
}

AsyncOperationSPtr DeleteServiceContextAsyncOperation::BeginDeleteService(
    Common::AsyncOperationSPtr const & thisSPtr,
    AsyncCallback const & jobQueueCallback)
{
    return this->Client.BeginDeleteService(
        context_.ApplicationName,
        ServiceModel::DeleteServiceDescription(
            context_.AbsoluteServiceName,
            context_.IsForceDelete),
        context_.ActivityId,
        this->RemainingTimeout,
        jobQueueCallback,
        thisSPtr);
}

void DeleteServiceContextAsyncOperation::EndDeleteService(AsyncOperationSPtr const & operation)
{
    Common::AsyncOperationSPtr const & thisSPtr = operation->Parent;

    ErrorCode error = this->Client.EndDeleteService(operation);
    if (error.IsSuccess() || error.IsError(ErrorCodeValue::UserServiceNotFound))
    {
        if (this->context_.ServiceDescriptor.Service.ServiceDnsName.empty())
        {
            this->FinishDelete(thisSPtr);
        }
        else
        {
            error = this->Replica.ScheduleNamingAsyncWork(
                [this, thisSPtr](AsyncCallback const & jobQueueCallback)
                {
                    return this->BeginDeleteServiceDnsName(
                        this->context_.ServiceDescriptor.Service.Name,
                        this->context_.ServiceDescriptor.Service.ServiceDnsName,
                        ActivityId, RemainingTimeout, jobQueueCallback, thisSPtr);
                },
                [this](AsyncOperationSPtr const & operation) { this->OnDeleteServiceDnsNameComplete(operation); });

            if (!error.IsSuccess()) { this->TryComplete(thisSPtr, error); }
        }
    }
    else
    {
        this->TryComplete(operation->Parent, error);
    }
}

void DeleteServiceContextAsyncOperation::OnDeleteServiceDnsNameComplete(Common::AsyncOperationSPtr const & operation)
{
    ErrorCode error = this->EndDeleteServiceDnsName(operation);
    if (error.IsSuccess())
    {
        this->FinishDelete(operation->Parent);
    }
    else
    {
        this->TryComplete(operation->Parent, error);
    }
}

void DeleteServiceContextAsyncOperation::FinishDelete(AsyncOperationSPtr const & thisSPtr)
{
    context_.SetCommitPending();

    auto storeTx = this->CreateTransaction();

    ErrorCode error = storeTx.DeleteIfFound(context_);

    if (error.IsSuccess())
    {
        auto operation = StoreTransaction::BeginCommit(
            move(storeTx),
            context_,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
            thisSPtr);
        this->OnCommitComplete(operation, true);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void DeleteServiceContextAsyncOperation::OnCommitComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    if (error.IsSuccess())
    {
        context_.ResetCommitPending();
    }

    this->TryComplete(operation->Parent, error);
}

ErrorCode DeleteServiceContextAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    auto casted = AsyncOperation::End<DeleteServiceContextAsyncOperation>(operation);
    return casted->Error;
}
