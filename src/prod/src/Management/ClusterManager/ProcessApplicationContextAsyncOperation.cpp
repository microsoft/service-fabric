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

StringLiteral const TraceComponent("ApplicationContextAsyncOperation");

ProcessApplicationContextAsyncOperation::ProcessApplicationContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in ApplicationContext & context,
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
    , appDescription_()
    , defaultServices_()
{
}

void ProcessApplicationContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->PerfCounters.NumberOfAppCreates.Increment();

    // Do not use the IsCommitPending optimization here since we must reload the list of 
    // default services on retry
    //
    auto error = this->Replica.ScheduleNamingAsyncWork(
        [this, thisSPtr](AsyncCallback const & jobQueueCallback)
        {
            return this->BeginCreateName(thisSPtr, jobQueueCallback);
        },
        [this](AsyncOperationSPtr const & operation) { this->EndCreateName(operation); });

    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
    }
}

AsyncOperationSPtr ProcessApplicationContextAsyncOperation::BeginCreateName(
    AsyncOperationSPtr const & thisSPtr,
    AsyncCallback const & jobQueueCallback)
{
    return this->Client.BeginCreateName(
        context_.ApplicationName,
        context_.ActivityId,
        this->GetCommunicationTimeout(),
        jobQueueCallback,
        thisSPtr);
}

void ProcessApplicationContextAsyncOperation::EndCreateName(AsyncOperationSPtr const & operation)
{
    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    ErrorCode error = this->Client.EndCreateName(operation);
    if (error.IsError(ErrorCodeValue::NameAlreadyExists))
    {
        error = ErrorCodeValue::Success;
    }

    if (error.IsSuccess())
    {
        error = this->Replica.ScheduleNamingAsyncWork(
            [this, thisSPtr](AsyncCallback const & jobQueueCallback)
            {
                return this->BeginCreateProperty(thisSPtr, jobQueueCallback);
            },
            [this](AsyncOperationSPtr const & operation) { this->EndCreateProperty(operation); });
    }

    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
    }
}

AsyncOperationSPtr ProcessApplicationContextAsyncOperation::BeginCreateProperty(
    AsyncOperationSPtr const & thisSPtr,
    AsyncCallback const & jobQueueCallback)
{
    vector<byte> flag;
    flag.push_back(1);

    // Put a property to prevent users from successfully calling DeleteName on the application name
    // This should be removed when access control is implemented.
    return this->Client.BeginPutProperty(
        context_.ApplicationName,
        Constants::ApplicationPropertyName,
        flag,
        context_.ActivityId,
        this->GetCommunicationTimeout(),
        jobQueueCallback,
        thisSPtr);
}

void ProcessApplicationContextAsyncOperation::EndCreateProperty(AsyncOperationSPtr const & operation)
{
    auto const & thisSPtr = operation->Parent;

    ErrorCode error = this->Client.EndPutProperty(operation);

    // We do not write any service contexts for default services that are still pending creation.
    // Once all the default services have been successfully created (i.e. create application has
    // completed), we will write service contexts for the completed default services to track
    // their existence.
    //
    if (error.IsSuccess())
    {
        auto buildApplicationOperation = this->ImageBuilder.BeginBuildApplication(
            this->ActivityId,
            context_.ApplicationName,
            context_.ApplicationId,
            context_.TypeName,
            context_.TypeVersion,
            context_.Parameters,
            this->GetImageBuilderTimeout(),
            [&](AsyncOperationSPtr const & operation) { this->OnBuildApplicationComplete(operation, false); },
            thisSPtr);
        this->OnBuildApplicationComplete(buildApplicationOperation, true);
    }
    else
    {
        this->TryComplete(thisSPtr, move(error));
    }
}

void ProcessApplicationContextAsyncOperation::OnBuildApplicationComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->ImageBuilder.EndBuildApplication(operation, /*out*/ appDescription_);

    if (error.IsSuccess())
    {
        wstring errorDetails;
        if (!appDescription_.TryValidate(errorDetails))
        {
            error = this->TraceAndGetErrorDetails(
                TraceComponent,
                ErrorCodeValue::ImageBuilderValidationError,
                move(errorDetails));
        }

        this->SendCreateApplicationRequestToFM(thisSPtr);
    }

    if (!error.IsSuccess())
	{
		this->TryComplete(thisSPtr, move(error));
		return;
	}
}

void ProcessApplicationContextAsyncOperation::SendCreateApplicationRequestToFM(
    AsyncOperationSPtr const & thisSPtr)
{
    // Send the request to FM with capacity description and with RG settings
    ApplicationIdentifier appId;
    ErrorCode error = ApplicationIdentifier::FromString(context_.ApplicationId.Value, appId);

    if (error.IsSuccess())
    {
        CreateApplicationRequestMessageBody body(
            context_.ApplicationName,
            appId,
            context_.PackageInstance,
            context_.ApplicationCapacity,
            appDescription_.ResourceGovernanceDescriptions,
            appDescription_.CodePackageContainersImages,
            appDescription_.Networks);

        auto request = RSMessage::GetCreateApplicationRequest().CreateMessage(body);

        request->Headers.Add(Transport::FabricActivityHeader(context_.ActivityId));

        auto sendToFMOperation = this->Router.BeginRequestToFM(
            move(request),
            TimeSpan::MaxValue,
            this->RemainingTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnRequestToFMComplete(operation, false); },
            thisSPtr);
        this->OnRequestToFMComplete(sendToFMOperation, true);
    }
    else
    {
        WriteError(
            TraceComponent,
            "{0} failed to parse as application ID: '{1}'",
            this->TraceId,
            context_.ApplicationId);

        this->TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
    }
}

void ProcessApplicationContextAsyncOperation::OnRequestToFMComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    Transport::MessageUPtr reply;
    auto error = this->Router.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        BasicFailoverReplyMessageBody body;
        if (reply->GetBody(body))
        {
            error = body.ErrorCodeValue;
        }
        else
        {
            error = ErrorCode::FromNtStatus(reply->GetStatus());
        }
    }

    // UpdateApplication was added to the FM in 4.3. For lower versions,
    // the FM will return OperationFailed upon failing to handle the
    // request.
    if (error.IsError(ErrorCodeValue::ApplicationAlreadyExists) ||
        error.IsError(ErrorCodeValue::OperationFailed) ||
        error.IsSuccess())
    {
        // check if the type of each default service is supported by some package
        error = ErrorCodeValue::Success;
        this->CheckDefaultServicePackages(thisSPtr, move(error));
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to route update request to FM: application={1} error={2}",
            this->TraceId,
            context_.ApplicationName,
            error);

        // let RolloutManager retry logic handle retry if there is capacity in the cluster
        if (!error.IsError(ErrorCodeValue::NetworkNotFound) &&
            !error.IsError(ErrorCodeValue::InsufficientClusterCapacity))
        {
            error = ErrorCodeValue::OperationCanceled;
        }
        this->TryComplete(thisSPtr, move(error));
    }
}

void ProcessApplicationContextAsyncOperation::CheckDefaultServicePackages(AsyncOperationSPtr const & thisSPtr, ErrorCode && error)
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

                ServiceModelServiceNameEx names(serviceContext.AbsoluteServiceName, serviceContext.ServiceDescriptor.Service.ServiceDnsName);
                context_.AddPendingDefaultService(move(names));

                defaultServices_.push_back(move(serviceContext));

                break;
            }
        }

        if (!packageFound)
        {
            WriteWarning(
                TraceComponent, 
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
        if (context_.IsCommitPending || defaultServices_.empty())
        {
            error = this->ReportApplicationPolicy(TraceComponent, thisSPtr, context_);
        }
        else
        {
            auto storeTx = this->CreateTransaction();

            // Must persist the list of pending default services. In the event of a non-retryable
            // create application failure, delete application must also clean up (delete) all
            // pending default services
            //
            error = storeTx.Update(context_);

            if (error.IsSuccess())
            {
                auto commitOperation = StoreTransaction::BeginCommit(
                    move(storeTx),
                    context_,
                    [this](AsyncOperationSPtr const & operation) { this->OnCommitPendingDefaultServicesComplete(operation, false); },
                    thisSPtr);
                this->OnCommitPendingDefaultServicesComplete(commitOperation, true);
            }
        }
    }

    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
    }
}

void ProcessApplicationContextAsyncOperation::OnCommitPendingDefaultServicesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    auto const & thisSPtr = operation->Parent;

    if (error.IsSuccess())
    {
        this->CreateServices(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, move(error));
    }
}

void ProcessApplicationContextAsyncOperation::CreateServices(
    Common::AsyncOperationSPtr const & thisSPtr)
{
    auto operation = AsyncOperation::CreateAndStart<ParallelRetryableAsyncOperation>(
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

ErrorCode ProcessApplicationContextAsyncOperation::ScheduleCreateService(
    AsyncOperationSPtr const & parallelAsyncOperation,
    size_t operationIndex,
    ParallelOperationsCompletedCallback const & callback)
{
    return this->Replica.ScheduleNamingAsyncWork(
        [parallelAsyncOperation, this, operationIndex](AsyncCallback const & jobQueueCallback)
        {
            return this->BeginCreateService(parallelAsyncOperation, operationIndex, jobQueueCallback);
        },
        [this, callback](AsyncOperationSPtr const & operation) { this->EndCreateService(operation, callback); });
}

AsyncOperationSPtr ProcessApplicationContextAsyncOperation::BeginCreateService(
    AsyncOperationSPtr const & parallelAsyncOperation,
    size_t operationIndex,
    AsyncCallback const & jobQueueCallback)
{
    //
    // if the service has a dns name, then try to create the dns name first, if that succeeds
    // create the service at FM.
    //
    Common::ActivityId innerActivityId(context_.ActivityId, static_cast<uint64>(operationIndex));
    auto const &defaultServiceContext = defaultServices_[operationIndex];
    return AsyncOperation::CreateAndStart<CreateDefaultServiceWithDnsNameIfNeededAsyncOperation>(
        defaultServiceContext,
        *this,
        innerActivityId,
        this->GetCommunicationTimeout(),
        jobQueueCallback,
        parallelAsyncOperation);
}

void ProcessApplicationContextAsyncOperation::EndCreateService(
    Common::AsyncOperationSPtr const & operation,
    ParallelOperationsCompletedCallback const & callback)
{
    auto error = CreateDefaultServiceWithDnsNameIfNeededAsyncOperation::End(operation);

    callback(operation->Parent, error);
}

void ProcessApplicationContextAsyncOperation::OnCreateServicesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto casted = AsyncOperation::End<ParallelRetryableAsyncOperation>(operation);
    auto const & thisSPtr = operation->Parent;
    
    if (!casted->Error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to create the services: {1}",
            this->TraceId,
            casted->Error);

        thisSPtr->TryComplete(thisSPtr, casted->Error);
        return;
    }

    auto error = this->ReportApplicationPolicy(TraceComponent, thisSPtr, context_);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
    }
}

void ProcessApplicationContextAsyncOperation::OnReportApplicationPolicyComplete(
    __in ApplicationContext & context,
    AsyncOperationSPtr const & thisSPtr, 
    ErrorCode const & error)
{
    UNREFERENCED_PARAMETER(context);

    if (error.IsSuccess())
    {
        this->FinishCreateApplication(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void ProcessApplicationContextAsyncOperation::FinishCreateApplication(AsyncOperationSPtr const & thisSPtr)
{
    ErrorCode error(ErrorCodeValue::Success);

    context_.SetCommitPending();

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
        WriteNoise(TraceComponent, "{0} persisting {1} service packages", this->TraceId, servicePackages.size());

        for (auto iter = servicePackages.begin(); iter != servicePackages.end(); ++iter)
        {
            if (iter->TypeName.size() > static_cast<size_t>(ManagementConfig::GetConfig().MaxServiceTypeNameLength))
            {
                WriteWarning(
                    TraceComponent, 
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
        WriteNoise(TraceComponent, "{0} persisting {1} service templates", this->TraceId, serviceTemplates.size());

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
        context_.ClearPendingDefaultServices();

        error = context_.Complete(storeTx);
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
    else
    {
        this->TryComplete(thisSPtr, move(error));
    }
}

void ProcessApplicationContextAsyncOperation::OnCommitComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    if (error.IsSuccess())
    {
        context_.ResetCommitPending();

        CMEvents::Trace->ApplicationCreated(
            context_.ApplicationName.ToString(), 
            context_.PackageInstance, 
            0, // dca_version
            this->ReplicaActivityId, 
            context_.TypeName.Value, 
            context_.TypeVersion.Value,
            context_.ManifestId);

        CMEvents::Trace->ApplicationCreatedOperational(
            Common::Guid::NewGuid(),
            context_.ApplicationName.ToString(), 
            context_.TypeName.Value, 
            context_.TypeVersion.Value,
            context_.ApplicationDefinitionKind);
    }

    this->TryComplete(operation->Parent, move(error));
}

ErrorCode ProcessApplicationContextAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    auto casted = AsyncOperation::End<ProcessApplicationContextAsyncOperation>(operation);
    return casted->Error;
}

void ProcessApplicationContextAsyncOperation::OnCompleted()
{
    this->PerfCounters.NumberOfAppCreates.Decrement();
}

TimeSpan ProcessApplicationContextAsyncOperation::GetCommunicationTimeout()
{
    return this->RemainingTimeout;
}

TimeSpan ProcessApplicationContextAsyncOperation::GetImageBuilderTimeout()
{
    return this->RemainingTimeout;
}