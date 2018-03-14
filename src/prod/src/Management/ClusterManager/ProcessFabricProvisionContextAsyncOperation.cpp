// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace Store;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("FabricProvisionContextAsyncOperation");

ProcessFabricProvisionContextAsyncOperation::ProcessFabricProvisionContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in FabricProvisionContext & context,
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

void ProcessFabricProvisionContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    ErrorCode error(ErrorCodeValue::Success);

    if (error.IsSuccess())
    {
        if (context_.IsProvision)
        {
            error = this->ImageBuilder.ProvisionFabric(
                context_.CodeFilepath,
                context_.ClusterManifestFilepath,
                this->RemainingTimeout);
        }
        else
        {
            error = this->ImageBuilder.CleanupFabricVersion(
                context_.Version,
                this->RemainingTimeout);

            if (!error.IsSuccess() && this->IsImageBuilderSuccessOnCleanup(error))
            {
                error = ErrorCodeValue::Success;
            }
        }
    }

    if (error.IsSuccess())
    {
        auto storeTx = this->CreateTransaction();

        if (context_.IsProvision)
        {
            error = context_.CompleteProvisioning(storeTx);
        }
        else
        {
            error = context_.CompleteUnprovisioning(storeTx);
        }

        if (error.IsSuccess())
        {
            auto operation = StoreTransaction::BeginCommit(
                move(storeTx),
                context_,
                [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
                thisSPtr);
            this->OnCommitComplete(operation, true);
        }
    }

    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
    }
}

void ProcessFabricProvisionContextAsyncOperation::OnCommitComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    this->TryComplete(operation->Parent, error);
}

ErrorCode ProcessFabricProvisionContextAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<ProcessFabricProvisionContextAsyncOperation>(operation)->Error;
}
