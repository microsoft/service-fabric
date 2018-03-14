// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace Transport;
    using namespace Federation;
    using namespace std;

    StringLiteral const TraceComponent("RecoverPrimaryAsyncOperation");

    RecoverPrimaryAsyncOperation::RecoverPrimaryAsyncOperation(
        __in StoreService & replica,
        __in NamingStore & store,
        StoreServiceProperties const & properties,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & root)
        : AsyncOperation(callback, root)
        , ActivityTracedComponent(store.TraceId, FabricActivityHeader(Guid::NewGuid()))
        , replica_(replica)
        , store_(store)
        , properties_(properties)
        , retryTimer_()
        , timerLock_()
        , pendingRecoveryOperationCount_(0)
        , pendingRecoveryRequests_()
        , failedRecoveryOperationCount_(0)
    {
        WriteInfo(
            TraceComponent,
            "{0} RecoverPrimaryAsyncOperation::ctor",
            this->TraceId);
    }

    RecoverPrimaryAsyncOperation::~RecoverPrimaryAsyncOperation()
    {
        WriteNoise(
            TraceComponent,
            "{0} RecoverPrimaryAsyncOperation::~dtor",
            this->TraceId);
    }

    void RecoverPrimaryAsyncOperation::End(Common::AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<RecoverPrimaryAsyncOperation>(operation);
        casted->Error.ReadValue();
    }

    void RecoverPrimaryAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        // Start monitoring the duration of the operation until completely started
        properties_.HealthMonitor.AddMonitoredOperation(
            this->ActivityHeader.ActivityId,
            HealthMonitoredOperationName::PrimaryRecoveryStarted,
            L"" /*operationMetadata*/,
            NamingConfig::GetConfig().PrimaryRecoveryStartedHealthDuration);

        // Start monitoring the total duration of the recovery operation
        properties_.HealthMonitor.AddMonitoredOperation(
            this->ActivityHeader.ActivityId,
            HealthMonitoredOperationName::PrimaryRecovery,
            L"" /*operationMetadata*/,
            NamingConfig::GetConfig().NamingServiceProcessOperationHealthDuration);
                
        LONG pendingRequestCount = replica_.PendingRequestCount;
        if (pendingRequestCount > 0)
        {
            WriteInfo(
                TraceComponent,
                "{0} recovery waiting for {1} requests to complete",
                this->TraceId,
                pendingRequestCount);

            // At this point, the recovery operation has already been queued and will prevent
            // new requests from being accepted until the recovery process starts
            //
            this->SchedulePendingRequestCheck(thisSPtr, NamingConfig::GetConfig().OperationRetryInterval);
        }
        else
        {
            this->ScheduleRecovery(thisSPtr, TimeSpan::Zero);
        }
    }

    void RecoverPrimaryAsyncOperation::SchedulePendingRequestCheck(AsyncOperationSPtr const & thisSPtr, TimeSpan const & delay)
    {
        AcquireExclusiveLock lock(timerLock_);

        retryTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const & timer) 
            { 
                timer->Cancel();
                this->OnStart(thisSPtr);
            });

        retryTimer_->Change(delay);
    }

    void RecoverPrimaryAsyncOperation::ScheduleRecovery(AsyncOperationSPtr const & thisSPtr, TimeSpan const & delay)
    {
        AcquireExclusiveLock lock(timerLock_);

        retryTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const & timer) 
            { 
                timer->Cancel();
                this->RecoveryTimerCallback(thisSPtr);
            });
        retryTimer_->Change(delay);
    }

    void RecoverPrimaryAsyncOperation::RecoveryTimerCallback(AsyncOperationSPtr const & thisSPtr)
    {
        this->InitializeRootNames(thisSPtr);
    }

    void RecoverPrimaryAsyncOperation::InitializeRootNames(AsyncOperationSPtr const & thisSPtr)
    {
        // Initialize this partition with the root names if they don't already exist.
        // One for the hierarchy names and one for the non-hierarchy names.
        //
        TransactionSPtr txSPtr;
        auto error = store_.CreateTransaction(this->ActivityHeader, txSPtr);

        if (error.IsSuccess())
        {
            error = store_.TryInsertRootName(
                txSPtr,
                HierarchyNameData(HierarchyNameState::Created, UserServiceState::None, 0), 
                Constants::HierarchyNameEntryType);
        }

        if (error.IsSuccess())
        {
            error = store_.TryInsertRootName(
                txSPtr,
                NameData(UserServiceState::None, 0), 
                Constants::NonHierarchyNameEntryType);
        }

        if (error.IsSuccess())
        {
            auto operation = store_.BeginCommit(
                move(txSPtr),
                TimeSpan::MaxValue,
                [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
                thisSPtr);
            this->OnCommitComplete(operation, true);
        }
        else
        {
            this->HandleError(thisSPtr, error);
        }
    }

    void RecoverPrimaryAsyncOperation::OnCommitComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = store_.EndCommit(operation);

        if (error.IsSuccess())
        {
            this->LoadRecoveryOperations(thisSPtr);
        }
        else
        {
            this->HandleError(thisSPtr, error);
        }
    }

    void RecoverPrimaryAsyncOperation::LoadRecoveryOperations(AsyncOperationSPtr const & thisSPtr)
    {
        pendingRecoveryOperationCount_.store(1);
        pendingRecoveryRequests_.clear();
        failedRecoveryOperationCount_.store(0);

        TransactionSPtr txSPtr;
        ErrorCode error = store_.CreateTransaction(this->ActivityHeader, txSPtr);

        if (error.IsSuccess())
        {
            error = this->LoadPsdCache(txSPtr);
        }

        if (error.IsSuccess())
        {
            error = this->LoadAuthorityOwnerRecoveryOperations(txSPtr);
        }

        NamingStore::CommitReadOnly(move(txSPtr));

        if (error.IsSuccess())
        {
            this->StartRecoveryOperations(thisSPtr);
        }
        else
        {
            this->HandleError(thisSPtr, error);
        }
    }

    ErrorCode RecoverPrimaryAsyncOperation::LoadAuthorityOwnerRecoveryOperations(
        TransactionSPtr const & txSPtr)
    {
        EnumerationSPtr enumSPtr;
        ErrorCode error = store_.CreateEnumeration(
            txSPtr,
            Constants::HierarchyNameEntryType,
            NamingUri::RootNamingUriString,
            enumSPtr);

        if (!error.IsSuccess())
        {
            return error;
        }

        while ((error = enumSPtr->MoveNext()).IsSuccess())
        {
            HierarchyNameData authorityEntry;
            error = store_.ReadCurrentData<HierarchyNameData>(enumSPtr, authorityEntry);

            if (!error.IsSuccess())
            {
                break;
            }

            wstring nameString;
            error = enumSPtr->CurrentKey(nameString);

            if (!error.IsSuccess())
            {
                break;
            }

            NamingUri nameToRecover;
            if (!NamingUri::TryParse(nameString, nameToRecover))
            {
                TRACE_WARNING_AND_TESTASSERT(
                    TraceComponent,
                    "{0} Hierarchy entry '{1}' was not a valid NamingUri for recovery",
                    this->TraceId,
                    nameString);

                continue;
            }

            MessageUPtr recoveryRequest;

            if (authorityEntry.NameState == HierarchyNameState::Creating)
            {
                recoveryRequest = NamingMessage::GetPeerNameCreate(nameToRecover);
            }
            else if (authorityEntry.NameState == HierarchyNameState::Deleting)
            {
                recoveryRequest = NamingMessage::GetPeerNameDelete(nameToRecover);
            }
            else if (authorityEntry.ServiceState == UserServiceState::Creating)
            {
                PartitionedServiceDescriptor psd;
                __int64 unused;
                error = store_.TryReadData(
                    txSPtr,
                    Constants::UserServiceRecoveryDataType,
                    nameToRecover.ToString(),
                    psd,
                    unused);

                if (error.IsError(ErrorCodeValue::NotFound))
                {
                    TRACE_INFO_AND_TESTASSERT(
                        TraceComponent,
                        "{0} service recovery data not found for hierarchy name {1}",
                        this->TraceId,
                        nameToRecover);

                    // skip recovery in production
                    error = ErrorCodeValue::Success;
                }
                else if (!error.IsSuccess())
                {
                    break;
                }

                recoveryRequest = NamingMessage::GetPeerCreateService(CreateServiceMessageBody(nameToRecover, psd));
            }
            else if (authorityEntry.ServiceState == UserServiceState::Updating)
            {
                ServiceUpdateDescription updateDescription;
                __int64 unused;
                error = store_.TryReadData(
                    txSPtr,
                    Constants::UserServiceUpdateDataType,
                    nameToRecover.ToString(),
                    updateDescription,
                    unused);

                if (error.IsError(ErrorCodeValue::NotFound))
                {
                    TRACE_INFO_AND_TESTASSERT(
                        TraceComponent,
                        "{0} service update data not found for hierarchy name {1}",
                        this->TraceId,
                        nameToRecover);

                    // skip recovery in production
                    error = ErrorCodeValue::Success;
                }
                else if (!error.IsSuccess())
                {
                    break;
                }

                recoveryRequest = NamingMessage::GetPeerUpdateService(UpdateServiceRequestBody(nameToRecover, updateDescription));
            }
            else if (UserServiceState::IsDeleting(authorityEntry.ServiceState))
            {
                recoveryRequest = NamingMessage::GetPeerDeleteService(
                    DeleteServiceMessageBody(
                        ServiceModel::DeleteServiceDescription(nameToRecover)));
            }

            if (recoveryRequest)
            {
                WriteInfo(
                    TraceComponent,
                    "{0} recovering operation: name={1} action={2}",
                    this->TraceId,
                    nameToRecover,
                    recoveryRequest->Action);

                recoveryRequest->Headers.Replace(PrimaryRecoveryHeader());
                pendingRecoveryRequests_.push_back(move(recoveryRequest));
            }
        } // end while hierarchy names

        if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            error = ErrorCodeValue::Success;
        }

        return error;
    }

    ErrorCode RecoverPrimaryAsyncOperation::LoadPsdCache(TransactionSPtr const & txSPtr)
    {
        EnumerationSPtr enumSPtr;
        ErrorCode error = store_.CreateEnumeration(
            txSPtr,
            Constants::UserServiceDataType,
            L"",
            enumSPtr);

        if (!error.IsSuccess())
        {
            return error;
        }

        auto & psdCache = properties_.PsdCache;

        while ((error = enumSPtr->MoveNext()).IsSuccess())
        {
            if (psdCache.IsCacheLimitEnabled && psdCache.Size >= psdCache.CacheLimit)
            {
                error = ErrorCodeValue::EnumerationCompleted;

                break;
            }

            PartitionedServiceDescriptor psd;
            error = store_.ReadCurrentData<PartitionedServiceDescriptor>(enumSPtr, psd);

            if (error.IsSuccess())
            {
                FABRIC_SEQUENCE_NUMBER lsn = -1;
                error = enumSPtr->CurrentOperationLSN(lsn);
                
                if (error.IsSuccess())
                {
                    psd.Version = lsn;

                    auto cacheEntry = make_shared<StoreServicePsdCacheEntry>(move(psd));
                    psdCache.UpdateCacheEntry(cacheEntry);
                }
            }
        }

        if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            FABRIC_SEQUENCE_NUMBER lsn = 0;
            error = store_.GetCurrentProgress(lsn);

            if (error.IsSuccess())
            {
                psdCache.UpdateServicesLsn(lsn);
            }
        }

        return error;
    }

    void RecoverPrimaryAsyncOperation::StartRecoveryOperations(AsyncOperationSPtr const & thisSPtr)
    {
        pendingRecoveryOperationCount_ += static_cast<LONG>(pendingRecoveryRequests_.size());

        WriteInfo(
            TraceComponent,
            "{0} Recovering {1} repair operations",
            this->TraceId,
            pendingRecoveryRequests_.size());

        for (auto iter = pendingRecoveryRequests_.begin(); iter != pendingRecoveryRequests_.end(); ++iter)
        {
            MessageUPtr & recoveryRequest = *iter;

            auto operation = replica_.BeginProcessRequest(
                move(recoveryRequest),
                NamingConfig::GetConfig().RepairOperationTimeout,
                [this] (AsyncOperationSPtr const & operation) { this->OnProcessingComplete(operation, false); },
                thisSPtr);
            this->OnProcessingComplete(operation, true);
        }

        // All necessary named locks have been acquired or will be acquired next
        // so it is now safe to start accepting user requests. This also assumes
        // that processing of the recovery operation will not release the named
        // lock until consistency has been achieved.
        //
        // We must accept user requests after starting recovery and acquiring
        // the necessary locks. Otherwise, we will run into distributed deadlocks.
        //
        replica_.RecoverPrimaryStartedOrCancelled(thisSPtr);

        // Since the incoming requests are not blocked anymore, mark the primary recovery as being completely started
        properties_.HealthMonitor.CompleteSuccessfulMonitoredOperation(
            this->ActivityHeader.ActivityId,
            HealthMonitoredOperationName::PrimaryRecoveryStarted,
            L"");

        this->FinishRecoveryOperation(thisSPtr, ErrorCodeValue::Success);
    }

    void RecoverPrimaryAsyncOperation::OnProcessingComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr unusedReply;
        ErrorCode error = replica_.EndProcessRequest(operation, unusedReply);

        this->FinishRecoveryOperation(operation->Parent, error);
    }

    void RecoverPrimaryAsyncOperation::FinishRecoveryOperation(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode const & error)
    {
        if (!error.IsSuccess())
        {
            ++failedRecoveryOperationCount_;
        }

        if (--pendingRecoveryOperationCount_ == 0)
        {
            if (failedRecoveryOperationCount_.load() > 0)
            {
                WriteInfo(
                    TraceComponent,
                    "{0} failed {1} recovery operation(s)",
                    this->TraceId,
                    failedRecoveryOperationCount_.load());

                // Safe to check after RecoverPrimaryStartedOrCancelled is called, which
                // removes this RecoverPrimaryAsyncOperation from the list of pending recoveries.
                //
                if (replica_.ShouldAbortProcessing())
                {
                    WriteInfo(
                        TraceComponent,
                        "{0} aborted recovery",
                        this->TraceId);

                    this->TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
                }
                else
                {
                    this->ScheduleRecovery(thisSPtr, NamingConfig::GetConfig().OperationRetryInterval);
                }
            }
            else
            {
                WriteInfo(
                    TraceComponent,
                    "{0} recovery complete",
                    this->TraceId);

                this->TryComplete(thisSPtr, ErrorCodeValue::Success);
            }
        }
    }

    void RecoverPrimaryAsyncOperation::HandleError(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
    {
        if (store_.IsRepairable(error))
        {
            WriteInfo(
                TraceComponent,
                "{0} failed to process primary recovery: {1}",
                this->TraceId,
                error);

            this->ScheduleRecovery(thisSPtr, NamingConfig::GetConfig().OperationRetryInterval);
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} cancelling recovery due to {1}",
                this->TraceId,
                error);

            replica_.RecoverPrimaryStartedOrCancelled(thisSPtr);
            
            this->TryComplete(thisSPtr, error);
        }
    }

    void RecoverPrimaryAsyncOperation::OnCompleted()
    {
        properties_.HealthMonitor.CompleteMonitoredOperation(
            this->ActivityHeader.ActivityId,
            HealthMonitoredOperationName::PrimaryRecovery,
            L"",
            this->Error,
            false /*keepOperation*/);

        properties_.HealthMonitor.CompleteMonitoredOperation(
            this->ActivityHeader.ActivityId,
            HealthMonitoredOperationName::PrimaryRecoveryStarted,
            L"",
            this->Error,
            false /*keepOperation*/);
    }
}
