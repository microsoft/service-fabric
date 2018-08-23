//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace Store;
using namespace ServiceModel;
using namespace Reliability;
using namespace Management::ClusterManager;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("DeleteSingleInstanceDeploymentContextAsyncOperation");

class DeleteSingleInstanceDeploymentContextAsyncOperation::InnerDeleteApplicationTypeContextAsyncOperation 
    : public DeleteApplicationTypeContextAsyncOperation
{
    DENY_COPY(InnerDeleteApplicationTypeContextAsyncOperation);

public:
    InnerDeleteApplicationTypeContextAsyncOperation(
        __in DeleteSingleInstanceDeploymentContextAsyncOperation &owner,
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
            owner_.singleInstanceDeploymentContext_,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
            thisSPtr);
        this->OnCommitComplete(operation, true);
    }

private:
    DeleteSingleInstanceDeploymentContextAsyncOperation &owner_;
};

DeleteSingleInstanceDeploymentContextAsyncOperation::DeleteSingleInstanceDeploymentContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in SingleInstanceDeploymentContext & context,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ProcessRolloutContextAsyncOperationBase(
        rolloutManager,
        context.ReplicaActivityId,
        timeout,
        callback,
        root)
    , singleInstanceDeploymentContext_(context)
    , applicationContext_(
        context.ReplicaActivityId,
        context.ApplicationName,
        context.ApplicationId,
        context.GlobalInstanceCount,
        context.TypeName,
        context.TypeVersion,
        L"",
        map<wstring, wstring>(),
        ApplicationDefinitionKind::Enum::MeshApplicationDescription)
    , applicationTypeContext_(
        context.TypeName,
        context.TypeVersion)
{
}

void DeleteSingleInstanceDeploymentContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->DeleteApplication(thisSPtr);
}

void DeleteSingleInstanceDeploymentContextAsyncOperation::DeleteApplication(AsyncOperationSPtr const & thisSPtr)
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
        // PendingDefaultServices are persisted in singleInstanceDeploymentContext_, while InnerDeleteApplication() will only check applicationContext_.
        // Deleting application must also delete all pending default services, so PendingDefaultServices should be passed to applicationContext_.
        for (auto iter = singleInstanceDeploymentContext_.PendingDefaultServices.begin(); iter != singleInstanceDeploymentContext_.PendingDefaultServices.end(); ++iter)
        {
            ServiceModelServiceNameEx name(*iter);
            applicationContext_.AddPendingDefaultService(move(name));
        }
        this->InnerDeleteApplication(thisSPtr);
    }
}

void DeleteSingleInstanceDeploymentContextAsyncOperation::InnerDeleteApplication(AsyncOperationSPtr const &thisSPtr)
{
    auto operation = AsyncOperation::CreateAndStart<DeleteApplicationContextAsyncOperation>(
        const_cast<RolloutManager&>(this->Rollout),
        applicationContext_,
        TimeSpan::FromMinutes(2), // TODO: This should be a configurable timeout within max val. 
        [this](AsyncOperationSPtr const & operation) { this->OnInnerDeleteApplicationComplete(operation, false); },
        thisSPtr);

    this->OnInnerDeleteApplicationComplete(operation, true);
}

void DeleteSingleInstanceDeploymentContextAsyncOperation::OnInnerDeleteApplicationComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = DeleteApplicationContextAsyncOperation::End(operation);
    auto thisSPtr = operation->Parent;
    if (!error.IsSuccess())
    {
        this->WriteInfo(
            TraceComponent,
            "{0} Delete application for {1} failed with {2}",
            this->TraceId,
            singleInstanceDeploymentContext_.DeploymentName,
            error);

        this->RefreshContext();
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->DeleteApplication(thisSPtr); });
    }
    else
    {
        this->UnprovisionApplicationType(thisSPtr);
    }
}

void DeleteSingleInstanceDeploymentContextAsyncOperation::UnprovisionApplicationType(AsyncOperationSPtr const &thisSPtr)
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

void DeleteSingleInstanceDeploymentContextAsyncOperation::InnerUnprovisionApplicationType(AsyncOperationSPtr const &thisSPtr)
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

void DeleteSingleInstanceDeploymentContextAsyncOperation::OnInnerUnprovisionComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = InnerDeleteApplicationTypeContextAsyncOperation::End(operation);
    auto thisSPtr = operation->Parent;

    if (!error.IsSuccess())
    {
        this->WriteInfo(
            TraceComponent,
            "{0} Unprovision application type for {1} failed with {2}",
            this->TraceId,
            singleInstanceDeploymentContext_.DeploymentName,
            error);

        this->RefreshContext();
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->UnprovisionApplicationType(thisSPtr); });
        return;
    }

    if (singleInstanceDeploymentContext_.IsFailed)
    {
        this->WriteInfo(
            TraceComponent,
            "{0} cleanup of single instance deployment {1} completed.",
            this->TraceId,
            singleInstanceDeploymentContext_.DeploymentName);
        this->TryComplete(thisSPtr, error);
    }
    else
    {
        this->CompleteDeletion(thisSPtr);
    }
}

void DeleteSingleInstanceDeploymentContextAsyncOperation::CompleteDeletion(
    AsyncOperationSPtr const & thisSPtr)
{
    this->WriteInfo(
        TraceComponent,
        "{0} Delete single instance deployment {1} completed.",
        this->TraceId,
        singleInstanceDeploymentContext_.DeploymentName);
 
    this->TryComplete(thisSPtr, ErrorCodeValue::Success);
}

ErrorCode DeleteSingleInstanceDeploymentContextAsyncOperation::UpdateContextStatusAfterUnprovision(StoreTransaction &storeTx)
{
    ErrorCode error(ErrorCodeValue::Success);

    // Keep the context around if this is a rollback of a failed create.
    if (singleInstanceDeploymentContext_.IsFailed)
    {
        error = singleInstanceDeploymentContext_.UpdateDeploymentStatus(
            storeTx, 
            SingleInstanceDeploymentStatus::Enum::Failed);

        if (!error.IsSuccess())
        {
            this->WriteInfo(
                TraceComponent,
                "{0} commit for cleaning up single instance deployment {1} failed with {2}",
                this->TraceId,
                singleInstanceDeploymentContext_.DeploymentName,
                error);

            return error;
        }
        
        // Failed replacing request
        // This should delete only the "new" stuff and leave the "old version" there.
        if (!singleInstanceDeploymentContext_.NewTypeVersion.Value.empty())
        {
            StoreDataSingleInstanceApplicationInstance singleInstanceApplicationInstance(
                singleInstanceDeploymentContext_.DeploymentName,
                singleInstanceDeploymentContext_.NewTypeVersion);

            error = storeTx.ReadExact(singleInstanceApplicationInstance);
            if (!error.IsSuccess())
            {
                this->WriteInfo(
                    TraceComponent,
                    "{0} Unable to read single instance application new deployment instance to delete for {1} error {2}",
                    this->TraceId,
                    singleInstanceDeploymentContext_.DeploymentName,
                    error);
                return error;
            }

            error =  storeTx.DeleteIfFound(singleInstanceApplicationInstance);
            if (!error.IsSuccess())
            {
                this->WriteInfo(
                    TraceComponent,
                    "{0} Unable to delete single instance application instances to delete for application {1} error {2}",
                    this->TraceId,
                    singleInstanceDeploymentContext_.DeploymentName,
                    error);

                return error;
            }
            
            ApplicationTypeContext newTypeContext(
                singleInstanceDeploymentContext_.TypeName,
                singleInstanceDeploymentContext_.TypeVersion);
            
            error = storeTx.DeleteIfFound(newTypeContext);
            if (!error.IsSuccess())
            {
                this->WriteInfo(
                    TraceComponent,
                    "{0} Unable to delete new application type context for {1} {2} error {3}",
                    this->TraceId,
                    singleInstanceDeploymentContext_.TypeName,
                    singleInstanceDeploymentContext_.TypeVersion,
                    error);
                return error;
            }
        }
    }
    else
    {
        vector<StoreDataSingleInstanceApplicationInstance> singleInstanceApplicationInstances;
        StoreDataSingleInstanceApplicationInstance storeData(
            singleInstanceDeploymentContext_.DeploymentName, 
            singleInstanceDeploymentContext_.TypeVersion);

        error = storeTx.ReadPrefix(Constants::StoreType_SingleInstanceApplicationInstance, storeData.GetDeploymentNameKeyPrefix(), singleInstanceApplicationInstances);
        if (!error.IsSuccess())
        {
            this->WriteInfo(
                TraceComponent,
                "{0} Unable to read single instance application instances to delete for {1} error {2}",
                this->TraceId,
                singleInstanceDeploymentContext_.DeploymentName,
                error);

            return error;
        }

        for (auto it = singleInstanceApplicationInstances.begin(); it != singleInstanceApplicationInstances.end(); ++it)
        {
            if (singleInstanceDeploymentContext_.Status == RolloutStatus::Enum::Replacing
                && it->TypeVersion == singleInstanceDeploymentContext_.NewTypeVersion) { continue; }

            error =  storeTx.DeleteIfFound(*it);
            if (!error.IsSuccess())
            {
                this->WriteInfo(
                    TraceComponent,
                    "{0} Unable to delete single instance application instances to delete for application {1} error {2}",
                    this->TraceId,
                    singleInstanceDeploymentContext_.DeploymentName,
                    error);

                return error;
            }
        }

        if (singleInstanceDeploymentContext_.Status != RolloutStatus::Enum::Replacing)
        {
            error = storeTx.DeleteIfFound(singleInstanceDeploymentContext_);
            if (!error.IsSuccess())
            {
                this->WriteInfo(
                    TraceComponent,
                    "{0} commit for deleting single instance deployment context {1} failed with {2}",
                    this->TraceId,
                    singleInstanceDeploymentContext_.DeploymentName,
                    error);

                return error;
            }
        }
    }

    return error;
}

void DeleteSingleInstanceDeploymentContextAsyncOperation::FinishDelete(AsyncOperationSPtr const &thisSPtr, StoreTransaction &storeTx)
{
    auto error = this->UpdateContextStatusAfterUnprovision(storeTx);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
        return;
    }

    auto operation = StoreTransaction::BeginCommit(
        move(storeTx),
        singleInstanceDeploymentContext_,
        [this](AsyncOperationSPtr const & operation) { this->OnFinishDeleteComplete(operation, false); },
        thisSPtr);
    this->OnFinishDeleteComplete(operation, true);
}

void DeleteSingleInstanceDeploymentContextAsyncOperation::OnFinishDeleteComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = StoreTransaction::EndCommit(operation);
    this->TryComplete(operation->Parent, error);
}

ErrorCode DeleteSingleInstanceDeploymentContextAsyncOperation::GetApplicationTypeContextForDeleting(
    StoreTransaction const &storeTx, 
    ApplicationTypeContext &appTypeContext) const
{
    ErrorCode error = this->Replica.GetApplicationTypeContext(storeTx, appTypeContext);

    if (!error.IsSuccess())
    {
        return error;
    }

    // Lifecycle management of the application context is controlled via the container application context.
    if (!appTypeContext.IsComplete && !appTypeContext.IsFailed)
    {
        this->WriteWarning(
            TraceComponent,
            "{0} Application type context for {1} {2} is not ready for deleting when SingleInstanceDeployment context is deletable - current state {3}",
            this->TraceId,
            appTypeContext.TypeName,
            appTypeContext.TypeVersion.Value,
            appTypeContext.Status);

        Assert::TestAssert(
            "{0} Application type context for {1} {2} is not ready for deleting when SingleInstanceDeployment context is deletable - current state {3}",
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

ErrorCode DeleteSingleInstanceDeploymentContextAsyncOperation::GetApplicationContextForDeleting(StoreTransaction const &storeTx)
{
    ErrorCode error = storeTx.ReadExact(applicationContext_);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        // If not found, and if we are processing a failed rollout, continue with cleaning up the failed application context. 
        // Context already initialized in the constructor.
        if (singleInstanceDeploymentContext_.IsFailed)
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
        // Lifecycle management of the application context is controlled via the container application context.
        if (!applicationContext_.IsComplete && !applicationContext_.IsFailed)
        {
            this->WriteWarning(
                TraceComponent,
                "{0} Application context for {1} is not ready for deleting when SingleInstanceDeployment context is deletable - curent state {2}",
                this->TraceId,
                applicationContext_.ApplicationName,
                applicationContext_.Status);

            Assert::TestAssert(
                "{0} Application context for {1} is not ready for deleting when SingleInstanceDeployment context is deletable - curent state {2}",
                this->TraceId,
                applicationContext_.ApplicationName,
                applicationContext_.Status);
        }
    }

    // Since we are not using rollout manager for rolling out this application delete, 
    // the rollout status for this need not be set to DeletePending here. TODO: Test/check

    return ErrorCodeValue::Success;
}

void DeleteSingleInstanceDeploymentContextAsyncOperation::TryScheduleRetry(
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

bool DeleteSingleInstanceDeploymentContextAsyncOperation::IsRetryable(ErrorCode const & error) const
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

void DeleteSingleInstanceDeploymentContextAsyncOperation::StartTimer(
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

ErrorCode DeleteSingleInstanceDeploymentContextAsyncOperation::RefreshContext()
{
    auto storeTx = this->CreateTransaction();

    auto error = singleInstanceDeploymentContext_.Refresh(storeTx);
    if (!error.IsSuccess())
    {
        this->WriteInfo(
            TraceComponent,
            "{0} Refresh compose application context for {1} failed - {2}",
            this->TraceId,
            singleInstanceDeploymentContext_.ApplicationName,
            error);
    }

    return error;
}
