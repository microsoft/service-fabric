// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Management/DnsService/include/DnsPropertyValue.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace Store;
using namespace ServiceModel;
using namespace Reliability;
using namespace Management::ClusterManager;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("DeleteApplicationContextAsyncOperation");

DeleteApplicationContextAsyncOperation::DeleteApplicationContextAsyncOperation(
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
    , appName_(context.ApplicationName)
    , serviceContexts_()
    , serviceNames_()
    , keepContextAlive_(false)
{
}

DeleteApplicationContextAsyncOperation::DeleteApplicationContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in ApplicationContext & context,
    __in bool keepContextAlive,
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
    , appName_(context.ApplicationName)
    , serviceContexts_()
    , serviceNames_()
    , keepContextAlive_(keepContextAlive)
{
}

void DeleteApplicationContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->PerfCounters.NumberOfAppRemoves.Increment();
    this->DeleteAllServices(thisSPtr);
}

void DeleteApplicationContextAsyncOperation::DeleteAllServices(AsyncOperationSPtr const & thisSPtr)
{
    auto error = this->ReadServiceContexts(serviceContexts_);
    if (error.IsSuccess())
    {
        for (auto iter = serviceContexts_.begin(); iter != serviceContexts_.end(); ++iter)
        {
            // The application context is in a DeletePending state so new Create/Delete service
            // requests cannot start. Wait for existing requests to complete before proceeding to
            // delete the application.
            //
            if (iter->IsPending)
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} waiting for incomplete service context {1}", 
                    this->TraceId, 
                    (*iter));

                error = ErrorCodeValue::CMRequestAlreadyProcessing;
                break;
            }
            else
            {
                ServiceModelServiceNameEx names(iter->AbsoluteServiceName, iter->ServiceDescriptor.Service.ServiceDnsName);
                serviceNames_.push_back(move(names));
            }
        }
    }

    if (error.IsSuccess())
    {
        // Must also delete pending default services if we are cleaning up a failed create application operation
        for (auto iter = context_.PendingDefaultServices.begin(); iter != context_.PendingDefaultServices.end(); ++iter)
        {
            ServiceModelServiceNameEx name(*iter);
            auto findIter = find(serviceNames_.begin(), serviceNames_.end(), name);
            if (findIter == serviceNames_.end())
            {
                serviceNames_.push_back(move(name));
            }
        }

        if (context_.IsCommitPending)
        {
            this->PrepareCommitDeleteApplication(thisSPtr);
        }
        else
        {
            if (!serviceNames_.empty())
            {
                this->ParallelDeleteServices(thisSPtr);
            }
            else
            {
                this->DeleteApplicationFromFM(thisSPtr);
            }
        }
    }

    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
    }
}

void DeleteApplicationContextAsyncOperation::ParallelDeleteServices(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = AsyncOperation::CreateAndStart<ParallelRetryableAsyncOperation>(
        *this,
        context_,
        static_cast<long>(serviceNames_.size()),
        [this](AsyncOperationSPtr const & parallelAsyncOperation, size_t operationIndex, ParallelOperationsCompletedCallback const & callback)->ErrorCode
        {
            return this->ScheduleDeleteService(parallelAsyncOperation, operationIndex, callback);
        },
        [this](Common::AsyncOperationSPtr const & operation) { this->OnParallelDeleteServicesComplete(operation, false); },
        thisSPtr);
    OnParallelDeleteServicesComplete(operation, true);
}

ErrorCode DeleteApplicationContextAsyncOperation::ScheduleDeleteService(
    AsyncOperationSPtr const & parallelAsyncOperation,
    size_t operationIndex,
    ParallelOperationsCompletedCallback const & callback)
{
    return this->Replica.ScheduleNamingAsyncWork(
        [parallelAsyncOperation, this, operationIndex](AsyncCallback const & jobQueueCallback)
        {
            return this->BeginDeleteService(parallelAsyncOperation, operationIndex, jobQueueCallback);
        },
        [this, operationIndex, callback](AsyncOperationSPtr const & operation) { this->EndDeleteService(operation, operationIndex, callback); });
}

AsyncOperationSPtr DeleteApplicationContextAsyncOperation::BeginDeleteService(
    AsyncOperationSPtr const & parallelAsyncOperation,
    size_t operationIndex,
    AsyncCallback const & jobQueueCallback)
{
    Common::NamingUri namingUri;
    bool success = NamingUri::TryParse(serviceNames_[operationIndex].Name, namingUri);
    ASSERT_IFNOT(success, "Could not parse {0} into a NamingUri", serviceNames_[operationIndex].Name);

    // parallelAsyncOperation keeps thisSPtr alive as its parent
    Common::ActivityId innerActivityId(context_.ActivityId, static_cast<uint64>(operationIndex));
    return this->Client.BeginDeleteService(
        context_.ApplicationName,
        DeleteServiceDescription(namingUri, context_.IsForceDelete),
        innerActivityId,
        this->RemainingTimeout,
        jobQueueCallback,
        parallelAsyncOperation);
}

void DeleteApplicationContextAsyncOperation::EndDeleteService(
    Common::AsyncOperationSPtr const & operation,
    size_t operationIndex,
    ParallelOperationsCompletedCallback const & callback)
{
    auto error = this->Client.EndDeleteService(operation);
    if (error.IsError(ErrorCodeValue::UserServiceNotFound))
    {
        error = ErrorCode::Success();
    }

    Common::AsyncOperationSPtr const & thisSPtr = operation->Parent;

    if (error.IsSuccess() && !this->serviceNames_[operationIndex].DnsName.empty())
    {
        error = this->Replica.ScheduleNamingAsyncWork(
            [this, thisSPtr, operationIndex](AsyncCallback const & jobQueueCallback)
            {
                return this->BeginDeleteServiceDnsName(
                    this->serviceNames_[operationIndex].Name,
                    this->serviceNames_[operationIndex].DnsName,
                    ActivityId, RemainingTimeout, jobQueueCallback, thisSPtr);
            },
            [this, callback](AsyncOperationSPtr const & operation) { this->OnDeleteServiceDnsNameComplete(operation, callback); });

        if (!error.IsSuccess()) { callback(thisSPtr, error); }
    }
    else
    {
        callback(thisSPtr, error);
    }
}

void DeleteApplicationContextAsyncOperation::OnDeleteServiceDnsNameComplete(
    Common::AsyncOperationSPtr const & operation,
    ParallelOperationsCompletedCallback const & callback)
{
    ErrorCode error = this->EndDeleteServiceDnsName(operation);
    callback(operation->Parent, error);
}

Common::ErrorCode DeleteApplicationContextAsyncOperation::ReadServiceContexts(
    __inout std::vector<ServiceContext> & serviceContexts)
{
    auto storeTx = this->CreateTransaction();

    auto error = ServiceContext::ReadApplicationServices(storeTx, context_.ApplicationId, serviceContexts);

    storeTx.CommitReadOnly();
    return error;
}

void DeleteApplicationContextAsyncOperation::OnParallelDeleteServicesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto casted = AsyncOperation::End<ParallelRetryableAsyncOperation>(operation);
    auto const & thisSPtr = operation->Parent;
    if (casted->Error.IsSuccess())
    {
        this->WriteInfo(TraceComponent, "{0}: all services have been deleted", this->TraceId);
        this->DeleteApplicationFromFM(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, casted->Error);
    }
}

void DeleteApplicationContextAsyncOperation::DeleteApplicationFromFM(Common::AsyncOperationSPtr const & thisSPtr)
{
    ApplicationIdentifier appId;
    auto error = ApplicationIdentifier::FromString(context_.ApplicationId.Value, appId);

    if (error.IsSuccess())
    {
        DeleteApplicationRequestMessageBody body(
            appId,
            ApplicationUpgradeContext::GetDeleteApplicationInstance(context_.PackageInstance));
        auto request = RSMessage::GetDeleteApplicationRequest().CreateMessage(body);

		request->Headers.Add(Transport::FabricActivityHeader(context_.ActivityId));

        auto operation = this->Router.BeginRequestToFM(
            move(request),
            TimeSpan::MaxValue,
            this->RemainingTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnDeleteApplicationFromFMComplete(operation, false); },
            thisSPtr);
        this->OnDeleteApplicationFromFMComplete(operation, true);
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

void DeleteApplicationContextAsyncOperation::OnDeleteApplicationFromFMComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    Transport::MessageUPtr reply;
    auto error = this->Router.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        DeleteApplicationReplyMessageBody body;
        if (reply->GetBody(body))
        {
            error = body.ErrorCodeValue;
        }
        else
        {
            error = ErrorCode::FromNtStatus(reply->GetStatus());
        }
    }

    // DeleteApplication was added to the FM in 3.1. For lower versions,
    // the FM will return OperationFailed upon failing to handle the
    // request.
    //
    if (error.IsError(ErrorCodeValue::ApplicationNotFound) ||
        error.IsError(ErrorCodeValue::OperationFailed))
    {
        error = ErrorCodeValue::Success;
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to route delete application request to FM: application={1} error={2}",
            this->TraceId,
            context_.ApplicationName,
            error);

        // let RolloutManager retry logic handle retry
        //
        error = ErrorCodeValue::OperationCanceled;
    }

    if (error.IsSuccess())
    {
        error = this->Replica.ScheduleNamingAsyncWork(
            [this, thisSPtr](AsyncCallback const & jobQueueCallback)
            {
                return this->BeginDeleteApplicationProperty(thisSPtr, jobQueueCallback);
            },
            [this](AsyncOperationSPtr const & operation) { this->EndDeleteApplicationProperty(operation); });
    }

    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
    }
}

AsyncOperationSPtr DeleteApplicationContextAsyncOperation::BeginDeleteApplicationProperty(
    AsyncOperationSPtr const & thisSPtr,
    Common::AsyncCallback const & jobQueueCallback)
{
    return this->Client.BeginDeleteProperty(
        context_.ApplicationName,
        Constants::ApplicationPropertyName,
        context_.ActivityId,
        this->RemainingTimeout,
        jobQueueCallback,
        thisSPtr);
}

void DeleteApplicationContextAsyncOperation::EndDeleteApplicationProperty(
    AsyncOperationSPtr const & operation)
{
    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    ErrorCode error = this->Client.EndDeleteProperty(operation);
    if (error.IsError(ErrorCodeValue::NameNotFound) || error.IsError(ErrorCodeValue::PropertyNotFound))
    {
        error = ErrorCodeValue::Success;
    }

    if (error.IsSuccess())
    {
        error = this->Replica.ScheduleNamingAsyncWork(
            [this, thisSPtr](AsyncCallback const & jobQueueCallback)
            {
                return this->BeginDeleteApplicationName(thisSPtr, jobQueueCallback);
            },
            [this](AsyncOperationSPtr const & operation) { this->EndDeleteApplicationName(operation); });
    }
    else if (error.IsError(ErrorCodeValue::InvalidNameUri))
    {
        // If the name is invalid, continue to cleanup the rest of the resources.
        // This error can happen during create application: create fails because the name is invalid,
        // so CM tries to cleanup the context, which gets into invalid name again.
        // Instead of failing the operation and retrying forever, since the is no need to delete the name
        // (as create also failed), continue with next step.
        this->CleanupImageStore(operation->Parent);
        return;
    }
    
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
    }
}

AsyncOperationSPtr DeleteApplicationContextAsyncOperation::BeginDeleteApplicationName(
    AsyncOperationSPtr const & thisSPtr,
    AsyncCallback const & jobQueueCallback)
{
    return this->Client.BeginDeleteName(
        context_.ApplicationName,
        context_.ActivityId,
        this->RemainingTimeout,
        jobQueueCallback,
        thisSPtr);
}

void DeleteApplicationContextAsyncOperation::EndDeleteApplicationName(
    AsyncOperationSPtr const & operation)
{
    auto error = this->Client.EndDeleteName(operation);
    if (error.IsSuccess())
    {
        this->CleanupImageStore(operation->Parent);
    }
    else
    {
        this->TryComplete(operation->Parent, move(error));
    }
}

void DeleteApplicationContextAsyncOperation::CleanupImageStore(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = this->ImageBuilder.BeginCleanupApplication(
        this->ActivityId,
        context_.TypeName,
        context_.ApplicationId,
        this->RemainingTimeout,
        [&](AsyncOperationSPtr const & operation) { this->OnCleanupImageStoreComplete(operation, false); },
        thisSPtr);
    this->OnCleanupImageStoreComplete(operation, true);
}

void DeleteApplicationContextAsyncOperation::OnCleanupImageStoreComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->ImageBuilder.EndCleanupApplication(operation);

    if (!error.IsSuccess() && this->IsImageBuilderSuccessOnCleanup(error))
    {
        error = ErrorCodeValue::Success;
    }

    if (error.IsSuccess())
    {
        // Delete application health from HM after services so that 
        // health queries remain available during delete
        //
        this->DeleteApplicationHealth(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, move(error));
    }
}

void DeleteApplicationContextAsyncOperation::DeleteApplicationHealth(AsyncOperationSPtr const & thisSPtr)
{
    IHealthAggregatorSPtr healthAggregator;
    auto error = this->Replica.GetHealthAggregator(healthAggregator);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: failed to get the health aggregator: {1}",
            this->TraceId,
            error);
        this->TryComplete(thisSPtr, move(error));
        return;
    }
    
    vector<HealthReport> reports;
    this->Replica.CreateDeleteApplicationHealthReport(
        context_.ApplicationName.ToString(),
        context_.GlobalInstanceCount,
        context_.SequenceNumber,
        reports);

    auto operation = healthAggregator->BeginUpdateApplicationsHealth(
        context_.ActivityId,
        move(reports),
        this->RemainingTimeout,
        [this](AsyncOperationSPtr const & operation) { this->OnDeleteApplicationHealthComplete(operation, false); },
        thisSPtr);
    this->OnDeleteApplicationHealthComplete(operation, true);
}

void DeleteApplicationContextAsyncOperation::OnDeleteApplicationHealthComplete(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    IHealthAggregatorSPtr healthAggregator;
    auto error = this->Replica.GetHealthAggregator(healthAggregator);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: failed to get the health aggregator: {1}",
            this->TraceId,
            error);
        this->TryComplete(thisSPtr, move(error));
        return;
    }

    error = healthAggregator->EndUpdateApplicationsHealth(operation);
        
    if (error.IsError(ErrorCodeValue::HealthEntityNotFound) || error.IsError(ErrorCodeValue::HealthEntityDeleted))
    {
        error = ErrorCodeValue::Success;
    }

    if (error.IsSuccess())
    {
        this->PrepareCommitDeleteApplication(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, move(error));
    }
}

void DeleteApplicationContextAsyncOperation::PrepareCommitDeleteApplication(AsyncOperationSPtr const & thisSPtr)
{
    context_.SetCommitPending();

    auto storeTx = this->CreateTransaction();

    ErrorCode error(ErrorCodeValue::Success);

    for (auto iter = serviceContexts_.begin(); iter != serviceContexts_.end(); ++iter)
    {
        if (!(error = storeTx.DeleteIfFound(*iter)).IsSuccess())
        {
            break;
        }
    }

    // delete template data items transactionally
    //
    if (error.IsSuccess())
    {
        vector<StoreDataServiceTemplate> serviceTemplates;
        error = StoreDataServiceTemplate::ReadServiceTemplates(storeTx, context_.ApplicationId, serviceTemplates);

        if (error.IsSuccess())
        {
            WriteNoise(TraceComponent, "{0} deleting {1} service templates", this->TraceId, serviceTemplates.size());

            for (auto iter = serviceTemplates.begin(); iter != serviceTemplates.end(); ++iter)
            {
                if (!(error = storeTx.DeleteIfFound(*iter)).IsSuccess())
                {
                    break;
                }
            }
        }
    }

    // delete package data items transactionally
    //
    if (error.IsSuccess())
    {
        vector<StoreDataServicePackage> servicePackages;
        error = StoreDataServicePackage::ReadServicePackages(storeTx, context_.ApplicationId, servicePackages);

        if (error.IsSuccess())
        {
            WriteNoise(TraceComponent, "{0} deleting {1} service packages", this->TraceId, servicePackages.size());

            for (auto iter = servicePackages.begin(); iter != servicePackages.end(); ++iter)
            {
                if (!(error = storeTx.DeleteIfFound(*iter)).IsSuccess())
                {
                    break;
                }
            }
        }
    }

    if (error.IsSuccess() && !keepContextAlive_)
    {
        error = storeTx.DeleteIfFound(context_);
    }

    if (error.IsSuccess())
    {
        auto appIdMap = make_unique<StoreDataApplicationIdMap>(context_.ApplicationId);

        error = storeTx.DeleteIfFound(*appIdMap);
    }

    if (error.IsSuccess())
    {
        auto upgradeContext = make_unique<ApplicationUpgradeContext>(context_.ApplicationName);

        error = storeTx.DeleteIfFound(*upgradeContext);
    }

    if (error.IsSuccess())
    {
        auto verifiedDomains = make_unique<StoreDataVerifiedUpgradeDomains>(context_.ApplicationId);

        error = storeTx.DeleteIfFound(*verifiedDomains);
    }

    if (error.IsSuccess())
    {
        this->CommitDeleteApplication(thisSPtr, storeTx);
    }
    else
    {
        this->TryComplete(thisSPtr, move(error));
    }
}

void DeleteApplicationContextAsyncOperation::CommitDeleteApplication(
    AsyncOperationSPtr const &thisSPtr,
    StoreTransaction &storeTx)
{
    auto operation = StoreTransaction::BeginCommit(
        move(storeTx),
        context_,
        [this](AsyncOperationSPtr const & operation) { this->OnCommitDeleteApplicationComplete(operation, false); },
        thisSPtr);
    this->OnCommitDeleteApplicationComplete(operation, true);
}

void DeleteApplicationContextAsyncOperation::OnCommitDeleteApplicationComplete(
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

        CMEvents::Trace->ApplicationDeleted(
            context_.ApplicationName.ToString(), 
            context_.PackageInstance,
            0, // dca_version
            this->ReplicaActivityId,
            context_.ManifestId);

        CMEvents::Trace->ApplicationDeletedOperational(
            Common::Guid::NewGuid(),
            context_.ApplicationName.ToString(), 
            context_.TypeName.Value,
            context_.TypeVersion.Value);
    }

    this->TryComplete(operation->Parent, move(error));
}

ErrorCode DeleteApplicationContextAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<DeleteApplicationContextAsyncOperation>(operation)->Error;
}

void DeleteApplicationContextAsyncOperation::OnCompleted()
{
    this->PerfCounters.NumberOfAppRemoves.Decrement();
}
