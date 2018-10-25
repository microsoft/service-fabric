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
using namespace Reliability;
using namespace Management::ClusterManager;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("DeleteComposeDeploymentContextAsyncOperation");

class DeleteComposeDeploymentContextAsyncOperation::InnerDeleteApplicationContextAsyncOperation
    : public DeleteApplicationContextAsyncOperation
{
    DENY_COPY(InnerDeleteApplicationContextAsyncOperation);

public:
    InnerDeleteApplicationContextAsyncOperation(
        __in DeleteComposeDeploymentContextAsyncOperation &owner,
        __in RolloutManager &rolloutManager,
        __in ApplicationContext &appContext,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const &parent)
        : DeleteApplicationContextAsyncOperation(
            rolloutManager,
            appContext,
            timeout,
            callback,
            parent)
        , owner_(owner)
    {
    }

private:
    void CommitDeleteApplication(
        AsyncOperationSPtr const &thisSPtr,
        StoreTransaction &storeTx)
    {
        auto error = owner_.UpdateContextStatusAfterDeleteApplication(storeTx);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        auto operation = StoreTransaction::BeginCommit(
            move(storeTx),
            owner_.dockerComposeDeploymentContext_,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitDeleteApplicationComplete(operation, false); },
            thisSPtr);
        this->OnCommitDeleteApplicationComplete(operation, true);
    }

    DeleteComposeDeploymentContextAsyncOperation &owner_;
};

class DeleteComposeDeploymentContextAsyncOperation::InnerDeleteApplicationTypeContextAsyncOperation 
    : public DeleteApplicationTypeContextAsyncOperation
{
    DENY_COPY(InnerDeleteApplicationTypeContextAsyncOperation);

public:
    InnerDeleteApplicationTypeContextAsyncOperation(
        __in DeleteComposeDeploymentContextAsyncOperation &owner,
        __in RolloutManager &rolloutManager,
        __in ApplicationTypeContext &appTypeContext,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const &parent)
        : DeleteApplicationTypeContextAsyncOperation(
            rolloutManager,
            appTypeContext,
            timeout,
            callback,
            parent)
        , owner_(owner)
    {
    }

protected:
    void CommitDeleteApplicationType(AsyncOperationSPtr const & thisSPtr, StoreTransaction &storeTx) override
    {
        auto error = owner_.UpdateContextStatusAfterUnprovision(storeTx);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        auto operation = StoreTransaction::BeginCommit(
            move(storeTx),
            owner_.dockerComposeDeploymentContext_,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
            thisSPtr);
        this->OnCommitComplete(operation, true);
    }

private:
    DeleteComposeDeploymentContextAsyncOperation &owner_;
};

DeleteComposeDeploymentContextAsyncOperation::DeleteComposeDeploymentContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in ComposeDeploymentContext & context,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ProcessRolloutContextAsyncOperationBase(
        rolloutManager,
        context.ReplicaActivityId,
        timeout,
        callback,
        root)
    , dockerComposeDeploymentContext_(context)
    , applicationContext_(
        context.ReplicaActivityId,
        context.ApplicationName,
        context.ApplicationId,
        context.GlobalInstanceCount,
        context.TypeName,
        context.TypeVersion,
        L"", // docker compose applications don't have a manifestId currently
        map<wstring, wstring>(),
        ApplicationDefinitionKind::Enum::Compose)
    , applicationTypeContext_(
        context.TypeName,
        context.TypeVersion)
{
}

void DeleteComposeDeploymentContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (this->ShouldDeleteApplicationContext())
    {
        this->DeleteApplication(thisSPtr);
    }
    else
    {
        this->UnprovisionApplicationType(thisSPtr);
    }
}

void DeleteComposeDeploymentContextAsyncOperation::DeleteApplication(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction(); 
    auto error = this->GetApplicationContextForDeleting(storeTx);
    storeTx.CommitReadOnly();
    if (error.IsError(ErrorCodeValue::ApplicationNotFound))
    {
        this->UnprovisionApplicationType(thisSPtr);
    }
    else
    {
        // PendingDefaultServices are persisted in dockerComposeDeploymentContext_, while InnerDeleteApplication() will only check applicationContext_.
        // Deleting application must also delete all pending default services, so PendingDefaultServices should be passed to applicationContext_.
        for (auto iter = dockerComposeDeploymentContext_.PendingDefaultServices.begin(); iter != dockerComposeDeploymentContext_.PendingDefaultServices.end(); ++iter)
        {
            ServiceModelServiceNameEx name(*iter);
            applicationContext_.AddPendingDefaultService(move(name));
        }
        this->InnerDeleteApplication(thisSPtr);
    }
}

void DeleteComposeDeploymentContextAsyncOperation::InnerDeleteApplication(AsyncOperationSPtr const &thisSPtr)
{
    auto operation = AsyncOperation::CreateAndStart<InnerDeleteApplicationContextAsyncOperation>(
        *this,
        const_cast<RolloutManager&>(this->Rollout),
        applicationContext_,
        TimeSpan::FromMinutes(2), // TODO: THis should be a configurable timeout within max val. 
        [this](AsyncOperationSPtr const & operation) { this->OnInnerDeleteApplicationComplete(operation, false); },
        thisSPtr);

    this->OnInnerDeleteApplicationComplete(operation, true);
}


void DeleteComposeDeploymentContextAsyncOperation::OnInnerDeleteApplicationComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = InnerDeleteApplicationContextAsyncOperation::End(operation);
    auto thisSPtr = operation->Parent;
    if (!error.IsSuccess())
    {
        this->WriteInfo(
            TraceComponent,
            "{0} Delete application for {1} failed with {2}",
            this->TraceId,
            dockerComposeDeploymentContext_.ApplicationName,
            error);

        this->RefreshContext();
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->DeleteApplication(thisSPtr); });
        return;
    }

    this->UnprovisionApplicationType(thisSPtr);
}

void DeleteComposeDeploymentContextAsyncOperation::UnprovisionApplicationType(AsyncOperationSPtr const &thisSPtr)
{
    //
    // If apptype context is found, start the unprovision, 
    // If not, delete the compose application context and commit.
    //
    auto storeTx = this->CreateTransaction();
    auto error = this->GetApplicationTypeContextForDeleting(storeTx, applicationTypeContext_);
    if (error.IsError(ErrorCodeValue::ApplicationTypeNotFound))
    {
        this->FinishDelete(thisSPtr, storeTx);
    }
    else
    {
        storeTx.CommitReadOnly();
        this->InnerUnprovisionApplicationType(thisSPtr);
    }
}

void DeleteComposeDeploymentContextAsyncOperation::InnerUnprovisionApplicationType(AsyncOperationSPtr const &thisSPtr)
{
    auto operation = AsyncOperation::CreateAndStart<InnerDeleteApplicationTypeContextAsyncOperation>(
        *this,
        const_cast<RolloutManager&>(this->Rollout),
        applicationTypeContext_,
        TimeSpan::MaxValue,
        [this](AsyncOperationSPtr const & operation) { this->OnInnerUnprovisionComplete(operation, false); },
        thisSPtr);

    this->OnInnerUnprovisionComplete(operation, true);
}

void DeleteComposeDeploymentContextAsyncOperation::OnInnerUnprovisionComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = DeleteApplicationTypeContextAsyncOperation::End(operation);
    auto thisSPtr = operation->Parent;

    if (!error.IsSuccess())
    {
        this->WriteInfo(
            TraceComponent,
            "{0} Unprovision application type for {1} failed with {2}",
            this->TraceId,
            dockerComposeDeploymentContext_.ApplicationName,
            error);

        this->RefreshContext();
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->UnprovisionApplicationType(thisSPtr); });
        return;
    }

    if (dockerComposeDeploymentContext_.IsFailed)
    {
        this->WriteInfo(
            TraceComponent,
            "{0} cleanup of compose application {1} completed.",
            this->TraceId,
            dockerComposeDeploymentContext_.ApplicationName);
    }
    else
    {
        this->WriteInfo(
            TraceComponent,
            "{0} Delete compose application {1} completed.",
            this->TraceId,
            dockerComposeDeploymentContext_.ApplicationName);
    }

    this->TryComplete(thisSPtr, error);
}

ErrorCode DeleteComposeDeploymentContextAsyncOperation::UpdateContextStatusAfterDeleteApplication(StoreTransaction &storeTx)
{
    ErrorCode error(ErrorCodeValue::Success);

    if (!dockerComposeDeploymentContext_.IsFailed)
    {
        error = dockerComposeDeploymentContext_.UpdateComposeDeploymentStatus(
            storeTx,
            ComposeDeploymentStatus::Unprovisioning);

        if (!error.IsSuccess())
        {
            this->WriteInfo(
                TraceComponent,
                "{0} updating docker compose application status for {1} to unprovisioning failed with {2}",
                this->TraceId,
                dockerComposeDeploymentContext_.ApplicationName,
                error);
        }
    }

    return error;
}

ErrorCode DeleteComposeDeploymentContextAsyncOperation::UpdateContextStatusAfterUnprovision(StoreTransaction &storeTx)
{
    ErrorCode error(ErrorCodeValue::Success);

    // Keep the context around if this is a rollback of a failed create.
    if (dockerComposeDeploymentContext_.IsFailed)
    {
        error = dockerComposeDeploymentContext_.UpdateComposeDeploymentStatus(
            storeTx, 
            ComposeDeploymentStatus::Failed);

        if (!error.IsSuccess())
        {
            this->WriteInfo(
                TraceComponent,
                "{0} commit for cleaning up docker compose application {1} failed with {2}",
                this->TraceId,
                dockerComposeDeploymentContext_.ApplicationName,
                error);

            return error;
        }
    }
    else
    {
        vector<StoreDataComposeDeploymentInstance> composeDeploymentInstances;
        StoreDataComposeDeploymentInstance storeData(
            dockerComposeDeploymentContext_.DeploymentName, 
            dockerComposeDeploymentContext_.TypeVersion);

        error = storeTx.ReadPrefix(Constants::StoreType_ComposeDeploymentInstance, storeData.GetDeploymentNameKeyPrefix(), composeDeploymentInstances);
        if (!error.IsSuccess())
        {
            this->WriteInfo(
                TraceComponent,
                "{0} Unable to read compose deployment instances to delete for application {1} error {2}",
                this->TraceId,
                dockerComposeDeploymentContext_.ApplicationName,
                error);

            return error;
        }

        for (auto it = composeDeploymentInstances.begin(); it != composeDeploymentInstances.end(); ++it)
        {
            error =  storeTx.DeleteIfFound(*it);
            if (!error.IsSuccess())
            {
                this->WriteInfo(
                    TraceComponent,
                    "{0} Unable to delete compose deployment instances to delete for application {1} error {2}",
                    this->TraceId,
                    dockerComposeDeploymentContext_.ApplicationName,
                    error);

                return error;
            }
        }

        unique_ptr<ComposeDeploymentUpgradeContext> composeUpgradeContext = make_unique<ComposeDeploymentUpgradeContext>(
            dockerComposeDeploymentContext_.DeploymentName);

        error = storeTx.DeleteIfFound(*composeUpgradeContext);
        if (!error.IsSuccess())
        {
            this->WriteInfo(
                TraceComponent,
                "{0} Unable to delete compose deployment upgrade context during delete for application {1} error {2}",
                this->TraceId,
                dockerComposeDeploymentContext_.ApplicationName,
                error);

            return error;
        }
        
        error = storeTx.DeleteIfFound(dockerComposeDeploymentContext_);
        if (!error.IsSuccess())
        {
            this->WriteInfo(
                TraceComponent,
                "{0} commit for deleting docker compose application {1} failed with {2}",
                this->TraceId,
                dockerComposeDeploymentContext_.ApplicationName,
                error);

            return error;
        }
    }

    return error;
}

void DeleteComposeDeploymentContextAsyncOperation::FinishDelete(AsyncOperationSPtr const &thisSPtr, StoreTransaction &storeTx)
{
    auto error = this->UpdateContextStatusAfterUnprovision(storeTx);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
        return;
    }

    auto operation = StoreTransaction::BeginCommit(
        move(storeTx),
        dockerComposeDeploymentContext_,
        [this](AsyncOperationSPtr const & operation) { this->OnFinishDeleteComplete(operation, false); },
        thisSPtr);
    this->OnFinishDeleteComplete(operation, true);
}

void DeleteComposeDeploymentContextAsyncOperation::OnFinishDeleteComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = StoreTransaction::EndCommit(operation);
    this->TryComplete(operation->Parent, error);
}

ErrorCode DeleteComposeDeploymentContextAsyncOperation::GetApplicationTypeContextForDeleting(
    StoreTransaction const &storeTx, 
    ApplicationTypeContext &appTypeContext) const
{
    ErrorCode error = this->Replica.GetApplicationTypeContext(storeTx, appTypeContext);

    if (!error.IsSuccess())
    {
        return error;
    }

    // Lifecycle management of the application context is controlled via the compose deployment context.
    if (!appTypeContext.IsComplete && !appTypeContext.IsFailed)
    {
        this->WriteWarning(
            TraceComponent,
            "{0} Application type context for {1} {2} is not ready for deleting when ComposeDeployment context is deletable - current state {3}",
            this->TraceId,
            appTypeContext.TypeName,
            appTypeContext.TypeVersion.Value,
            appTypeContext.Status);

        Assert::TestAssert(
            "{0} Application type context for {1} {2} is not ready for deleting when ComposeDeployment context is deletable - current state {3}",
            this->TraceId,
            appTypeContext.TypeName,
            appTypeContext.TypeVersion.Value,
            appTypeContext.Status);
    }

    // Since we are not using rollout manager for rolling out this application type delete, 
    // the rollout status for this need not be set to DeletePending here. TODO: Test/check

    //
    // unprovision is async.
    //
    appTypeContext.UpdateIsAsync(true);
    return ErrorCodeValue::Success;
}

ErrorCode DeleteComposeDeploymentContextAsyncOperation::GetApplicationContextForDeleting(StoreTransaction const &storeTx)
{
    ErrorCode error = storeTx.ReadExact(applicationContext_);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        // If not found, and if we are processing a failed rollout, continue with cleaning up the failed application context. 
        // Context already initialized in the constructor.
        if (dockerComposeDeploymentContext_.IsFailed)
        {
            return ErrorCodeValue::Success;
        }
        else
        {
            this->WriteInfo(
                TraceComponent,
                "{0} application {1} not found",
                this->TraceId,
                applicationContext_.ApplicationName);

            return ErrorCodeValue::ApplicationNotFound;
        }
    }
    else
    {
        // Lifecycle management of the application context is controlled via the compose deployment context.
        if (!applicationContext_.IsComplete && !applicationContext_.IsFailed)
        {
            this->WriteWarning(
                TraceComponent,
                "{0} Application context for {1} is not ready for deleting when ComposeDeployment context is deletable - curent state {2}",
                this->TraceId,
                applicationContext_.ApplicationName,
                applicationContext_.Status);

            Assert::TestAssert(
                "{0} Application context for {1} is not ready for deleting when ComposeDeployment context is deletable - curent state {2}",
                this->TraceId,
                applicationContext_.ApplicationName,
                applicationContext_.Status);
        }
    }

    // Since we are not using rollout manager for rolling out this application delete, 
    // the rollout status for this need not be set to DeletePending here. TODO: Test/check

    return ErrorCodeValue::Success;
}

bool DeleteComposeDeploymentContextAsyncOperation::ShouldDeleteApplicationContext()
{
    if (dockerComposeDeploymentContext_.Status == RolloutStatus::Failed)
    {
        //
        // Provision completed and we failed during create. So rollback the create.
        //
        if (dockerComposeDeploymentContext_.ComposeDeploymentStatus == ComposeDeploymentStatus::Creating)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else // DeletePending
    {
        if (dockerComposeDeploymentContext_.ComposeDeploymentStatus == ComposeDeploymentStatus::Failed)
        {
            //
            // We should already have cleaned up any application context state.
            //
            return false;
        }
        else if (dockerComposeDeploymentContext_.ComposeDeploymentStatus == ComposeDeploymentStatus::Unprovisioning)
        {
            //
            // We have already deleted the application. We can continue to unprovision the type.
            //
            return false;
        }
        else
        {
            return true;
        }
    }
}

void DeleteComposeDeploymentContextAsyncOperation::TryScheduleRetry(
    ErrorCode const & error,
    AsyncOperationSPtr const & thisSPtr,
    RetryCallback const & callback)
{
    if (this->IsRetryable(error))
    {
        this->StartTimer(
            thisSPtr,
            callback,
            this->Replica.GetRandomizedOperationRetryDelay(error));
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

bool DeleteComposeDeploymentContextAsyncOperation::IsRetryable(ErrorCode const & error) const
{
    switch (error.ReadValue())
    {
    case ErrorCodeValue::ApplicationAlreadyExists:
    case ErrorCodeValue::ApplicationTypeAlreadyExists:

        // non-retryable ImageStore errors
    case ErrorCodeValue::ImageBuilderUnexpectedError:
    case ErrorCodeValue::AccessDenied:
    case ErrorCodeValue::InvalidArgument:
    case ErrorCodeValue::InvalidDirectory:
    case ErrorCodeValue::PathTooLong:
        return false;

    default:
        return true;
    }
}

void DeleteComposeDeploymentContextAsyncOperation::StartTimer(
    AsyncOperationSPtr const & thisSPtr,
    RetryCallback const & callback,
    TimeSpan const delay)
{
    AcquireExclusiveLock lock(timerLock_);

    WriteNoise(
        TraceComponent,
        "{0} scheduling retry in {1}",
        this->TraceId,
        delay);
    timerSPtr_ = Timer::Create(TimerTagDefault, [this, thisSPtr, callback](TimerSPtr const & timer)
    {
        timer->Cancel();
        callback(thisSPtr);
    },
        true); //allow concurrency
    timerSPtr_->Change(delay);
}

ErrorCode DeleteComposeDeploymentContextAsyncOperation::RefreshContext()
{
    auto storeTx = this->CreateTransaction();

    auto error = dockerComposeDeploymentContext_.Refresh(storeTx);
    if (!error.IsSuccess())
    {
        this->WriteInfo(
            TraceComponent,
            "{0} Refresh compose application context for {1} failed - {2}",
            this->TraceId,
            dockerComposeDeploymentContext_.ApplicationName,
            error);
    }

    return error;
}
