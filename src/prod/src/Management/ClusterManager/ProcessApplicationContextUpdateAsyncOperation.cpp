// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management::ClusterManager;
using namespace Reliability;
using namespace ServiceModel;
using namespace Common;
using namespace Store;

StringLiteral const TraceComponent("ApplicationContextUpdateAsyncOperation");

ProcessApplicationContextUpdateAsyncOperation::ProcessApplicationContextUpdateAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in ApplicationUpdateContext & context,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root,
    bool forwardOperation)
    : ProcessRolloutContextAsyncOperationBase(
    rolloutManager,
    context.ReplicaActivityId,
    timeout,
    callback,
    root)
    , context_(context)
    , applicationContext_(context_.ApplicationName)
    , forwardOperation_(forwardOperation)
{
}

void ProcessApplicationContextUpdateAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    auto error = ApplicationIdentifier::FromString(context_.ApplicationId.Value, appId_);

    if (error.IsSuccess())
    {
        // Get the application context from the store
        auto storeTx = this->CreateTransaction();
        error = this->Replica.GetCompletedOrUpgradingApplicationContext(storeTx, applicationContext_);

        if (error.IsSuccess())
        {
            // Udate application capacity. Do not check for error.
            // Replica has already checked if updated parameters are valid and failed operation if not.
            if (forwardOperation_)
            {
                // In case of update, use updated capacities and increment update ID by 1.
                applicationContext_.UpdateID = applicationContext_.UpdateID + 1;
                applicationContext_.SetApplicationCapacity(context_.UpdatedCapacities);
            }
            else
            {
                // In case of reverting the update, use original capacities and increment update ID by 2.
                // If forward operation has already sent update to FM, then FM has incremented update ID
                // Increment by 2 is necessary so that FM will accept the new version.
                applicationContext_.UpdateID = applicationContext_.UpdateID + 2;
                applicationContext_.SetApplicationCapacity(context_.CurrentCapacities);
            }

            UpdateApplicationRequestMessageBody body(
                context_.ApplicationName,
                this->appId_,
                applicationContext_.PackageInstance,
                applicationContext_.UpdateID,
                applicationContext_.ApplicationCapacity);

            auto request = RSMessage::GetUpdateApplicationRequest().CreateMessage(body);
            request->Headers.Add(Transport::FabricActivityHeader(context_.ActivityId));

            WriteNoise(TraceComponent, "{0} sending request to FM for application {1}: {2}", this->TraceId, context_.ApplicationName, body);

            auto operation = this->Router.BeginRequestToFM(
                move(request),
                TimeSpan::MaxValue,
                this->RemainingTimeout,
                [this](AsyncOperationSPtr const & operation) { this->OnRequestToFMComplete(operation, false); },
                thisSPtr);
            this->OnRequestToFMComplete(operation, true);
        }
        else
        {
            if (error.ReadValue() == ErrorCodeValue::ApplicationNotFound)
            {
                // Application got deleted while we were updating it.
                // If going forward, we need to fail the update (this error is not retryable).
                if (forwardOperation_)
                {
                    // Don't allow updates if application context is pending
                    WriteError(TraceComponent,
                        "{0} Encountered ApplicationNotFound while attempting to update application. Application ID '{1}'",
                        this->TraceId,
                        this->appId_);
                    this->TryComplete(thisSPtr, error);
                }
                else
                {
                    // When reverting, there is nothing that we need to revert because application does not exist.
                    // So, just delete the context and finish.
                    this->DeleteUpdateContext(thisSPtr);
                }
            }
            else
            {
                // Let RolloutManager retry on oher errors when reading ApplicationContext
                this->TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
            }
        }
    }
    else
    {
        WriteError(
            TraceComponent,
            "{0} failed to parse as application ID: '{1}'",
            this->TraceId,
            context_.ApplicationId);
        this->TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
    }
}

void ProcessApplicationContextUpdateAsyncOperation::OnRequestToFMComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto const & thisSPtr = operation->Parent;

    Transport::MessageUPtr reply;
    auto error = this->Router.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        BasicFailoverReplyMessageBody body;
        if (reply->GetBody(body))
        {
            error = body.ErrorCodeValue;
        }
        else
        {
            error = ErrorCode::FromNtStatus(reply->GetStatus());
        }
    }

    if (error.IsSuccess())
    {
        this->CommitContexts(thisSPtr);
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to route update request to FM: application={1} error={2}",
            this->TraceId,
            context_.ApplicationName,
            error);
        // Let RolloutManager decide if this is worh a retry.
        this->TryComplete(thisSPtr, error);
    }
}

void ProcessApplicationContextUpdateAsyncOperation::DeleteUpdateContext(Common::AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();
    auto error = storeTx.DeleteIfFound(context_);

    if (error.IsSuccess())
    {
        auto operation = StoreTransaction::BeginCommit(
            std::move(storeTx),
            context_,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
            thisSPtr);
        this->OnCommitComplete(operation, true);
    }
    else
    {
        WriteNoise(TraceComponent, "{0} encountered an error while deleting update context for application {1}, error={2}", this->TraceId, context_.ApplicationName, error);
        this->TryComplete(thisSPtr, error);
    }
}

void ProcessApplicationContextUpdateAsyncOperation::CommitContexts(Common::AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();

    auto appContextUPtr = make_unique<ApplicationContext>(std::move(applicationContext_));
    auto error = storeTx.Update(*appContextUPtr);

    if (error.IsSuccess())
    {
        error = storeTx.DeleteIfFound(context_);

        if (error.IsSuccess())
        {
            auto operation = StoreTransaction::BeginCommit(
                std::move(storeTx),
                context_,
                [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
                thisSPtr);
            this->OnCommitComplete(operation, true);

        }
        else
        {
            WriteNoise(TraceComponent, "{0} encountered an error while deleting update context for application {1}, error={2}", this->TraceId, context_.ApplicationName, error);
            this->TryComplete(thisSPtr, error);
        }
    }
    else
    {
        WriteNoise(TraceComponent, "{0} encountered an error while updating application context for application {1}, error={2}", this->TraceId, context_.ApplicationName, error);
        this->TryComplete(thisSPtr, error);
    }
}

void ProcessApplicationContextUpdateAsyncOperation::OnCommitComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    if (error.IsSuccess())
    {
        WriteNoise(TraceComponent, "{0} completed the update of application context for {1}", this->TraceId, context_.ApplicationName);
    }
    else
    {
        WriteNoise(TraceComponent, "{0} encountered an error while ending commit for application {1} error={2}", this->TraceId, context_.ApplicationName, error);
    }

    this->TryComplete(operation->Parent, error);
}

ErrorCode ProcessApplicationContextUpdateAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    auto casted = AsyncOperation::End<ProcessApplicationContextUpdateAsyncOperation>(operation);
    return casted->Error;
}


