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
using namespace Reliability;

static StringLiteral const CreateSingleInstanceApplicationTraceComponent("CreateSingleInstanceApplicationTraceComponent");
static StringLiteral const CreateSingleInstanceDeploymentTraceComponent("CreateSingleInstanceDeploymentTraceComponent");

class ProcessSingleInstanceDeploymentContextAsyncOperation::CreateSingleInstanceApplicationContextAsyncOperation 
    : public ProcessApplicationContextAsyncOperation
{
public:
    CreateSingleInstanceApplicationContextAsyncOperation(
        __in ProcessSingleInstanceDeploymentContextAsyncOperation & owner,
        __in RolloutManager & rolloutManager,
        __in ApplicationContext & applicationContext,
        __in ApplicationTypeContext & applicationTypeContext,
        DeploymentType::Enum deploymentType,
        __in shared_ptr<StoreData> & storeData,
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
        , applicationTypeContext_(applicationTypeContext)
        , deploymentType_(deploymentType)
        , storeData_(storeData)
        , startedProvision_(false)
    {
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        auto error = this->Replica.AppTypeRequestTracker.TryStartProvision(
            context_.TypeName, 
            context_.TypeVersion, 
            nullptr);

        if (error.IsSuccess())
        {
            startedProvision_ = true;
            ProcessApplicationContextAsyncOperation::OnStart(thisSPtr);
        } 
        else
        {
            this->TryComplete(thisSPtr, error);
        }

    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CreateSingleInstanceApplicationContextAsyncOperation>(operation)->Error;
    }

    __declspec(property(get=get_SingleInstanceDeploymentContext)) SingleInstanceDeploymentContext & SingleInstanceDeploymentContext;
    Management::ClusterManager::SingleInstanceDeploymentContext & get_SingleInstanceDeploymentContext() { return owner_.Context; }

    void OnCompleted() override
    {
        if (startedProvision_)
        {
            this->Replica.AppTypeRequestTracker.FinishRequest(applicationTypeContext_.TypeName, applicationTypeContext_.TypeVersion);
        }
    }

private:
    virtual TimeSpan GetCommunicationTimeout() override
    {
        return ManagementConfig::GetConfig().MinOperationTimeout;
    }

    virtual TimeSpan GetImageBuilderTimeout() override
    {
        return TimeSpan::MaxValue;
    }

    void CallImageBuilder(AsyncOperationSPtr const & thisSPtr)
    {
        switch (deploymentType_)
        {
        case DeploymentType::Enum::V2Application:
        {
            StoreDataSingleInstanceApplicationInstance singleInstanceApplicationInstance = *(static_pointer_cast<StoreDataSingleInstanceApplicationInstance>(storeData_));

            auto storeTx = this->CreateTransaction();
            // Look up volume refs
            for (auto & service : singleInstanceApplicationInstance.ApplicationDescription.Services)
            {
                for (auto & codePackage : service.Properties.CodePackages)
                {
                    for (auto & containerVolumeRef : codePackage.VolumeRefs)
                    {
                        wstring const & volumeName = containerVolumeRef.Name;
                        auto storeDataVolume = StoreDataVolume(volumeName);
                        ErrorCode error = storeTx.ReadExact(storeDataVolume);

                        if (error.IsError(ErrorCodeValue::NotFound))
                        {
                            WriteInfo(
                                CreateSingleInstanceApplicationTraceComponent,
                                "{0} Volume {1} does not exist",
                                this->TraceId,
                                volumeName);

                            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::VolumeNotFound, wformatString(GET_RC(Volume_Not_Found), volumeName)));
                        }
                        else if (!error.IsSuccess())
                        {
                            WriteWarning(
                                CreateSingleInstanceApplicationTraceComponent,
                                "{0} failed to read StoreDataVolume {1}: ({2} {3})",
                                this->TraceId,
                                volumeName,
                                error,
                                error.Message);
                            this->TryComplete(thisSPtr, move(error));
                        }

                        containerVolumeRef.VolumeDescriptionSPtr = storeDataVolume.VolumeDescription;
                    }
                }
            }
            storeTx.CommitReadOnly();

            auto operation = this->ImageBuilder.BeginBuildSingleInstanceApplication(
                this->ActivityId,
                SingleInstanceDeploymentContext.TypeName,
                SingleInstanceDeploymentContext.TypeVersion,
                singleInstanceApplicationInstance.ApplicationDescription,
                context_.ApplicationName,
                context_.ApplicationId,
                TimeSpan::MaxValue,
                [this](AsyncOperationSPtr const & operation) { this->OnBuildSingleInstanceApplicationComplete(operation, false); },
                thisSPtr);
            this->OnBuildSingleInstanceApplicationComplete(operation, true);
            break;
        }
        default:
            Assert::CodingError("Unknown deploymentType in CallImageBuilder {0}", static_cast<int>(deploymentType_));
        }
    }

    void EndCreateProperty(AsyncOperationSPtr const & operation) override
    {
        auto const & thisSPtr = operation->Parent;

        // TODO: handle property already exists error.
        ErrorCode error = this->Client.EndPutProperty(operation);

        if (error.IsSuccess())
        {
            WriteInfo(
                CreateSingleInstanceApplicationTraceComponent,
                "{0} building: type={1} version={2}",
                this->TraceId,
                applicationTypeContext_.TypeName,
                applicationTypeContext_.TypeVersion);

            this->CallImageBuilder(thisSPtr);
       }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void OnBuildSingleInstanceApplicationComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        vector<ServiceModelServiceManifestDescription> serviceManifest;
        wstring applicationManifest;
        ApplicationHealthPolicy applicationHealthPolicy;
        map<wstring, wstring> defaultParamList;

        ErrorCode error = this->ImageBuilder.EndBuildSingleInstanceApplication(
            operation,
            serviceManifest,
            applicationManifest,
            applicationHealthPolicy,
            defaultParamList,
            appDescription_);

        this->CommitDigestedApplication(
            thisSPtr,
            error,
            move(serviceManifest),
            move(applicationManifest),
            move(applicationHealthPolicy),
            move(defaultParamList));
    }

    void CommitDigestedApplication(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode & error,
        vector<ServiceModelServiceManifestDescription> && serviceManifest,
        wstring && applicationManifest,
        ApplicationHealthPolicy && applicationHealthPolicy,
        map<wstring, wstring>  && defaultParamList)
    {
        if (error.IsSuccess())
        {
            wstring errorDetails;
            if (!appDescription_.TryValidate(errorDetails))
            {
                error = this->TraceAndGetErrorDetails(
                    CreateSingleInstanceApplicationTraceComponent,
                    ErrorCodeValue::ImageBuilderValidationError,
                    move(errorDetails));
            }
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }

        auto storeTx = this->CreateTransaction();

        if (error.IsSuccess())
        {
            appManifest_ = make_shared<StoreDataApplicationManifest>(
                applicationTypeContext_.TypeName,
                applicationTypeContext_.TypeVersion,
                move(applicationManifest),
                move(applicationHealthPolicy),
                move(defaultParamList));

            svcManifest_ = make_shared<StoreDataServiceManifest>(
                applicationTypeContext_.TypeName,
                applicationTypeContext_.TypeVersion,
                move(serviceManifest));

            applicationTypeContext_.ManifestDataInStore = true;
            applicationTypeContext_.QueryStatusDetails = L"";

            error = applicationTypeContext_.Complete(storeTx);
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
            auto innerOperation = StoreTransaction::BeginCommit(
                move(storeTx),
                SingleInstanceDeploymentContext,
                [this](AsyncOperationSPtr const & operation) { this->OnCommitBuildComplete(operation, false); },
                thisSPtr);
            this->OnCommitBuildComplete(innerOperation, true);
        }
        // Leave RolloutManager for retrying.
        else
        {
            WriteInfo(
                CreateSingleInstanceApplicationTraceComponent,
                "{0} build single instance application failed: error={1}",
                this->TraceId,
                error);

            this->TryComplete(thisSPtr, move(error));
        }
    }

    void OnCommitBuildComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        ErrorCode error = StoreTransaction::EndCommit(operation);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            this->SendCreateApplicationRequestToFM(thisSPtr);
        }
    }

    void CheckDefaultServicePackages(AsyncOperationSPtr const & thisSPtr, ErrorCode && error) override
    {
        std::vector<StoreDataServicePackage> & servicePackages = appDescription_.ServicePackages;
        std::vector<PartitionedServiceDescriptor> & defaultServiceDescriptors = appDescription_.DefaultServices;

        // ensure that the type of each default service is supported by some package
        if (error.IsSuccess())
        {
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
                            this->ReplicaActivityId,
                            context_.ApplicationId,
                            iter2->PackageName,
                            iter2->PackageVersion,
                            context_.PackageInstance,
                            psd);

                        // Persist pending default service in SingleInstanceDeploymentContext
                        ServiceModelServiceNameEx name(serviceContext.AbsoluteServiceName, serviceContext.ServiceDescriptor.Service.ServiceDnsName);
                        this->SingleInstanceDeploymentContext.AddPendingDefaultService(move(name));

                        defaultServices_.push_back(move(serviceContext));

                        break;
                    }
                }

                if (!packageFound)
                {
                    WriteWarning(
                        CreateSingleInstanceApplicationTraceComponent,
                        "{0} Service type '{1}' not supported in application {2}. Cannot create required service {3}.",
                        this->TraceId,
                        psd.Service.Type,
                        context_.ApplicationName,
                        psd.Service.Name);

                    error = ErrorCodeValue::ServiceTypeNotFound;
                    break;
                }
            }
        }

        if (error.IsSuccess())
        {
            auto storeTx = this->CreateTransaction();

            error = SingleInstanceDeploymentContext.Refresh(storeTx);
            if (!error.IsSuccess())
            {
                WriteInfo(
                    CreateSingleInstanceApplicationTraceComponent,
                    "{0} failed to refresh context: error={1}",
                    this->TraceId,
                    error);
            }

            // Must persist the list of pending default services. In the event of a non-retryable
            // create application failure, delete application must also clean up (delete) all
            // pending default services
            if (error.IsSuccess())
            {
                error = storeTx.Update(this->SingleInstanceDeploymentContext);
            }

            if (error.IsSuccess())
            {
                auto commitOperation = StoreTransaction::BeginCommit(
                    move(storeTx),
                    this->SingleInstanceDeploymentContext,
                    [this](AsyncOperationSPtr const & operation) { this->OnCommitPendingDefaultServicesComplete(operation, false); },
                    thisSPtr);
                this->OnCommitPendingDefaultServicesComplete(commitOperation, true);
            }
        }

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void CreateServices(Common::AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<CreateSingleInstanceDeploymentRetryableAsyncOperation>(
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
            WriteNoise(CreateSingleInstanceApplicationTraceComponent, "{0} persisting {1} service packages", this->TraceId, servicePackages.size());

            for (auto iter = servicePackages.begin(); iter != servicePackages.end(); ++iter)
            {
                if (iter->TypeName.size() > static_cast<size_t>(ManagementConfig::GetConfig().MaxServiceTypeNameLength))
                {
                    WriteWarning(
                        CreateSingleInstanceApplicationTraceComponent, 
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
            WriteNoise(
                CreateSingleInstanceApplicationTraceComponent,
                "{0} persisting {1} service templates",
                this->TraceId,
                serviceTemplates.size());

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
            this->SingleInstanceDeploymentContext.ClearPendingDefaultServices();

            error = context_.Complete(storeTx);
        }

        if (error.IsSuccess())
        {
            error = this->SingleInstanceDeploymentContext.FinishCreating(storeTx, L"");
        }

        if (error.IsSuccess())
        {
            auto operation = StoreTransaction::BeginCommit(
                move(storeTx),
                this->SingleInstanceDeploymentContext,
                [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
                thisSPtr);
            this->OnCommitComplete(operation, true);
        }
        else
        {
            this->TryComplete(thisSPtr, move(error));
        }

    }

    class CreateSingleInstanceDeploymentRetryableAsyncOperation;

    ProcessSingleInstanceDeploymentContextAsyncOperation & owner_;
    ApplicationTypeContext applicationTypeContext_;
    DeploymentType::Enum deploymentType_;
    shared_ptr<StoreData> storeData_;
    bool startedProvision_;
    std::shared_ptr<StoreDataApplicationManifest> appManifest_;
    std::shared_ptr<StoreDataServiceManifest> svcManifest_;
};

//
// *** class used to execute parallel operations with retries
//
class ProcessSingleInstanceDeploymentContextAsyncOperation::CreateSingleInstanceApplicationContextAsyncOperation::CreateSingleInstanceDeploymentRetryableAsyncOperation : public ProcessRolloutContextAsyncOperationBase::ParallelRetryableAsyncOperation
{
    DENY_COPY(CreateSingleInstanceDeploymentRetryableAsyncOperation);

public:
    CreateSingleInstanceDeploymentRetryableAsyncOperation(
        __in CreateSingleInstanceApplicationContextAsyncOperation & owner,
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
            this->TryComplete(thisSPtr, error);
            return;
        }

        if (!owner_.IsRolloutManagerActive())
        {
            WriteInfo(
                CreateSingleInstanceApplicationTraceComponent,
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
            CreateSingleInstanceApplicationTraceComponent,
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
    CreateSingleInstanceApplicationContextAsyncOperation & owner_;
};

ProcessSingleInstanceDeploymentContextAsyncOperation::ProcessSingleInstanceDeploymentContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in SingleInstanceDeploymentContext & context,
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
    , applicationTypeContext_(context.TypeName, context.TypeVersion)
    , timerSPtr_()
    , timerLock_()
{
}

void ProcessSingleInstanceDeploymentContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->BuildApplicationPackage(thisSPtr);
}

void ProcessSingleInstanceDeploymentContextAsyncOperation::BuildApplicationPackage(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();
    ErrorCode error = this->Replica.GetApplicationContext(storeTx, applicationContext_);
    if (error.IsSuccess())
    {
        error = this->Replica.GetApplicationTypeContext(storeTx, applicationTypeContext_);
    }

    shared_ptr<StoreData> storeDataInstance;
    if (error.IsSuccess())
    {
        error = this->Replica.GetStoreData(
            storeTx,
            context_.DeploymentType,
            context_.DeploymentName,
            context_.TypeVersion,
            storeDataInstance);
    }
    storeTx.CommitReadOnly();

    if (!error.IsSuccess())
    {
        this->RefreshContext();
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->BuildApplicationPackage(thisSPtr); });
        return;
    }
    else
    {
        auto operation = AsyncOperation::CreateAndStart<CreateSingleInstanceApplicationContextAsyncOperation>(
            *this,
            const_cast<RolloutManager&>(this->Rollout),
            applicationContext_,
            applicationTypeContext_,
            context_.DeploymentType,
            storeDataInstance,
            this->RemainingTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnCreateApplicationComplete(operation, false); },
            thisSPtr);

        this->OnCreateApplicationComplete(operation, true);
    }
}

void ProcessSingleInstanceDeploymentContextAsyncOperation::OnCreateApplicationComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = CreateSingleInstanceApplicationContextAsyncOperation::End(operation);

    auto const & thisSPtr = operation->Parent;

    if (error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
    }
    // Retryable errors
    else if (error.IsError(ErrorCodeValue::OperationCanceled)
        || error.IsError(ErrorCodeValue::Timeout))
    {
        WriteInfo(
            CreateSingleInstanceDeploymentTraceComponent,
            "{0} single instance deployment create failed due to {1} {2}. Enqueue to retry.",
            TraceId,
            error,
            error.Message);
        this->TryComplete(thisSPtr, move(error));
    }
    else
    {
        WriteWarning(
            CreateSingleInstanceDeploymentTraceComponent,
            "{0} single instance deployment create failed due to {1} {2}",
            TraceId,
            error,
            error.Message);

        auto storeTx = this->CreateTransaction();

        // This failure message will be kept as "Last error" in StatusDetails until single instance deployment succeeds/fails
        context_.StatusDetails = wformatString(GET_RC(Container_Group_Creation_Failed), error, error.Message);
        ErrorCode innerError = context_.Fail(storeTx);;

        if (innerError.IsSuccess())
        {
            innerError = applicationContext_.Fail(storeTx);
        }

        if (innerError.IsSuccess())
        {
            innerError = applicationTypeContext_.Fail(storeTx);
        }

        if (innerError.IsSuccess())
        {
            auto commitOperation = StoreTransaction::BeginCommit(
                move(storeTx),
                context_,
                [this, error](AsyncOperationSPtr const & commitOperation) { this->OnCommitCreateDeploymentFailureComplete(error, commitOperation, false); },
                thisSPtr);
            this->OnCommitCreateDeploymentFailureComplete(error, commitOperation ,true);
        }
        else
        {
            this->TryScheduleRetry(innerError, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->BuildApplicationPackage(thisSPtr); });
        }
    }
}

void ProcessSingleInstanceDeploymentContextAsyncOperation::OnCommitCreateDeploymentFailureComplete(
    ErrorCode const & orginalError,
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    auto const & thisSPtr = operation->Parent;

    if (error.IsSuccess())
    {
        this->TryComplete(thisSPtr, orginalError);
    }
    else
    {
        this->RefreshContext();
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->BuildApplicationPackage(thisSPtr); });
    }
}

ErrorCode ProcessSingleInstanceDeploymentContextAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<ProcessSingleInstanceDeploymentContextAsyncOperation>(operation)->Error;
}

void ProcessSingleInstanceDeploymentContextAsyncOperation::TryScheduleRetry(
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

bool ProcessSingleInstanceDeploymentContextAsyncOperation::IsRetryable(ErrorCode const & error) const
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

void ProcessSingleInstanceDeploymentContextAsyncOperation::StartTimer(
    AsyncOperationSPtr const & thisSPtr,
    RetryCallback const & callback,
    TimeSpan const delay)
{
    AcquireExclusiveLock lock(timerLock_);

    WriteNoise(
        CreateSingleInstanceDeploymentTraceComponent,
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

ErrorCode ProcessSingleInstanceDeploymentContextAsyncOperation::RefreshContext()
{
    auto storeTx = this->CreateTransaction();

    return context_.Refresh(storeTx);
}
