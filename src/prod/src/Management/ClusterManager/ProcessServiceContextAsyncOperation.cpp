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

StringLiteral const TraceComponent("ServiceContextAsyncOperation");

ProcessServiceContextAsyncOperation::ProcessServiceContextAsyncOperation(
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

void ProcessServiceContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();

    auto appContext = make_unique<ApplicationContext>(context_.ApplicationName);
    ErrorCode error = this->Replica.GetApplicationContextForServiceProcessing(storeTx, *appContext);

    if (error.IsSuccess())
    {
        error = this->Replica.ValidateServiceTypeDuringUpgrade(storeTx, *appContext, context_.ServiceTypeName);
    }

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();
    }
    else
    {
        storeTx.Rollback();

        // Let the rollout manager either retry waiting for the application context to complete
        // or fail and remove the service context
        //
        this->TryComplete(thisSPtr, error);
        return;
    }

    if (context_.IsCommitPending)
    {
        this->FinishCreateService(thisSPtr);
    }
    else
    {
        //
        // If service has a DNS name, add DNS property to Naming before the service is created.
        // Otherwise, proceed to service creation.
        //
        if (!this->context_.ServiceDescriptor.Service.ServiceDnsName.empty())
        {
            auto operation = this->BeginCreateServiceDnsName(
                this->context_.ServiceDescriptor.Service.Name,
                this->context_.ServiceDescriptor.Service.ServiceDnsName,
                this->ActivityId,
                this->RemainingTimeout,
                [this](AsyncOperationSPtr const &operation)
                {
                    this->OnCreateDnsNameComplete(operation, false);
                },
                thisSPtr);

            this->OnCreateDnsNameComplete(operation, true);
        }
        else
        {
            error = this->Replica.ScheduleNamingAsyncWork(
                [this, thisSPtr](AsyncCallback const & jobQueueCallback) { return this->BeginCreateService(thisSPtr, jobQueueCallback); },
                [this](AsyncOperationSPtr const & operation) { this->EndCreateService(operation); });
        }

        if (!error.IsSuccess()) { this->TryComplete(thisSPtr, error); }
    }
}

AsyncOperationSPtr ProcessServiceContextAsyncOperation::BeginCreateService(
    AsyncOperationSPtr const & thisSPtr,
    AsyncCallback const & jobQueueCallback)
{
    return this->Client.BeginCreateService(
        context_.ServiceDescriptor,
        context_.PackageVersion,
        context_.PackageInstance,
        context_.ActivityId,
        this->RemainingTimeout,
        jobQueueCallback,
        thisSPtr);
}

void ProcessServiceContextAsyncOperation::EndCreateService(AsyncOperationSPtr const & operation)
{
    ErrorCode error = this->Client.EndCreateService(operation);
    if (error.IsError(ErrorCodeValue::StaleServicePackageInstance))
    {
        // Just trace here. The conversion to a retryable error code
        // happens in ClientRequestAsyncOperation.cpp
        //
        WriteInfo(
            TraceComponent,
            "{0} stale service package: {1}:{2}",
            this->TraceId,
            context_.PackageVersion,
            context_.PackageInstance);
    }

    if (error.IsSuccess() || error.IsError(ErrorCodeValue::UserServiceAlreadyExists))
    {
        this->FinishCreateService(operation->Parent);
    }
    else
    {
        this->TryComplete(operation->Parent, error);
    }
}

ErrorCode ProcessServiceContextAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    auto casted = AsyncOperation::End<ProcessServiceContextAsyncOperation>(operation);
    return casted->Error;
}

void ProcessServiceContextAsyncOperation::FinishCreateService(AsyncOperationSPtr const & thisSPtr)
{
    context_.SetCommitPending();

    auto storeTx = this->CreateTransaction();

    ErrorCode error = context_.Complete(storeTx);

    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
    }
    else
    {
        auto operation = StoreTransaction::BeginCommit(
            move(storeTx), 
            context_,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
            thisSPtr);
        this->OnCommitComplete(operation, true);
    }
}

void ProcessServiceContextAsyncOperation::OnCommitComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
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

void ProcessServiceContextAsyncOperation::OnCreateDnsNameComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = this->EndCreateServiceDnsName(operation);
    if (error.IsSuccess())
    {
        error = this->Replica.ScheduleNamingAsyncWork(
            [this, thisSPtr](AsyncCallback const & jobQueueCallback) { return this->BeginCreateService(thisSPtr, jobQueueCallback); },
            [this](AsyncOperationSPtr const & operation) { this->EndCreateService(operation); });

        if (!error.IsSuccess()) { this->TryComplete(thisSPtr, error); }
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

