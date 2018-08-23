//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace Store;
using namespace ServiceModel;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ReplaceSingleInstanceDeploymentContextAsyncOperation");

ReplaceSingleInstanceDeploymentContextAsyncOperation::ReplaceSingleInstanceDeploymentContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in SingleInstanceDeploymentContext & context,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : DeleteSingleInstanceDeploymentContextAsyncOperation(
        rolloutManager,
        context,
        timeout,
        callback,
        root)
{
}

void ReplaceSingleInstanceDeploymentContextAsyncOperation::InnerDeleteApplication(
    AsyncOperationSPtr const & thisSPtr)
{
    auto operation = AsyncOperation::CreateAndStart<DeleteApplicationContextAsyncOperation>(
        const_cast<RolloutManager&>(this->Rollout),
        applicationContext_,
        true, // keepContextAlive
        TimeSpan::FromMinutes(2), // TODO: this should be a configurable timeout within max val.
        [this](AsyncOperationSPtr const & operation) { this->OnInnerDeleteApplicationComplete(operation, false); },
        thisSPtr);

    this->OnInnerDeleteApplicationComplete(operation, true);
}

void ReplaceSingleInstanceDeploymentContextAsyncOperation::CompleteDeletion(
    AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();

    this->RefreshContext();
    StoreDataGlobalInstanceCounter instanceCounter;

    ServiceModelApplicationId applicationId;
    ErrorCode error = instanceCounter.GetNextApplicationId(
        storeTx,
        singleInstanceDeploymentContext_.TypeName,
        singleInstanceDeploymentContext_.NewTypeVersion,
        applicationId);
    if (!error.IsSuccess())
    {
        this->WriteInfo(
            TraceComponent,
            "{0} GetNextApplicationId for {1} {2} failed with {3}",
            this->TraceId,
            singleInstanceDeploymentContext_.TypeName,
            singleInstanceDeploymentContext_.NewTypeVersion.Value,
            error);
        this->RefreshContext();
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CompleteDeletion(thisSPtr); });
        return;
    }

    singleInstanceDeploymentContext_.ApplicationId = applicationId;
    singleInstanceDeploymentContext_.GlobalInstanceCount = instanceCounter.Value;
    singleInstanceDeploymentContext_.TypeVersion = singleInstanceDeploymentContext_.NewTypeVersion;
    singleInstanceDeploymentContext_.NewTypeVersion = ServiceModelVersion();

    // Update store as well
    error = singleInstanceDeploymentContext_.SwitchReplaceToCreate(storeTx);
    if (!error.IsSuccess())
    {
        this->WriteInfo(
            TraceComponent,
            "{0} SwitchReplaceToCreate for {1} failed with {2}",
            this->TraceId,
            singleInstanceDeploymentContext_.DeploymentName,
            error);
        this->RefreshContext();
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CompleteDeletion(thisSPtr); });
        return;
    }

    instanceCounter.UpdateNextApplicationId(storeTx);

    auto applicationContext = make_shared<ApplicationContext>(
        singleInstanceDeploymentContext_.ReplicaActivityId,
        singleInstanceDeploymentContext_.ApplicationName,
        singleInstanceDeploymentContext_.ApplicationId,
        singleInstanceDeploymentContext_.GlobalInstanceCount,
        singleInstanceDeploymentContext_.TypeName,
        singleInstanceDeploymentContext_.TypeVersion,
        L"",
        map<wstring, wstring>(),
        ApplicationDefinitionKind::Enum::MeshApplicationDescription);

    // The current application context was not deleted.
    // If single instance deployment failed, the application context was never created.
    error = storeTx.UpdateOrInsertIfNotFound(*applicationContext);
    if (!error.IsSuccess())
    {
        this->WriteInfo(
            TraceComponent,
            "{0} update application context {1} failed with {2}",
            this->TraceId,
            applicationContext->ApplicationName,
            error);
        this->RefreshContext();
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CompleteDeletion(thisSPtr); });
    }
    else
    {
        auto operation = StoreTransaction::BeginCommit(
            move(storeTx),
            singleInstanceDeploymentContext_,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitSwitchReplaceToCreateComplete(operation, false); },
            thisSPtr);
        this->OnCommitSwitchReplaceToCreateComplete(operation, true);
    }
}

void ReplaceSingleInstanceDeploymentContextAsyncOperation::OnFinishDeleteComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
    ErrorCode error = StoreTransaction::EndCommit(operation);
    this->CompleteDeletion(operation->Parent);
}

void ReplaceSingleInstanceDeploymentContextAsyncOperation::OnCommitSwitchReplaceToCreateComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    auto const & thisSPtr = operation->Parent;

    if (!error.IsSuccess())
    {
        this->WriteInfo(
            TraceComponent,
            "{0} commit switch replace to create failed with {1}",
            this->TraceId,
            error);
        this->RefreshContext();
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CompleteDeletion(thisSPtr); });
    }
    else
    {
        this->WriteInfo(
        TraceComponent,
        "{0} Replace single instance deployment {1} completed.",
        this->TraceId,
        singleInstanceDeploymentContext_.DeploymentName);
 
        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
}
