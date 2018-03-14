// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Management/DnsService/include/DnsPropertyValue.h"

using namespace Common;
using namespace std;
using namespace Store;
using namespace Management::ClusterManager;
using namespace Management::HealthManager;

StringLiteral const ParallelRetryableAsyncOperationTraceComponent("ParallelRetryableAsyncOperation");
StringLiteral const CreateDnsNameAsyncOperationTraceComponent("CreateServiceDnsNameAsyncOperation");
StringLiteral const DeleteDnsNameAsyncOperationTraceComponent("DeleteServiceDnsNameAsyncOperation");
// --------------------------------------
// Inner class
// --------------------------------------
class ProcessRolloutContextAsyncOperationBase::CreateServiceDnsNameAsyncOperation
    : public TimedAsyncOperation
{
    DENY_COPY(CreateServiceDnsNameAsyncOperation)

public:
    CreateServiceDnsNameAsyncOperation(
        wstring const &serviceName,
        wstring const &serviceDnsName,
        ProcessRolloutContextAsyncOperationBase &owner,
        Common::ActivityId const &activityId,
        TimeSpan timeout,
        AsyncCallback callback,
        AsyncOperationSPtr const &parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , serviceName_(serviceName)
        , serviceDnsName_(serviceDnsName)
        , owner_(owner)
        , activityId_(activityId)
    {
    }

    static ErrorCode CreateServiceDnsNameAsyncOperation::End(AsyncOperationSPtr const &operation)
    {
        auto casted = AsyncOperation::End<CreateServiceDnsNameAsyncOperation>(operation);
        return casted->Error;
    }

    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        const std::wstring & dnsName = ServiceModel::SystemServiceApplicationNameHelper::PublicDnsServiceName;
        bool success = NamingUri::TryParse(dnsName, this->dnsUri_);
        ASSERT_IFNOT(success, "Could not parse {0} into a NamingUri", dnsName);

        ByteBuffer serializedDnsPropertyValue;
        auto error = GetSerializedPropertyValue(this->serviceName_, serializedDnsPropertyValue);
        if (!error.IsSuccess())
        {
            WriteInfo(
                CreateDnsNameAsyncOperationTraceComponent,
                "{0}: dns name serialization failed with {1}",
                activityId_,
                error);

            this->TryComplete(thisSPtr, error);
            return;
        }

        Naming::NamePropertyOperationBatch batch(this->dnsUri_);
        batch.AddCheckExistenceOperation(this->serviceDnsName_, false /*isExpected*/);
        batch.AddPutPropertyOperation(this->serviceDnsName_, move(serializedDnsPropertyValue), FABRIC_PROPERTY_TYPE_BINARY);

        auto operation = this->owner_.Client.BeginSubmitPropertyBatch(
            move(batch),
            activityId_,
            this->RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
            this->OnCreateDnsNameComplete(operation, false);
            },
            thisSPtr);

        this->OnCreateDnsNameComplete(operation, true);
    }

    void OnCreateDnsNameComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        Api::IPropertyBatchResultPtr result;
        ErrorCode error = this->owner_.Client.EndSubmitPropertyBatch(operation, result);
        if (error.IsSuccess())
        {
            error = result->GetError();
        }

        //
        // If service DNS name property is successfully created, we can proceed to actual service creation.
        // Otherwise inspect the failure. If the failure is due to the DNS name being used already,
        // check which service is using it.
        //
        if (error.IsSuccess())
        {
            this->TryComplete(operation->Parent, error);
            return;
        }
        else if (error.IsError(ErrorCodeValue::PropertyCheckFailed))
        {
            WriteInfo(
                CreateDnsNameAsyncOperationTraceComponent,
                "{0}: dns name ( {1} -> {2} ) creation failed with {3}",
                activityId_, this->serviceDnsName_, this->serviceName_, error);

            auto innerOp = this->owner_.Client.BeginGetProperty(this->dnsUri_, this->serviceDnsName_, this->RemainingTime,
                [this](AsyncOperationSPtr const &operation) { this->OnCheckExistingDnsServiceNameComplete(operation, false); },
                operation->Parent);

            this->OnCheckExistingDnsServiceNameComplete(innerOp, true);
        }
        else
        {
            WriteInfo(
                CreateDnsNameAsyncOperationTraceComponent,
                "{0}: dns name ( {1} -> {2} ) creation failed with {3}",
                activityId_, this->serviceDnsName_, this->serviceName_, error);

            this->TryComplete(operation->Parent, error);
            return;
        }
    }

    void OnCheckExistingDnsServiceNameComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        DNS::DnsPropertyValue value;
        Naming::NamePropertyResult result;
        ErrorCode error = this->owner_.Client.EndGetProperty(operation, result);
        if (error.IsSuccess())
        {
            std::vector<byte> bytes = result.TakeBytes();
            error = Common::FabricSerializer::Deserialize(value, bytes);
            if (error.IsSuccess() && !StringUtility::AreEqualCaseInsensitive(value.ServiceName, this->serviceName_))
            {
                error = ErrorCode(ErrorCodeValue::DnsNameInUse, wformatString(GET_RC(Dns_Name_In_Use), serviceName_, serviceDnsName_));
            }
        }

        WriteInfo(
            CreateDnsNameAsyncOperationTraceComponent,
            "{0}: dns name check ( {1} -> {2} ), naming store value {3}, error {4}",
            activityId_, this->serviceDnsName_, this->serviceName_, value.ServiceName, error);

        TryComplete(operation->Parent, error);
    }

    ErrorCode GetSerializedPropertyValue(wstring serviceName, __out ByteBuffer &serializedDnsPropertyValue)
    {
        DNS::DnsPropertyValue value(serviceName);
        return Common::FabricSerializer::Serialize(&value, serializedDnsPropertyValue);
    }

private:
    wstring serviceDnsName_;
    wstring serviceName_;
    ProcessRolloutContextAsyncOperationBase &owner_;
    Common::ActivityId activityId_;
    NamingUri dnsUri_;
};

class ProcessRolloutContextAsyncOperationBase::DeleteServiceDnsNameAsyncOperation
    : public TimedAsyncOperation
{
    DENY_COPY(DeleteServiceDnsNameAsyncOperation)

public:
    DeleteServiceDnsNameAsyncOperation(
        wstring const &serviceName,
        wstring const &serviceDnsName,
        ProcessRolloutContextAsyncOperationBase &owner,
        Common::ActivityId const &activityId,
        TimeSpan timeout,
        AsyncCallback callback,
        AsyncOperationSPtr const &parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , serviceName_(serviceName)
        , serviceDnsName_(serviceDnsName)
        , owner_(owner)
        , activityId_(activityId)
    {
    }

    static ErrorCode DeleteServiceDnsNameAsyncOperation::End(AsyncOperationSPtr const &operation)
    {
        auto casted = AsyncOperation::End<DeleteServiceDnsNameAsyncOperation>(operation);
        return casted->Error;
    }

    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        const std::wstring & dnsName = ServiceModel::SystemServiceApplicationNameHelper::PublicDnsServiceName;
        bool success = NamingUri::TryParse(dnsName, this->dnsUri_);
        ASSERT_IFNOT(success, "Could not parse {0} into a NamingUri", dnsName);

        auto innerOp = this->owner_.Client.BeginGetProperty(this->dnsUri_, this->serviceDnsName_, this->RemainingTime,
            [this](AsyncOperationSPtr const &operation) { this->OnCheckExistingDnsServiceNameComplete(operation, false); },
            thisSPtr);

        this->OnCheckExistingDnsServiceNameComplete(innerOp, true);
    }

    void OnCheckExistingDnsServiceNameComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        std::vector<byte> bytes;
        DNS::DnsPropertyValue value;
        Naming::NamePropertyResult result;
        ErrorCode error = this->owner_.Client.EndGetProperty(operation, result);
        if (error.IsSuccess())
        {
            bytes = result.TakeBytes();
            error = Common::FabricSerializer::Deserialize(value, bytes);
            if (error.IsSuccess() && !StringUtility::AreEqualCaseInsensitive(value.ServiceName, this->serviceName_))
            {
                error = ErrorCodeValue::DnsServiceNotFound;
            }
        }

        WriteInfo(
            DeleteDnsNameAsyncOperationTraceComponent,
            "{0}: dns name check ( {1} -> {2} ), naming store value {3}, error {4}",
            activityId_, this->serviceDnsName_, this->serviceName_, value.ServiceName, error);

        if (!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::DnsServiceNotFound) || error.IsError(ErrorCodeValue::PropertyNotFound))
            {
                error = ErrorCode::Success();
            }

            TryComplete(operation->Parent, error);
            return;
        }

        auto innerOp = this->owner_.BeginSubmitDeleteDnsServicePropertyBatch(
            operation->Parent,
            this->serviceDnsName_,
            move(bytes),
            activityId_,
            [this](AsyncOperationSPtr const &op) { this->OnDeleteDnsNameComplete(op, false); });

        this->OnDeleteDnsNameComplete(innerOp, true);
    }

    void OnDeleteDnsNameComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        ErrorCode error = this->owner_.EndSubmitDeleteDnsServicePropertyBatch(
            operation,
            DeleteDnsNameAsyncOperationTraceComponent,
            this->serviceDnsName_,
            this->serviceName_);
        
        WriteInfo(
            DeleteDnsNameAsyncOperationTraceComponent,
            "{0}: dns name ( {1} -> {2} ) delete completed with error {3}",
            activityId_, this->serviceDnsName_, this->serviceName_, error);

        this->TryComplete(operation->Parent, error);
    }

private:
    wstring serviceDnsName_;
    wstring serviceName_;
    ProcessRolloutContextAsyncOperationBase &owner_;
    Common::ActivityId activityId_;
    NamingUri dnsUri_;
};

ProcessRolloutContextAsyncOperationBase::ParallelRetryableAsyncOperation::ParallelRetryableAsyncOperation(
    __in ProcessRolloutContextAsyncOperationBase & owner,
    __in RolloutContext & context,
    long count,
    ParallelOperationCallback const & operationCallback,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent)
    , owner_(owner)
    , context_(context)
    , count_(count)
    , waitForAllOperationsOnError_(false)
    , pendingOperations_(count)
    , operationsCompleted_()
    , operationCallback_(operationCallback)
{
    this->InitializeCtor();
}

ProcessRolloutContextAsyncOperationBase::ParallelRetryableAsyncOperation::ParallelRetryableAsyncOperation(
    __in ProcessRolloutContextAsyncOperationBase & owner,
    __in RolloutContext & context,
    long count,
    bool waitForAllOperationsOnError,
    ParallelOperationCallback const & operationCallback,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent)
    , owner_(owner)
    , context_(context)
    , count_(count)
    , waitForAllOperationsOnError_(waitForAllOperationsOnError)
    , pendingOperations_(count)
    , operationsCompleted_()
    , operationCallback_(operationCallback)
{
    this->InitializeCtor();
}

void ProcessRolloutContextAsyncOperationBase::ParallelRetryableAsyncOperation::InitializeCtor()
{
    operationsCompleted_.resize(count_, ErrorCode(ErrorCodeValue::NotReady));
}

Common::ErrorCode ProcessRolloutContextAsyncOperationBase::ParallelRetryableAsyncOperation::End(Common::AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<ParallelRetryableAsyncOperation>(operation)->Error;
}

void ProcessRolloutContextAsyncOperationBase::ParallelRetryableAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (count_ == 0)
    {
        this->TryComplete(thisSPtr, ErrorCode::Success());
        return;
    }

    // Start with initial count of operations
    this->Run(thisSPtr, count_);
}

void ProcessRolloutContextAsyncOperationBase::ParallelRetryableAsyncOperation::Run(
    AsyncOperationSPtr const & thisSPtr,
    long count)
{
    ASSERT_IF(count <= 0, "{0}: ParallelRetryableAsyncOperation: Run called with {1} operations", context_.ActivityId, count);

    pendingOperations_.store(count);
    bool reachedQueueFull = false;
    for (size_t i = 0; i < operationsCompleted_.size(); ++i)
    {
        if (!operationsCompleted_[i].IsSuccess())
        {
            if (reachedQueueFull)
            {
                // Do not start the operation, just call complete to decrement the count of remaining items.
                // The operation will be retried when pending operations complete.
                this->OnOperationComplete(thisSPtr, i, ErrorCode(ErrorCodeValue::JobQueueFull));
            }
            else
            {
                auto error = operationCallback_(
                    thisSPtr,
                    i,
                    [i, this](Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & error) { this->OnOperationComplete(thisSPtr, i, error); });

                if (!error.IsSuccess())
                {
                    if (error.IsError(ErrorCodeValue::JobQueueFull))
                    {
                        reachedQueueFull = true;
                    }

                    this->OnOperationComplete(thisSPtr, i, error);
                }
            }
        }
    }
}

void ProcessRolloutContextAsyncOperationBase::ParallelRetryableAsyncOperation::OnOperationComplete(
    Common::AsyncOperationSPtr const & thisSPtr,
    size_t index,
    Common::ErrorCode const & error)
{
    // Mark the operation status. There should never be multiple 
    // operations started for the same index, so this doesn't need to be protected with lock
    ASSERT_IF(operationsCompleted_[index].IsSuccess(), "{0}: ParallelRetryableAsyncOperation: operation at index {1} was already completed", context_.ActivityId, index);
    
    operationsCompleted_[index].ReadValue();
    operationsCompleted_[index] = error;
    
    if (!waitForAllOperationsOnError_ &&
        !error.IsSuccess() && 
        !this->IsInnerErrorRetryable(error))
    {
        WriteInfo(
            ParallelRetryableAsyncOperationTraceComponent,
            "{0}: operation at index {1} completed with non-retryable error {2}",
            context_.ActivityId,
            index,
            error);

        this->TryComplete(thisSPtr, error);
        return;
    }

    WriteNoise(
        ParallelRetryableAsyncOperationTraceComponent,
        "{0}: operation at index {1} completed with error {2}",
        context_.ActivityId,
        index,
        error);

    if (this->InternalIsCompleted)
    {
        // Nothing to do, a previous operation completed with non-retryable error
        return;
    }

    // Wait for all started operations to complete
    if (--pendingOperations_ == 0)
    {
        long failedOperations = 0;
        Common::ErrorCode firstError(ErrorCodeValue::Success);
        for (auto ix=0; ix<operationsCompleted_.size(); ++ix)
        {
            auto status = operationsCompleted_[ix];

            if (!status.IsSuccess())
            {
                if (this->IsInnerErrorRetryable(status))
                {
                    ++failedOperations;
                    firstError = ErrorCode::FirstError(firstError, status);
                }
                else
                {
                    WriteInfo(
                        ParallelRetryableAsyncOperationTraceComponent,
                        "{0}: operation at index {1} completed with non-retryable error {2}",
                        context_.ActivityId,
                        ix,
                        status);

                    this->TryComplete(thisSPtr, move(status));
                    return;
                }
            }
        }

        if (failedOperations == 0)
        {
            ASSERT_IFNOT(firstError.IsSuccess(), "{0}: All operations completed successfully, but firstError is {1}", context_.ActivityId, firstError);
#ifdef DBG
            for (size_t i = 0; i < operationsCompleted_.size(); ++i)
            {
                ASSERT_IFNOT(operationsCompleted_[i].IsSuccess(), "{0}: operations completed, but operation at {1} has status {2}", context_.ActivityId, i, operationsCompleted_[i]);
            }
#endif
            this->TryComplete(thisSPtr, ErrorCode::Success());
            return;
        }

        ASSERT_IF(firstError.IsSuccess(), "{0}: {1} operations failed, but firstError is {2}", context_.ActivityId, failedOperations, firstError);

        WriteInfo(
            ParallelRetryableAsyncOperationTraceComponent,
            "{0}: scheduling inner retry for {1}: {2} parallel operations failed: firstError={3}, context previous timeouts={4}",
            context_.ActivityId,
            context_,
            failedOperations,
            firstError,
            context_.GetTimeoutCount());

        this->ScheduleRetryOnInnerOperationFailed(
            thisSPtr,
            [this, failedOperations](AsyncOperationSPtr const & thisSPtr)
            {
                this->Run(thisSPtr, failedOperations);
            },
            move(firstError));
    }
}

void ProcessRolloutContextAsyncOperationBase::ParallelRetryableAsyncOperation::ScheduleRetryOnInnerOperationFailed(
    AsyncOperationSPtr const & thisSPtr,
    RetryCallback const & callback,
    Common::ErrorCode && firstError)
{
    if (context_.ShouldTerminateProcessing())
    {
        this->TryComplete(thisSPtr, std::move(firstError));
        return;
    }

    if (!owner_.IsRolloutManagerActive())
    {
        WriteInfo(
            ParallelRetryableAsyncOperationTraceComponent, 
            "{0} cancelling retry - rollout manager not active",
            context_.ActivityId);

        this->TryComplete(thisSPtr, ErrorCodeValue::CMRequestAborted);
        return;
    }

    // Some of the operations didn't complete successfully, so schedule retry.
    // This will execute only the operations that were not successful.    
    // Increment timeouts to cause exponential delays on retries
    context_.IncrementTimeoutCount();

    // Refresh the context to load any changes made by other requests (like timeout changes)
    auto refreshError = owner_.Rollout.Refresh(context_);
    if (!refreshError.IsSuccess())
    {
        this->TryComplete(thisSPtr, std::move(refreshError));
        return;
    }

    auto timeout = RolloutManager::GetOperationTimeout(context_);
    auto retryDelay = RolloutManager::GetOperationRetry(context_, firstError);

    WriteNoise(
        ParallelRetryableAsyncOperationTraceComponent,
        "{0}: scheduling inner retry in retryDelay={1}, context timeouts={2}, new timeout={3}",
        context_.ActivityId,
        retryDelay,
        context_.GetTimeoutCount(),
        timeout);

    context_.SetRetryTimer([this, thisSPtr, callback, timeout]()
        {
            owner_.ResetTimeout(timeout);
            callback(thisSPtr);
        }, 
        retryDelay);
}

bool ProcessRolloutContextAsyncOperationBase::ParallelRetryableAsyncOperation::IsInnerErrorRetryable(Common::ErrorCode const & error) const
{
    return RolloutManager::IsRetryable(error);
}

ProcessRolloutContextAsyncOperationBase::ProcessRolloutContextAsyncOperationBase(
    __in RolloutManager & rolloutManager,
    Store::ReplicaActivityId const & replicaActivityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : AsyncOperation(callback, root)
    , Store::ReplicaActivityTraceComponent<TraceTaskCodes::ClusterManager>(replicaActivityId)
    , rolloutManager_(rolloutManager)
    , timeoutHelper_(timeout)
{
}

ErrorCode ProcessRolloutContextAsyncOperationBase::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<ProcessRolloutContextAsyncOperationBase>(operation)->Error;
}

StoreTransaction ProcessRolloutContextAsyncOperationBase::CreateTransaction()
{
    return StoreTransaction::Create(
        this->Replica.ReplicatedStore, 
        this->Replica.PartitionedReplicaId, 
        this->ActivityId);
}

void ProcessRolloutContextAsyncOperationBase::ResetTimeout(TimeSpan const timeout)
{
    timeoutHelper_.SetRemainingTime(timeout);
}

ErrorCode ProcessRolloutContextAsyncOperationBase::ReportApplicationPolicy(StringLiteral const & traceComponent, AsyncOperationSPtr const & thisSPtr, __in ApplicationContext & context)
{
    ErrorCode error(ErrorCodeValue::Success);

    StoreDataApplicationManifest manifestData(context.TypeName, context.TypeVersion);
    {
        auto storeTx = this->CreateTransaction();

        error = storeTx.ReadExact(manifestData);

        if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteInfo(
                traceComponent, 
                "{0} manifest data {1}:{2} not found",
                this->TraceId, 
                context.TypeName,
                context.TypeVersion);

            error = ErrorCodeValue::Success;
        }

        if (error.IsSuccess())
        {
            storeTx.CommitReadOnly();
        }
    }
    
    IHealthAggregatorSPtr healthAggregator;
    error = this->Replica.GetHealthAggregator(healthAggregator);
    if (!error.IsSuccess())
    {
        return error;
    }

    vector<ServiceModel::HealthReport> reports;
    this->Replica.CreateApplicationHealthReport(
        context,
        Common::SystemHealthReportCode::CM_ApplicationCreated,
        manifestData.HealthPolicy.ToString(),
        reports);

    auto operation = healthAggregator->BeginUpdateApplicationsHealth(
        context.ActivityId,
        move(reports),
        this->RemainingTimeout,
        [this, &context](AsyncOperationSPtr const & operation) { this->OnReportApplicationPolicyComplete(context, operation, false); },
        thisSPtr);

    this->OnReportApplicationPolicyComplete(context, operation, true);
    return error;
}

void ProcessRolloutContextAsyncOperationBase::OnReportApplicationPolicyComplete(
    __in ApplicationContext & context,
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    IHealthAggregatorSPtr healthAggregator;
    auto error = this->Replica.GetHealthAggregator(healthAggregator);
    if (error.IsSuccess())
    {
        error = healthAggregator->EndUpdateApplicationsHealth(operation);

        if (error.IsError(ErrorCodeValue::HealthStaleReport))
        {
            error = ErrorCodeValue::Success;
        }
    }

    this->OnReportApplicationPolicyComplete(context, thisSPtr, error);
}

bool ProcessRolloutContextAsyncOperationBase::IsImageBuilderSuccessOnCleanup(ErrorCode const & error) const
{
    switch (error.ReadValue())
    {
        // Do not fail cleanup on these Image Builder errors (best effort)
        //
        case ErrorCodeValue::FileNotFound:
        case ErrorCodeValue::DirectoryNotFound:
        case ErrorCodeValue::InvalidDirectory:
        case ErrorCodeValue::PathTooLong:
            return true;
    }

    return false;
}

/// <summary>
/// Helper method for submitting a batch request to delete the DnsService from the naming store.
/// </summary>
AsyncOperationSPtr ProcessRolloutContextAsyncOperationBase::BeginSubmitDeleteDnsServicePropertyBatch(
    __in AsyncOperationSPtr const & thisSPtr,    
    __in wstring const & serviceDnsName,
    __in ByteBuffer && buffer, 
    __in Common::ActivityId const & activityId,
    __in AsyncCallback const & jobQueueCallback)
{
    ASSERT_IF(serviceDnsName.empty(), "Invalid service dns name, must not be empty");

    const std::wstring & dnsName = ServiceModel::SystemServiceApplicationNameHelper::PublicDnsServiceName;

    NamingUri dnsUri;
    bool success = NamingUri::TryParse(dnsName, dnsUri);
    ASSERT_IFNOT(success, "Could not parse {0} into a NamingUri", dnsName);

    Naming::NamePropertyOperationBatch batch(dnsUri);
    batch.AddCheckExistenceOperation(serviceDnsName, true /*isExpected*/);
    batch.AddCheckValuePropertyOperation(serviceDnsName, std::move(buffer), FABRIC_PROPERTY_TYPE_BINARY);
    batch.AddDeletePropertyOperation(serviceDnsName);

    return this->Client.BeginSubmitPropertyBatch(
        std::move(batch),
        activityId,
        this->RemainingTimeout,
        jobQueueCallback,
        thisSPtr);
}

/// <summary>
/// Helper method for getting the response from a previous batch request to delete the DnsService from the naming store.
/// In addition, this wraps certain error values to success based on whether the Dns name property exists in the store.
/// </summary>
ErrorCode ProcessRolloutContextAsyncOperationBase::EndSubmitDeleteDnsServicePropertyBatch(
    __in AsyncOperationSPtr const & operation,
    __in StringLiteral const & traceComponent,
    __in wstring const & serviceDnsName,
    __in wstring const & serviceName)
{
    Api::IPropertyBatchResultPtr result;
    ErrorCode error = this->Client.EndSubmitPropertyBatch(operation, result);

    if (error.IsSuccess())
    {
        error = result->GetError();
    }

    if (error.IsSuccess() ||
        error.IsError(ErrorCodeValue::PropertyNotFound) ||
        error.IsError(ErrorCodeValue::PropertyCheckFailed))
    {
        WriteInfo(
            traceComponent,
            "{0} successfully deleted Dns name property {1} for service {2} error {3}",
            this->TraceId,
            serviceDnsName,
            serviceName,
            error);

        return ErrorCodeValue::Success;
    }
    else
    {
        WriteWarning(
            traceComponent,
            "{0} failed to delete Dns name property {1} for service {2} error {3}",
            this->TraceId,
            serviceDnsName,
            serviceName,
            error);

        return error;
    }
}

AsyncOperationSPtr ProcessRolloutContextAsyncOperationBase::BeginCreateServiceDnsName(
    wstring const & serviceName,
    wstring const & serviceDnsName,
    Common::ActivityId const & activityId,
    TimeSpan const &timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<CreateServiceDnsNameAsyncOperation>(
        serviceName,
        serviceDnsName,
        *this,
        activityId,
        timeout,
        callback,
        parent);
}

ErrorCode ProcessRolloutContextAsyncOperationBase::EndCreateServiceDnsName(AsyncOperationSPtr const &operation)
{
    return CreateServiceDnsNameAsyncOperation::End(operation);
}

AsyncOperationSPtr ProcessRolloutContextAsyncOperationBase::BeginDeleteServiceDnsName(
    wstring const & serviceName,
    wstring const & serviceDnsName,
    Common::ActivityId const & activityId,
    TimeSpan const &timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<DeleteServiceDnsNameAsyncOperation>(
        serviceName,
        serviceDnsName,
        *this,
        activityId,
        timeout,
        callback,
        parent);
}

ErrorCode ProcessRolloutContextAsyncOperationBase::EndDeleteServiceDnsName(AsyncOperationSPtr const &operation)
{
    return DeleteServiceDnsNameAsyncOperation::End(operation);
}
