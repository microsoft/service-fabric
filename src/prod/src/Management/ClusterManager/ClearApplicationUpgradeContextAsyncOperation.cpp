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

StringLiteral const TraceComponent("ClearApplicationUpgradeContextAsyncOperation");

ClearApplicationUpgradeContextAsyncOperation::ClearApplicationUpgradeContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in ApplicationUpgradeContext & context,
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

void ClearApplicationUpgradeContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();

    // If the upgrade hasn't been accepted yet (client will
    // not have received a reply yet), then delete the context.
    //
    ErrorCode error = ClearApplicationUpgradeContextAsyncOperation::ClearUpgradingStatus(
        storeTx, 
        context_, 
        context_.IsPreparingUpgrade); //shouldDeleteUpgradeContext
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

ErrorCode ClearApplicationUpgradeContextAsyncOperation::ClearUpgradingStatus(Store::StoreTransaction const &storeTx, ApplicationUpgradeContext &context, bool shouldDeleteUpgradeContext)
{
    auto const & applicationName = context.UpgradeDescription.ApplicationName;

    auto appContext = make_unique<ApplicationContext>(applicationName);
    ErrorCode error = storeTx.ReadExact(*appContext);
    if (error.IsSuccess())
    {
        // Clear upgrade pending flag but leave instance unchanged
        error = appContext->SetUpgradeComplete(storeTx, appContext->PackageInstance);

        if (error.IsSuccess())
        {
            if (shouldDeleteUpgradeContext)
            {
                error = storeTx.DeleteIfFound(context);
            }
            else
            {
                // If the upgrade has already been accepted, then persist the failed 
                // status after successfully clearing upgrade status on the 
                // application context.
                //
                error = context.UpdateStatus(storeTx, RolloutStatus::Failed);
            }
        }
    }
    else if (error.IsError(ErrorCodeValue::NotFound))
    {
        // Delete application takes care of deleting the upgrade context
        // and the verified upgrade domains.
        //
        error = ErrorCodeValue::Success;
    }

    return error;
}

void ClearApplicationUpgradeContextAsyncOperation::OnCommitComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    this->TryComplete(operation->Parent, error);
}
