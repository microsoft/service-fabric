//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::ClusterManager;
using namespace ServiceModel;
using namespace Store;

static StringLiteral const TraceComponent("SingleInstanceDeploymentUpgradeContextAsyncOperation");
static StringLiteral const ApplicationUpgradeTraceComponent("SingleInstanceDeploymentUpgradeContextAsyncOperation.Upgrade");

class ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::InnerApplicationUpgradeAsyncOperation
    : public ProcessApplicationUpgradeContextAsyncOperation
{
private:
    class InnerInitilizeUpgradeAsyncOperation;

public:
    InnerApplicationUpgradeAsyncOperation(
        __in RolloutManager & rolloutManager,
        __in ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ProcessApplicationUpgradeContextAsyncOperation(
            rolloutManager,
            owner.applicationUpgradeContext_,
            "SingleInstanceDeploymentUpgradeContextAsyncOperation.Upgrade",
            callback,
            root)
        , targetStoreData_()
        , owner_(owner)
    {
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        auto error = this->LoadTargetStoreData();
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }
 
        __super::OnStart(thisSPtr);
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<InnerApplicationUpgradeAsyncOperation>(operation)->Error;
    }

protected:
    bool Get_SkipRollbackUpdateDefaultService() override { return false; }
    bool Get_EnableDefaultServicesUpgrade() override { return true; }

    ErrorCode OnRefreshUpgradeContext(StoreTransaction const & storeTx) override
    {
        return owner_.context_.Refresh(storeTx);
    }

private:
    // Image builder generator is called during application upgrade initialize and are called from InitializeUpgradeAsyncOperation as override method.
    AsyncOperationSPtr BeginLoadApplicationDescriptions(
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) override
    {
        WriteNoise(
            ApplicationUpgradeTraceComponent,
            "{0} loading application descriptions",
            this->TraceId);

        switch (owner_.context_.DeploymentType)
        {
        case DeploymentType::Enum::V2Application:
            return this->BeginBuildSingleInstanceApplicationForUpgrade(callback, parent);
        default:
            Assert::CodingError("Unknown single instance deployment upgrade type {0}", static_cast<int>(owner_.applicationContext_.ApplicationDefinitionKind));
        }
    }
    
    ErrorCode LoadTargetStoreData()
    {
        auto storeTx = this->CreateTransaction();

        switch (owner_.context_.DeploymentType)
        {
        case DeploymentType::Enum::V2Application:
            targetStoreData_ = make_shared<StoreDataSingleInstanceApplicationInstance>(
                owner_.context_.DeploymentName,
                owner_.context_.TargetTypeVersion);
            break;
        default:
            Assert::CodingError("Unknown single instance deployment upgrade type {0}", static_cast<int>(owner_.applicationContext_.ApplicationDefinitionKind));
        }

        return storeTx.ReadExact(*targetStoreData_);
    }

    AsyncOperationSPtr BeginInitializeUpgrade(
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) override
    {
        return AsyncOperation::CreateAndStart<InnerInitilizeUpgradeAsyncOperation>(*this, callback, parent);
    }

    AsyncOperationSPtr BeginBuildSingleInstanceApplicationForUpgrade(
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto storeDataSingleInstance = dynamic_pointer_cast<StoreDataSingleInstanceApplicationInstance>(targetStoreData_);
        TESTASSERT_IF(
            storeDataSingleInstance == nullptr,
            "storeData is not for single instance deployment");

        auto storeTx = this->CreateTransaction();
        // Look up volume refs
        for (auto & service : storeDataSingleInstance->ApplicationDescription.Services)
        {
            for (auto & codePackage : service.Properties.CodePackages)
            {
                for (auto & containerVolumeRef : codePackage.VolumeRefs)
                {
                    wstring const & volumeName = containerVolumeRef.Name;
                    auto storeDataVolume = StoreDataVolume(volumeName);
                    ErrorCode error = storeTx.ReadExact(storeDataVolume);

                    //TODO: Return AsyncOp and complete itself with error
                    if (error.IsError(ErrorCodeValue::NotFound))
                    {
                        WriteError(
                            ApplicationUpgradeTraceComponent,
                            "{0} Volume {1} does not exist",
                            this->TraceId,
                            volumeName);
                    }
                    else if (!error.IsSuccess())
                    {
                        WriteWarning(
                            ApplicationUpgradeTraceComponent,
                            "{0} failed to read StoreDataVolume {1}: ({2} {3})",
                            this->TraceId,
                            volumeName,
                            error,
                            error.Message);
                    }

                    if (error.IsSuccess())
                    {
                        containerVolumeRef.VolumeDescriptionSPtr = storeDataVolume.VolumeDescription;
                    }
                    else
                    {
                        containerVolumeRef.VolumeDescriptionSPtr = nullptr;
                    }
                }

            }
        }
        storeTx.CommitReadOnly();

        return this->ImageBuilder.BeginBuildSingleInstanceApplicationForUpgrade(
            this->ActivityId,
            owner_.context_.TypeName,
            owner_.context_.CurrentTypeVersion,
            owner_.context_.TargetTypeVersion,
            storeDataSingleInstance->ApplicationDescription,
            owner_.context_.ApplicationName,
            owner_.applicationContext_.ApplicationId,
            owner_.applicationContext_.ApplicationVersion,
            TimeSpan::FromMinutes(2), // TODO: Should be a configurable time, so we can retry incase IB is stuck.
            callback,
            parent);
    }

    ErrorCode EndLoadApplicationDescriptions(
        AsyncOperationSPtr const & operation,
        __out vector<ServiceModelServiceManifestDescription> & serviceManifest,
        __out wstring &applicationManifest,
        __out ApplicationHealthPolicy &applicationHealthPolicy,
        __out map<wstring, wstring> &defaultParamList)
    {
        ErrorCode error;
        if (owner_.context_.DeploymentType == DeploymentType::Enum::V2Application)
        {
            error = this->ImageBuilder.EndBuildSingleInstanceApplicationForUpgrade(
                operation,
                serviceManifest,
                applicationManifest,
                applicationHealthPolicy,
                defaultParamList,
                currentApplication_,
                targetApplication_);
        }
        else
        {
            Assert::CodingError("Unknown deployment type");
        }

        if (error.IsSuccess())
        {
            this->ResetImageStoreErrorCount();

            removedTypes_ = currentApplication_.ComputeRemovedServiceTypes(targetApplication_);

            // This must happen before any validations since any added packages must be cleaned up
            // on validation failure
            //
            addedPackages_ = currentApplication_.ComputeAddedServicePackages(targetApplication_);

            wstring errorDetails;
            if (!currentApplication_.TryValidate(errorDetails) || !targetApplication_.TryValidate(errorDetails))
            {
                error = this->TraceAndGetErrorDetails(ErrorCodeValue::ImageBuilderValidationError, move(errorDetails));
            }
        }
        else
        {
            WriteInfo(
                ApplicationUpgradeTraceComponent, 
                "{0} failed UpgradeApplication() call to ImageBuilder for buildApplicationForUpgrade '{1}' due to {2}",
                this->TraceId,
                this->UpgradeContext.UpgradeDescription.ApplicationName,
                error);
        }
        
        return error;
    }

    ErrorCode OnFinishUpgrade(StoreTransaction const & storeTx) override
    {
        auto singleInstanceDeploymentContext = make_unique<SingleInstanceDeploymentContext>(owner_.context_.DeploymentName);
        auto error = this->Replica.GetSingleInstanceDeploymentContext(storeTx, *singleInstanceDeploymentContext);
        if (!error.IsSuccess())
        {
            return error;
        }

        singleInstanceDeploymentContext->TypeVersion = owner_.context_.TargetTypeVersion;
        error = storeTx.Update(*singleInstanceDeploymentContext);
        if (!error.IsSuccess())
        {
            return error;
        }

        owner_.context_.VersionToUnprovision = owner_.context_.CurrentTypeVersion;
        owner_.context_.ResetUpgradeDescription();

        return owner_.context_.StartUnprovision(
            storeTx,
            wformatString(GET_RC(Deployment_Upgraded_To_Version), owner_.context_.TargetTypeVersion));
    }

    ErrorCode OnFinishRollback(StoreTransaction const & storeTx) override
    {
        auto singleInstanceDeploymentContext = make_unique<SingleInstanceDeploymentContext>(owner_.context_.DeploymentName);
        auto error = this->Replica.GetSingleInstanceDeploymentContext(storeTx, *singleInstanceDeploymentContext);
        if (!error.IsSuccess())
        {
            return error;
        }

        singleInstanceDeploymentContext->TypeVersion = owner_.context_.CurrentTypeVersion;
        error = storeTx.Update(*singleInstanceDeploymentContext);
        if (!error.IsSuccess())
        {
            return error;
        }

        owner_.context_.VersionToUnprovision = owner_.context_.TargetTypeVersion;
        owner_.context_.ResetUpgradeDescription();

        return owner_.context_.StartUnprovisionTarget(
            storeTx,
            wformatString(GET_RC(Deployment_Rolledback_To_Version), owner_.context_.CurrentTypeVersion));
    }

    ErrorCode OnStartRollback(StoreTransaction const & storeTx) override
    {
        WriteInfo(
            ApplicationUpgradeTraceComponent,
            "{0} Single Instance deployment {1} rollback initiated during application upgrade",
            TraceId,
            owner_.context_.DeploymentName);

        return owner_.context_.StartRollback(
            storeTx,
            wformatString(GET_RC(Deployment_Rollingback_To_Version), owner_.context_.TargetTypeVersion, owner_.context_.CurrentTypeVersion));
    }

    shared_ptr<StoreData> targetStoreData_;
    ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation & owner_;
};

class ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::InnerApplicationUpgradeAsyncOperation::InnerInitilizeUpgradeAsyncOperation
    : public ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation
{
public:
    InnerInitilizeUpgradeAsyncOperation(
        __in ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::InnerApplicationUpgradeAsyncOperation & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    : ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation(owner, callback, parent)
    , owner_(owner)
    {
    }

private:
    void OnLoadApplicationDescriptionsComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously) override
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;
        auto & rootAsyncOperation = owner_.owner_;
        auto storeTx = owner_.CreateTransaction();

        vector<ServiceModelServiceManifestDescription> serviceManifest;
        wstring applicationManifest;
        ApplicationHealthPolicy applicationHealthPolicy;
        map<wstring, wstring> defaultParamList;
        auto error = owner_.EndLoadApplicationDescriptions(
            operation,
            serviceManifest,
            applicationManifest,
            applicationHealthPolicy,
            defaultParamList);

        if (error.IsSuccess())
        {
            auto storeDataApplicationManifest = make_shared<StoreDataApplicationManifest>(
                rootAsyncOperation.context_.TypeName,
                rootAsyncOperation.context_.TargetTypeVersion,
                move(applicationManifest),
                move(applicationHealthPolicy),
                move(defaultParamList));

            auto storeDataServiceManifest = make_shared<StoreDataServiceManifest>(
                rootAsyncOperation.context_.TypeName,
                rootAsyncOperation.context_.TargetTypeVersion,
                move(serviceManifest));

            rootAsyncOperation.targetApplicationTypeContext_.ManifestDataInStore = true;
            rootAsyncOperation.targetApplicationTypeContext_.QueryStatusDetails = L"";

            error = rootAsyncOperation.targetApplicationTypeContext_.Complete(storeTx);
            if (error.IsSuccess())
            {
                error = storeTx.UpdateOrInsertIfNotFound(*storeDataApplicationManifest);
            }

            if (error.IsSuccess())
            {
                error = storeTx.UpdateOrInsertIfNotFound(*storeDataServiceManifest);
            }

            if (error.IsSuccess())
            {
                if (rootAsyncOperation.context_.Interrupted)
                {
                    rootAsyncOperation.context_.VersionToUnprovision = rootAsyncOperation.context_.TargetTypeVersion;

                    error = rootAsyncOperation.context_.StartUnprovisionTarget(
                        storeTx,
                        wformatString(
                            GET_RC(Deployment_Rolledback_To_Version),
                            rootAsyncOperation.context_.CurrentTypeVersion));
                }
                else
                {
                    error = rootAsyncOperation.context_.StartUpgrade(
                        storeTx,
                        wformatString(
                            GET_RC(Deployment_Upgrading_To_Version),
                            rootAsyncOperation.context_.CurrentTypeVersion,
                            rootAsyncOperation.context_.TargetTypeVersion));
                }
            }
        }

        if (error.IsSuccess())
        {
            auto innerOperation = StoreTransaction::BeginCommit(
                move(storeTx),
                rootAsyncOperation.context_, 
                [this](AsyncOperationSPtr const & operation) { this->OnCommitBuildComplete(operation, false); },
                thisSPtr);
            this->OnCommitBuildComplete(innerOperation, true);
        }
        //Let ProcessUpgradeContextAsyncOperationBase schedule retry.
        else
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }
    }

    void OnCommitBuildComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        ErrorCode error = StoreTransaction::EndCommit(operation);

        if (error.IsSuccess()) 
        {
            this->ValidateServices(thisSPtr);
        }
        //Let ProcessUpgradeContextAsyncOperationBase schedule retry.
        else
        {
            this->TryComplete(thisSPtr, move(error)); 
        }
    }

    InnerApplicationUpgradeAsyncOperation & owner_;
};

class ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::InnerUnprovisionAsyncOperation
    : public DeleteApplicationTypeContextAsyncOperation
{
public:
    InnerUnprovisionAsyncOperation(
        __in ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation & owner,
        __in RolloutManager & rolloutManager,
        __in ApplicationTypeContext & applicationTypeContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parents)
        : DeleteApplicationTypeContextAsyncOperation(
            rolloutManager,
            applicationTypeContext,
            timeout,
            callback,
            parents)
        , owner_(owner)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<InnerUnprovisionAsyncOperation>(operation)->Error;
    }
    
protected:
    void CommitDeleteApplicationType(AsyncOperationSPtr const & thisSPtr, StoreTransaction &storeTx) override
    {
        auto error = owner_.UpdateContextStatusAfterUnprovision(storeTx);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        auto operation = StoreTransaction::BeginCommit(
            move(storeTx),
            owner_.context_,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
            thisSPtr);
        this->OnCommitComplete(operation, true);
    }

private:
    ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation &owner_;
};

// The single instance rolling forward consists the following phases:
// 1. Inner deployment upgrade
//    a. Build application package
//    b. Persist store data and complete provision
//    c. Application upgrade
// 2. Unprovision current application type
ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in SingleInstanceDeploymentUpgradeContext & context,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ProcessRolloutContextAsyncOperationBase(
        rolloutManager,
        context.ReplicaActivityId,
        TimeSpan::MaxValue,
        callback,
        root)
    , context_(context)
    , applicationContext_(context.ApplicationName)
    , applicationUpgradeContext_(context.ApplicationName)
    , targetApplicationTypeContext_(context.TypeName, context.TargetTypeVersion)
    , applicationTypeContextToUnprovision_()
    , timerSPtr_()
    , timerLock_()
{
}

void ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();
    ErrorCode error = this->Replica.GetApplicationContext(storeTx, applicationContext_);
    
    if (error.IsSuccess())
    {
        error = this->Replica.GetApplicationTypeContext(storeTx, targetApplicationTypeContext_);
    }

    if (error.IsSuccess())
    {
        error = storeTx.ReadExact(applicationUpgradeContext_);
    }

    storeTx.CommitReadOnly();

    if (error.IsSuccess())
    {
        this->StartApplicationUpgrade(thisSPtr);
    }
    else
    {
        this->RefreshContext();
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->OnStart(thisSPtr); });
        return;
    }
}

ErrorCode ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation>(operation)->Error;
}

void ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::StartApplicationUpgrade(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = AsyncOperation::CreateAndStart<InnerApplicationUpgradeAsyncOperation>(
        const_cast<RolloutManager &>(this->Rollout),
        *this,
        [this](AsyncOperationSPtr const & operation) { this->OnUpgradeApplicationComplete(operation, false); },
        thisSPtr);
    this->OnUpgradeApplicationComplete(operation, true);
}

void ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::OnUpgradeApplicationComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
    
    ErrorCode error = InnerApplicationUpgradeAsyncOperation::End(operation);
    auto thisSPtr = operation->Parent;

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} Application upgrade for deployment {1} failed due to ({2} {3})",
            TraceId,
            context_.DeploymentName,
            error,
            error.Message);

        this->TryComplete(thisSPtr, move(error));
        return;
    }

    this->UnprovisionApplicationType(thisSPtr);
}

void ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::UnprovisionApplicationType(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();
    ErrorCode error = context_.Refresh(storeTx);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to refresh context. Error: {1}",
            this->TraceId,
            error);
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->UnprovisionApplicationType(thisSPtr); });
    }

    applicationTypeContextToUnprovision_ = make_unique<ApplicationTypeContext>(
        context_.TypeName,
        context_.VersionToUnprovision);

    error = this->Replica.GetApplicationTypeContext(storeTx, *applicationTypeContextToUnprovision_);
    if (error.IsError(ErrorCodeValue::ApplicationTypeNotFound))
    {
        this->FinishUpgrade(thisSPtr, storeTx);
    }
    else if (!error.IsSuccess())
    {
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->UnprovisionApplicationType(thisSPtr); });
        return;
    }
    else if (!applicationTypeContextToUnprovision_->IsComplete)
    {
        WriteWarning(
            TraceComponent,
            "{0} Application type context for {1} is not ready for unprovisioning, current state is {2}",
            this->TraceId,
            context_.DeploymentName,
            applicationTypeContextToUnprovision_->Status);

        Assert::TestAssert(
            TraceComponent,
            "{0} Application type context for {1} is not ready for unprovisioning, current state is {2}",
            this->TraceId,
            context_.DeploymentName,
            applicationTypeContextToUnprovision_->Status);
    }

    storeTx.CommitReadOnly();
    applicationTypeContextToUnprovision_->UpdateIsAsync(true);

    WriteInfo(
        TraceComponent,
        "{0} Deployment {1} starting to unprovision version: {2}",
        TraceId,
        context_.DeploymentName,
        applicationTypeContextToUnprovision_->TypeVersion);

    auto operation = AsyncOperation::CreateAndStart<InnerUnprovisionAsyncOperation>(
        *this,
        const_cast<RolloutManager &>(this->Rollout),
        *applicationTypeContextToUnprovision_,
        TimeSpan::MaxValue,
        [this](AsyncOperationSPtr const & operation) { this->OnUnprovisionApplicationTypeComplete(operation, false); },
        thisSPtr);
    this->OnUnprovisionApplicationTypeComplete(operation, true);
}

void ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::OnUnprovisionApplicationTypeComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
    
    auto error = InnerUnprovisionAsyncOperation::End(operation);
    auto thisSPtr = operation->Parent;

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} InnerUnprovision application type for {1} failed with {2}",
            this->TraceId,
            context_.DeploymentName,
            error);
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} single instance deployment upgrade for {1} completed successfully.",
            this->TraceId,
            context_.DeploymentName);
    }

    this->TryComplete(thisSPtr, move(error));
}

void ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::FinishUpgrade(AsyncOperationSPtr const & thisSPtr, StoreTransaction & storeTx)
{
    auto error = this->UpdateContextStatusAfterUnprovision(storeTx);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
        return;
    }

    auto operation = StoreTransaction::BeginCommit(
        move(storeTx),
        context_,
        [this](AsyncOperationSPtr const & operation) { this->OnFinishUpgradeComplete(operation, false); },
        thisSPtr);
    this->OnFinishUpgradeComplete(operation, true);
}

void ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::OnFinishUpgradeComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = StoreTransaction::EndCommit(operation);
    this->TryComplete(operation->Parent, move(error));
}

ErrorCode ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::UpdateContextStatusAfterUnprovision(StoreTransaction &storeTx)
{
    auto error = context_.FinishUpgrading(storeTx);
    if (!error.IsSuccess())
    {
        return error;
    }

    unique_ptr<StoreData> storeData;

    switch (applicationContext_.ApplicationDefinitionKind)
    {
    case ApplicationDefinitionKind::Enum::MeshApplicationDescription:
        storeData = make_unique<StoreDataSingleInstanceApplicationInstance>(
            context_.DeploymentName,
            context_.VersionToUnprovision);
        break;
    default:
        storeData = nullptr;
    }

    if (storeData)
    {
        error = storeTx.DeleteIfFound(*storeData);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    auto singleInstanceDeploymentContext = make_unique<SingleInstanceDeploymentContext>(context_.DeploymentName);
    error = this->Replica.GetSingleInstanceDeploymentContext(storeTx, *singleInstanceDeploymentContext);
    if (!error.IsSuccess())
    {
        return error;
    }

    return singleInstanceDeploymentContext->FinishCreating(storeTx, L"");
}

void ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::TryScheduleRetry(
    ErrorCode const & error,
    AsyncOperationSPtr const & thisSPtr,
    RetryCallback const & callback)
{
    this->RefreshContext();
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

bool ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::IsRetryable(ErrorCode const & error) const
{
    switch (error.ReadValue())
    {
        case ErrorCodeValue::ApplicationAlreadyExists:
        case ErrorCodeValue::ApplicationTypeAlreadyExists:
        case ErrorCodeValue::SingleInstanceApplicationAlreadyExists:
        case ErrorCodeValue::DnsServiceNotFound:
        case ErrorCodeValue::InvalidDnsName:
        case ErrorCodeValue::DnsNameInUse:

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

void ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::StartTimer(
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

ErrorCode ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::RefreshContext()
{
    auto storeTx = this->CreateTransaction();

    return context_.Refresh(storeTx);
}

ErrorCode ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation::RefreshApplicationUpgradeContext()
{
    auto storeTx = this->CreateTransaction();

    auto error = applicationUpgradeContext_.Refresh(storeTx);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} unable to refresh Application upgrade context due to {1}",
            this->TraceId,
            error);
    }

    storeTx.CommitReadOnly();

    return error;
}
