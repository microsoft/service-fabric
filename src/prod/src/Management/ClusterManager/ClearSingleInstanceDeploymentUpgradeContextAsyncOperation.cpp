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

StringLiteral const TraceComponent("ClearSingleInstanceDeploymentUpgradeContextAsyncOperation");

ClearSingleInstanceDeploymentUpgradeContextAsyncOperation::ClearSingleInstanceDeploymentUpgradeContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in SingleInstanceDeploymentUpgradeContext & context,
    TimeSpan const & timeout,
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

void ClearSingleInstanceDeploymentUpgradeContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    auto const & deploymentName = context_.DeploymentName;
    auto storeTx = this->CreateTransaction();

    auto deploymentContext = make_unique<SingleInstanceDeploymentContext>(deploymentName);
    ErrorCode error = storeTx.ReadExact(*deploymentContext);
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        this->WriteWarning(
            TraceComponent,
            "{0} SingleInstanceDeploymentContext {1} was already deleted before upgrade completes",
            context_.TraceId,
            deploymentName);

        Assert::TestAssert(
            TraceComponent,
            "{0} SingleInstanceDeploymentContext {1} was already deleted before upgrade completes",
            context_.TraceId,
            deploymentName);

        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        return;
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to read SingleInstanceDeploymentContext {1}. Error {2}",
            context_.TraceId,
            deploymentName,
            error);
        this->TryComplete(thisSPtr, move(error));
        return;
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} Clearing single instance deployment upgrade. DeploymentName {1}",
            context_.TraceId,
            deploymentName);
    }

    // If upgrading, clear ApplicationContext and ApplicationUpgradeContext
    if (context_.ShouldUpgrade())
    {
        auto applicationUpgradeContext = make_unique<ApplicationUpgradeContext>(context_.ApplicationName);
        error = storeTx.ReadExact(*applicationUpgradeContext);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} failed to read ApplicationUpgradeContext. Error {1}",
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
                "{0} failed to clear upgrade status from application contexts. Error {1}",
                context_.TraceId,
                error);
            this->TryComplete(thisSPtr, move(error));
            return;
        }
    }
    //TODO: Deal with scenario that UD failed in the middle
    else if (context_.ShouldProvision())
    {
        auto targetAppTypeContext = make_shared<ApplicationTypeContext>(
            context_.TypeName,
            context_.TargetTypeVersion);
        error = storeTx.DeleteIfFound(*targetAppTypeContext);
        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} Deleted target ApplicationTypeContext {1} {2}",
                context_.TraceId,
                context_.TypeName,
                context_.TargetTypeVersion);
        }

        auto targetStoreData = make_shared<StoreDataSingleInstanceApplicationInstance>(
            context_.DeploymentName,
            context_.TargetTypeVersion);
        error = storeTx.DeleteIfFound(*targetStoreData);
        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} Deleted target StoreDataSingleInstanceApplicationInstance {1} {2}",
                context_.TraceId,
                context_.DeploymentName,
                context_.TargetTypeVersion);
        }
    }

    // Clear context_
    error = context_.UpdateStatus(storeTx, RolloutStatus::Failed);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
        return;
    }

    error = context_.UpdateUpgradeStatus(storeTx, SingleInstanceDeploymentUpgradeState::Enum::Failed);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
        return;
    }

    error = deploymentContext->ClearUpgrading(storeTx, GET_RC(Deployment_Upgrade_Failed));
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

void ClearSingleInstanceDeploymentUpgradeContextAsyncOperation::OnCommitComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (expectedCompletedSynchronously != operation->CompletedSynchronously)
    {
        return;
    }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    this->TryComplete(operation->Parent, move(error));
}
