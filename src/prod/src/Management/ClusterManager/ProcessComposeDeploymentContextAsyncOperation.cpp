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
using namespace Reliability;

static StringLiteral const DockerComposeTraceComponent("CreateComposeDeployment.CreateApplication");
static StringLiteral const ProvisionTraceComponent("CreateComposeDeployment.Provision");
static StringLiteral const CreateComposeDeploymentTraceComponent("CreateComposeDeploymentContextAsyncOperation");

//
// This is part of docker compose application creation. 
// The rollout of ComposeDeploymentContext tracks the overall context. There is no actual 
// "ComposeDeploymentTypeContext". Docker compose application reuses ApplicationTypeContext.
// ApplicationTypeContext is only created and persisted at the completion of this async operation.
//
class ProcessComposeDeploymentContextAsyncOperation::ProcessComposeDeploymentProvisionAsyncOperation
    : public ProcessProvisionContextAsyncOperationBase
{
public:
    ProcessComposeDeploymentProvisionAsyncOperation(
        __in RolloutManager & rolloutManager,
        __in ProcessComposeDeploymentContextAsyncOperation & owner,
        __in ApplicationTypeContext & context,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ProcessProvisionContextAsyncOperationBase(
            rolloutManager,
            context,
            DockerComposeTraceComponent,
            timeout,
            callback,
            root)
        , owner_(owner)
        , mergedComposeFile_()
        , composeDeploymentInstanceData_(owner_.Context.DeploymentName, owner_.Context.TypeVersion)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<ProcessComposeDeploymentProvisionAsyncOperation>(operation)->Error;
    }

    __declspec(property(get=get_ComposeDeploymentContext)) ComposeDeploymentContext & ComposeDeploymentContext;
    Management::ClusterManager::ComposeDeploymentContext & get_ComposeDeploymentContext() { return owner_.Context; }

    void OnCompleted() override
    {
        if (startedProvision_)
        {
            this->Replica.AppTypeRequestTracker.FinishRequest(this->AppTypeContext.TypeName, this->AppTypeContext.TypeVersion);
        }

        __super::OnCompleted();
    }

private:
    void StartProvisioning(AsyncOperationSPtr const & thisSPtr)
    {
        provisioningError_ = this->CheckApplicationTypes_IgnoreCase();

        if (provisioningError_.IsSuccess())
        {
            provisioningError_ = owner_.GetStoreDataComposeDeploymentInstance(composeDeploymentInstanceData_);
        }

        if (provisioningError_.IsSuccess())
        {
            WriteInfo(
                DockerComposeTraceComponent,
                "{0} provisioning: type={1} version={2}",
                this->TraceId,
                this->AppTypeContext.TypeName,
                this->AppTypeContext.TypeVersion);

            //
            // This is happening as a part of compose deployment creation, which is async. So
            // the imagebuilder timeout is maxval.
            //
            auto operation = this->ImageBuilder.BeginBuildComposeDeploymentType(
                this->ActivityId,
                composeDeploymentInstanceData_.ComposeFiles[0],
                ByteBufferSPtr(),
                composeDeploymentInstanceData_.RepositoryUserName,
                composeDeploymentInstanceData_.RepositoryPassword,
                composeDeploymentInstanceData_.IsPasswordEncrypted,
                owner_.Context.ApplicationName,
                owner_.Context.TypeName,
                owner_.Context.TypeVersion,
                TimeSpan::MaxValue,
                [this](AsyncOperationSPtr const & operation) { this->OnBuildComposeDeploymentTypeComplete(operation, false); },
                thisSPtr);

            this->OnBuildComposeDeploymentTypeComplete(operation, true);
        }
        else
        {
            this->FinishBuildComposeDeploymentType(thisSPtr);
        }
    }

    void OnBuildComposeDeploymentTypeComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (this->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        vector<ServiceModelServiceManifestDescription> serviceManifest;
        wstring applicationManifestId;
        wstring applicationManifest;
        ApplicationHealthPolicy applicationHealthPolicy;
        wstring mergedComposeFile;
        map<wstring, wstring> defaultParamList;

        ErrorCode error = this->ImageBuilder.EndBuildComposeDeploymentType(
            operation,
            serviceManifest,
            applicationManifestId,
            applicationManifest,
            applicationHealthPolicy,
            defaultParamList,
            mergedComposeFile);

        if (error.IsSuccess())
        {
            appManifest_ = make_shared<StoreDataApplicationManifest>(
                this->AppTypeContext.TypeName,
                this->AppTypeContext.TypeVersion,
                move(applicationManifest),
                move(applicationHealthPolicy),
                move(defaultParamList));

            svcManifest_ = make_shared<StoreDataServiceManifest>(
                this->AppTypeContext.TypeName,
                this->AppTypeContext.TypeVersion,
                move(serviceManifest));

            mergedComposeFile_ = move(mergedComposeFile);
        }
        else
        {
            provisioningError_ = error;
        }

        this->FinishBuildComposeDeploymentType(operation->Parent);
    }

    void FinishBuildComposeDeploymentType(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error(ErrorCodeValue::Success);

        auto storeTx = this->CreateTransaction();

        error = owner_.Context.Refresh(storeTx);
        if (!error.IsSuccess())
        {
            WriteInfo(
                DockerComposeTraceComponent,
                "{0} failed to refresh context: error={1}",
                this->TraceId,
                error);
        }

        if (provisioningError_.IsSuccess())
        {
            this->AppTypeContext.ManifestDataInStore = true;

            this->AppTypeContext.QueryStatusDetails = L"";

            error = storeTx.Insert(this->AppTypeContext);

            if (error.IsSuccess())
            {
                error = this->AppTypeContext.Complete(storeTx);
            }

            if (error.IsSuccess())
            {
                error = storeTx.UpdateOrInsertIfNotFound(*appManifest_);
            }

            if (error.IsSuccess())
            {
                error = storeTx.UpdateOrInsertIfNotFound(*svcManifest_);
            }

            if (error.IsSuccess())
            {
                composeDeploymentInstanceData_.UpdateMergedComposeFile(storeTx, mergedComposeFile_);
                error = owner_.Context.UpdateComposeDeploymentStatus(storeTx, ComposeDeploymentStatus::Creating);
            }
        }
        else
        {
            //TODO: If IB build fails, this path should fail applicationtype. Consider fail app as well in the following path.
            error = provisioningError_;
        }

        if (error.IsSuccess())
        {
            auto innerOperation = StoreTransaction::BeginCommit(
                move(storeTx),
                owner_.Context,
                [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
                thisSPtr);
            this->OnCommitComplete(innerOperation, true);
        }
        else if (this->ShouldRefreshAndRetry(error))
        {
            WriteInfo(
                DockerComposeTraceComponent,
                "{0} retry persist provisioning results: error={1}",
                this->TraceId,
                error);

            Threadpool::Post(
                [this, thisSPtr]() { this->FinishBuildComposeDeploymentType(thisSPtr); },
                this->Replica.GetRandomizedOperationRetryDelay(error));
        }
        else
        {
            WriteInfo(
                DockerComposeTraceComponent,
                "{0} provisioning failed: error={1}",
                this->TraceId,
                error);

            this->TryComplete(thisSPtr, move(error));
        }
    }

    ProcessComposeDeploymentContextAsyncOperation & owner_;
    wstring mergedComposeFile_;
    StoreDataComposeDeploymentInstance composeDeploymentInstanceData_;
};

class ProcessComposeDeploymentContextAsyncOperation::CreateComposeDeploymentContextAsyncOperation: public ProcessApplicationContextAsyncOperation
{
public:
    CreateComposeDeploymentContextAsyncOperation(
        __in ProcessComposeDeploymentContextAsyncOperation & owner,
        __in RolloutManager & rolloutManager,
        __in ApplicationContext & applicationContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ProcessApplicationContextAsyncOperation(
            rolloutManager,
            applicationContext,
            timeout,
            callback,
            parent)
        , owner_(owner)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CreateComposeDeploymentContextAsyncOperation>(operation)->Error;
    }

    __declspec(property(get=get_ComposeDeploymentContext)) ComposeDeploymentContext & ComposeDeploymentContext;
    Management::ClusterManager::ComposeDeploymentContext & get_ComposeDeploymentContext() { return owner_.Context; }

private:
    virtual TimeSpan GetCommunicationTimeout() override
    {
        return ManagementConfig::GetConfig().MinOperationTimeout;
    }

    virtual TimeSpan GetImageBuilderTimeout() override
    {
        return TimeSpan::MaxValue;
    }

    void CheckDefaultServicePackages(AsyncOperationSPtr const & thisSPtr, ErrorCode && error) override
    {
        std::vector<StoreDataServicePackage> & servicePackages = appDescription_.ServicePackages;
        std::vector<PartitionedServiceDescriptor> & defaultServiceDescriptors = appDescription_.DefaultServices;

        // ensure that the type of each default service is supported by some package
        for (auto iter = defaultServiceDescriptors.begin(); iter != defaultServiceDescriptors.end(); ++iter)
        {
            Naming::PartitionedServiceDescriptor const & psd = (*iter);

            bool packageFound = false;
            for (auto iter2 = servicePackages.begin(); iter2 != servicePackages.end(); ++iter2)
            {
                if (iter2->TypeName == ServiceModelTypeName(psd.Service.Type.ServiceTypeName))
                {
                    packageFound = true;

                    ServiceContext serviceContext(
                        context_.ReplicaActivityId,
                        context_.ApplicationId,
                        iter2->PackageName,
                        iter2->PackageVersion,
                        context_.PackageInstance,
                        psd);

                    // Persist pending default service in ComposeDeploymentContext
                    ServiceModelServiceNameEx name(serviceContext.AbsoluteServiceName, serviceContext.ServiceDescriptor.Service.ServiceDnsName);
                    this->ComposeDeploymentContext.AddPendingDefaultService(move(name));
                    defaultServices_.push_back(move(serviceContext));

                    break;
                }
            }

            if (!packageFound)
            {
                WriteWarning(
                    DockerComposeTraceComponent, 
                    "{0} Service type '{1}' not supported in application {2}. Cannot create required service {3}.",
                    this->TraceId,
                    psd.Service.Type,
                    context_.ApplicationName,
                    psd.Service.Name);

                error = ErrorCodeValue::ServiceTypeNotFound;
                break;
            }
        }

        if (error.IsSuccess())
        {
            auto storeTx = this->CreateTransaction();

            // Must persist the list of pending default services. In the event of a non-retryable
            // create application failure, delete application must also clean up (delete) all
            // pending default services
            error = storeTx.Update(this->ComposeDeploymentContext);
            
            if (error.IsSuccess())
            {
                auto commitOperation = StoreTransaction::BeginCommit(
                    move(storeTx),
                    this->ComposeDeploymentContext,
                    [this](AsyncOperationSPtr const & operation) { this->OnCommitPendingDefaultServicesComplete(operation, false); },
                    thisSPtr);
                this->OnCommitPendingDefaultServicesComplete(commitOperation, true);
            }
        }

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
        }

    }

    void CreateServices(Common::AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<CreateComposeDeploymentParallelRetryableAsyncOperation>(
            *this,
            context_,
            static_cast<long>(defaultServices_.size()),
            [this](AsyncOperationSPtr const & parallelAsyncOperation, size_t operationIndex, ParallelOperationsCompletedCallback const & callback)->ErrorCode
        {
            return this->ScheduleCreateService(parallelAsyncOperation, operationIndex, callback);
        },
            [this](AsyncOperationSPtr const & operation) { this->OnCreateServicesComplete(operation, false); },
            thisSPtr);
        OnCreateServicesComplete(operation, true);
    }

    void FinishCreateApplication(AsyncOperationSPtr const & thisSPtr) override
    {
        ErrorCode error(ErrorCodeValue::Success);

        auto storeTx = this->CreateTransaction();

        uint64 appVersion(appDescription_.Application.Version);
        std::vector<StoreDataServicePackage> & servicePackages = appDescription_.ServicePackages;
        std::vector<StoreDataServiceTemplate> & serviceTemplates = appDescription_.ServiceTemplates;

        context_.SetApplicationVersion(appVersion);

        context_.SetHasReportedHealthEntity();

        for (auto iter = defaultServices_.begin(); iter != defaultServices_.end(); ++iter)
        {
            ServiceContext & serviceContext = (*iter);

            error = serviceContext.InsertCompletedIfNotFound(storeTx);

            if (!error.IsSuccess())
            {
                break;
            }
        }

        if (error.IsSuccess())
        {
            WriteNoise(DockerComposeTraceComponent, "{0} persisting {1} service packages", this->TraceId, servicePackages.size());

            for (auto iter = servicePackages.begin(); iter != servicePackages.end(); ++iter)
            {
                if (iter->TypeName.size() > static_cast<size_t>(ManagementConfig::GetConfig().MaxServiceTypeNameLength))
                {
                    WriteWarning(
                        DockerComposeTraceComponent, 
                        "{0} service type name '{1}' is too long: length = {2} max = {3}", 
                        this->TraceId, 
                        iter->TypeName,
                        iter->TypeName.size(),
                        ManagementConfig::GetConfig().MaxServiceTypeNameLength);

                    error = ErrorCodeValue::EntryTooLarge;
                    break;
                }
                else if (!(error = storeTx.InsertIfNotFound(*iter)).IsSuccess())
                {
                    break;
                }
            }
        }

        if (error.IsSuccess())
        {
            WriteNoise(DockerComposeTraceComponent, "{0} persisting {1} service templates", this->TraceId, serviceTemplates.size());

            for (auto iter = serviceTemplates.begin(); iter != serviceTemplates.end(); ++iter)
            {
                if (!(error = storeTx.InsertIfNotFound(*iter)).IsSuccess())
                {
                    break;
                }
            }
        }

        if (error.IsSuccess())
        {
            this->ComposeDeploymentContext.ClearPendingDefaultServices();

            error = context_.InsertCompletedIfNotFound(storeTx);
        }

        if (error.IsSuccess())
        {
            error = this->ComposeDeploymentContext.FinishCreating(storeTx, L"");
        }

        if (error.IsSuccess())
        {
            auto operation = StoreTransaction::BeginCommit(
                move(storeTx),
                this->ComposeDeploymentContext,
                [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
                thisSPtr);
            this->OnCommitComplete(operation, true);
        }
        else
        {
            this->TryComplete(thisSPtr, move(error));
        }

    }

    class CreateComposeDeploymentParallelRetryableAsyncOperation;

    ProcessComposeDeploymentContextAsyncOperation & owner_;
};

//
// *** class used to execute parallel operations with retries
//
class ProcessComposeDeploymentContextAsyncOperation::CreateComposeDeploymentContextAsyncOperation::CreateComposeDeploymentParallelRetryableAsyncOperation : public ProcessRolloutContextAsyncOperationBase::ParallelRetryableAsyncOperation
{
    DENY_COPY(CreateComposeDeploymentParallelRetryableAsyncOperation);

public:
    CreateComposeDeploymentParallelRetryableAsyncOperation(
        __in CreateComposeDeploymentContextAsyncOperation & owner,
        __in RolloutContext & context,
        long count,
        ParallelOperationCallback const & operationCallback,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : ProcessRolloutContextAsyncOperationBase::ParallelRetryableAsyncOperation(owner, context, count, operationCallback, callback, parent)
        , owner_(owner)
    {
    }

protected:
    virtual void ScheduleRetryOnInnerOperationFailed(
        AsyncOperationSPtr const & thisSPtr,
        RetryCallback const & callback,
        Common::ErrorCode && error) override
    {
        if (context_.ShouldTerminateProcessing())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        if (!owner_.IsRolloutManagerActive())
        {
            WriteInfo(
                CreateComposeDeploymentTraceComponent,
                "{0} cancelling retry - rollout manager not active",
                context_.ActivityId);

            this->TryComplete(thisSPtr, ErrorCodeValue::CMRequestAborted);
            return;
        }

        // Some of the operations didn't complete successfully, so schedule retry.
        // This will execute only the operations that were not successful.    
        // Increment timeouts to cause exponential delays on retries
        context_.IncrementTimeoutCount();

        auto retryDelay = RolloutManager::GetOperationRetry(context_, error);

        WriteNoise(
            CreateComposeDeploymentTraceComponent,
            "{0}: scheduling inner retry in retryDelay={1}, context timeouts={2}",
            context_.ActivityId,
            retryDelay,
            context_.GetTimeoutCount());

        context_.SetRetryTimer([this, thisSPtr, callback]()
        {
            callback(thisSPtr);
        },
            retryDelay);

    }

private:
    CreateComposeDeploymentContextAsyncOperation & owner_;
};

ProcessComposeDeploymentContextAsyncOperation::ProcessComposeDeploymentContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in ComposeDeploymentContext & context,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ProcessRolloutContextAsyncOperationBase(
        rolloutManager,
        context.ReplicaActivityId,
        TimeSpan::MaxValue,
        callback, 
        root)
    , context_(context)
    , applicationTypeContext_(
        context_.ReplicaActivityId,
        context_.TypeName,
        context_.TypeVersion,
        false,
        ApplicationTypeDefinitionKind::Enum::Compose,
        L"")

    , applicationContext_(
        context_.ReplicaActivityId,
        context_.ApplicationName,
        context_.ApplicationId,
        context_.GlobalInstanceCount,
        context_.TypeName,
        context_.TypeVersion,
        L"", //docker compose aplication currently does not have a ManifestId,
        map<wstring, wstring>(),
        ApplicationDefinitionKind::Enum::Compose)
{
}

void ProcessComposeDeploymentContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (context_.IsProvisionCompleted)
    {
        this->StartCreateApplication(thisSPtr);
    }
    else
    {
        this->StartProvisionApplicationType(thisSPtr);
    }
}

void ProcessComposeDeploymentContextAsyncOperation::StartProvisionApplicationType(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessComposeDeploymentProvisionAsyncOperation>(
        const_cast<RolloutManager&>(this->Rollout),
        *this,
        applicationTypeContext_,
        this->RemainingTimeout,
        [this](AsyncOperationSPtr const & operation) { this->OnProvisionComposeDeploymentComplete(operation, false); },
        thisSPtr); 
    this->OnProvisionComposeDeploymentComplete(operation, true);
}

void ProcessComposeDeploymentContextAsyncOperation::OnProvisionComposeDeploymentComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (this->CompletedSynchronously != expectedCompletedSynchronously) { return ; }

    ErrorCode error = ProcessComposeDeploymentProvisionAsyncOperation::End(operation);

    auto const & thisSPtr = operation->Parent;

    if (error.IsSuccess())
    {
        this->StartCreateApplication(thisSPtr);
    }
    else
    {
        WriteWarning(
            DockerComposeTraceComponent,
            "{0} compose deployment provision failed due to {1} {2}",
            TraceId,
            error,
            error.Message);

        auto storeTx = this->CreateTransaction();

        // This failure message will be kept as "Last error" in StatusDetails, so that it can be queried later.
        context_.StatusDetails = wformatString(GET_RC(Compose_App_Provisioning_Failed), error, error.Message);
        ErrorCode innerError = context_.Fail(storeTx);

        if (innerError.IsSuccess())
        {
            auto commitOperation = StoreTransaction::BeginCommit(
                move(storeTx),
                context_,
                [this, error](AsyncOperationSPtr const & commitOperation) { this->OnCommitProvisionComposeDeploymentFailureComplete(error, commitOperation, false); },
                thisSPtr);
            this->OnCommitProvisionComposeDeploymentFailureComplete(error, commitOperation, true);
        }
        else
        {
            WriteWarning(
                DockerComposeTraceComponent,
                "{0} Unable to persist docker compose application provision failure - {1}:{2}",
                TraceId,
                innerError,
                innerError.Message);

            this->TryComplete(thisSPtr, move(innerError));
        }
    }
}

void ProcessComposeDeploymentContextAsyncOperation::OnCommitProvisionComposeDeploymentFailureComplete(ErrorCode originalFailure, AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode commitError = StoreTransaction::EndCommit(operation);
    if (!commitError.IsSuccess())
    {
        WriteWarning(
            DockerComposeTraceComponent,
            "{0} commit docker compose application provision failed - {1}:{2}",
            TraceId,
            commitError,
            commitError.Message);
    }

    this->TryComplete(operation->Parent, originalFailure);
}

void ProcessComposeDeploymentContextAsyncOperation::StartCreateApplication(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = AsyncOperation::CreateAndStart<CreateComposeDeploymentContextAsyncOperation>(
        *this,
        const_cast<RolloutManager&>(this->Rollout),
        applicationContext_,
        this->RemainingTimeout,
        [this](AsyncOperationSPtr const & operation) { this->OnCreateApplicationComplete(operation, false); },
        thisSPtr);

    this->OnCreateApplicationComplete(operation, true);
}

void ProcessComposeDeploymentContextAsyncOperation::OnCreateApplicationComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = CreateComposeDeploymentContextAsyncOperation::End(operation);

    auto thisSPtr = operation->Parent;
    if (error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
    }
    // Retryable errors
    else if (error.IsError(ErrorCodeValue::OperationCanceled)
        || error.IsError(ErrorCodeValue::Timeout))
    {
        WriteInfo(
            DockerComposeTraceComponent,
            "{0} compose deployment create failed due to {1} {2}. Enqueue to retry.",
            TraceId,
            error,
            error.Message);
        this->TryComplete(thisSPtr, move(error));
    }
    else
    {
        WriteWarning(
            DockerComposeTraceComponent,
            "{0} compose deployment create failed due to {1} {2}",
            TraceId,
            error,
            error.Message);

        auto storeTx = this->CreateTransaction();

        // This failure message will be kept as "Last error" in StatusDetails until docker compose creation succeeds/fails
        context_.StatusDetails = wformatString(GET_RC(Compose_App_Creation_Failed), error, error.Message);
        ErrorCode innerError = context_.Fail(storeTx);;

        if (innerError.IsSuccess())
        {
            auto commitOperation = StoreTransaction::BeginCommit(
                move(storeTx),
                context_,
                [this, error](AsyncOperationSPtr const & commitOperation) { this->OnCommitCreateComposeDeploymentFailureComplete(error, commitOperation, false); },
                thisSPtr);
            this->OnCommitCreateComposeDeploymentFailureComplete(error, commitOperation ,true);
        }
        else
        {
            this->TryScheduleRetry(innerError, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->StartCreateApplication(thisSPtr); });
        }
    }
}

void ProcessComposeDeploymentContextAsyncOperation::OnCommitCreateComposeDeploymentFailureComplete(ErrorCode orginalError, AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    auto thisSPtr = operation->Parent;

    if (error.IsSuccess())
    {
        this->TryComplete(thisSPtr, orginalError);
    }
    else
    {
        this->RefreshContext();
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->StartCreateApplication(thisSPtr); });
    }
}

void ProcessComposeDeploymentContextAsyncOperation::TryScheduleRetry(
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

bool ProcessComposeDeploymentContextAsyncOperation::IsRetryable(ErrorCode const & error) const
{
    switch (error.ReadValue())
    {
        case ErrorCodeValue::ApplicationAlreadyExists:
        case ErrorCodeValue::ApplicationTypeAlreadyExists:
        case ErrorCodeValue::ComposeDeploymentAlreadyExists:
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

void ProcessComposeDeploymentContextAsyncOperation::StartTimer(
    AsyncOperationSPtr const & thisSPtr,
    RetryCallback const & callback,
    TimeSpan const delay)
{
    AcquireExclusiveLock lock(timerLock_);

    WriteNoise(
        DockerComposeTraceComponent,
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

ErrorCode ProcessComposeDeploymentContextAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<ProcessComposeDeploymentContextAsyncOperation>(operation)->Error;
}

ErrorCode ProcessComposeDeploymentContextAsyncOperation::RefreshContext()
{
    auto storeTx = this->CreateTransaction();

    return context_.Refresh(storeTx);
}

ErrorCode ProcessComposeDeploymentContextAsyncOperation::GetStoreDataComposeDeploymentInstance(
    StoreDataComposeDeploymentInstance &composeDeploymentInstanceData)
{
    auto storeTx = this->CreateTransaction();

    auto error = storeTx.ReadExact(composeDeploymentInstanceData);
    if (!error.IsSuccess())
    {
        error = ErrorCode(error.ReadValue(), wformatString(GET_RC(Compose_Deployment_Instance_Read_Failed), context_.ApplicationName, error.ReadValue()));
    }

    storeTx.CommitReadOnly();

    return error;
}
