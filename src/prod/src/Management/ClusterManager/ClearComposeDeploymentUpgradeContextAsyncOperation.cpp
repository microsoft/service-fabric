// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace Store;
using namespace ServiceModel;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ClearComposeDeploymentUpgradeContextAsyncOperation");

ClearComposeDeploymentUpgradeContextAsyncOperation::ClearComposeDeploymentUpgradeContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in ComposeDeploymentUpgradeContext & context,
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

void ClearComposeDeploymentUpgradeContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    auto const & deploymentName = context_.DeploymentName;

    auto storeTx = this->CreateTransaction();

    auto deploymentContext = make_unique<ComposeDeploymentContext>(deploymentName);
    auto error = storeTx.ReadExact(*deploymentContext);
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        error = ErrorCodeValue::Success;

        WriteInfo(
            TraceComponent,
            "{0} ComposeDeploymentContext was already deleted",
            context_.TraceId);

        this->TryComplete(thisSPtr, move(error));
        return;
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to read ComposeDeploymentContext - error : {1}",
            context_.TraceId,
            error);

        this->TryComplete(thisSPtr, move(error));
        return;
    }

    //
    // If the context state is provisioning, then we just mark that context as completed, and clear the pending flag in the 
    //  compose deployment.
    // If the context is in upgrading, then the failure is during the upgrade or rollback. We just clear the pending flag in
    //  the app contexts and the compose contexts. We dont unprovision since we might be mid way on a upgrade or rollback. Delete
    //  handles the 2nd apptype
    // If failed during unprovision, then we can just complete as success and let the delete handle the orphan apptype.
    //
    if (context_.ShouldUpgrade())
    {
        //
        // ApplicationContext and ApplicationUpgradeContext have been setup for upgrading. Clear the status.
        //
        auto applicationUpgradeContext = make_unique<ApplicationUpgradeContext>(deploymentContext->ApplicationName);
        error = storeTx.ReadExact(*applicationUpgradeContext);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} failed to read application upgrade context - error : {1}",
                context_.TraceId,
                error);

            this->TryComplete(thisSPtr, move(error));
            return;
        }

        error = ClearApplicationUpgradeContextAsyncOperation::ClearUpgradingStatus(
            storeTx, 
            *applicationUpgradeContext, 
            false); // ShouldDeleteUpgradeContext
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} failed to clear upgrading bit from application contexts - error : {1}",
                context_.TraceId,
                error);

            this->TryComplete(thisSPtr, move(error));
            return;
        }
    }

    // set the rollout status of the context to failed
    error = context_.UpdateStatus(storeTx, RolloutStatus::Failed);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
        return;
    }

    error = context_.UpdateUpgradeStatus(storeTx, ComposeDeploymentUpgradeState::Enum::Failed);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
        return;
    }

    error = deploymentContext->ClearUpgrading(storeTx, GET_RC( Deployment_Upgrade_Failed ));
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
        this->TryComplete(thisSPtr, move(error));
    }
}

void ClearComposeDeploymentUpgradeContextAsyncOperation::OnCommitComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    this->TryComplete(operation->Parent, move(error));
}
