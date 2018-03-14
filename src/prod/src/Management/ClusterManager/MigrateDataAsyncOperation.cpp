// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("MigrateDataAsyncOperation");

// This class handles migration of data between versions of WinFabric if needed.
//
// The current implementation migrates between V1 and V2 according to the following changes:
//
// 1) In V2, application and service manifests are now persisted on the CM in
//    StoreDataApplicationManifest and StoreDataServiceManifest data types.
//
// 2) In V2, application health entities are now reported by the CM to the HM
//    and persisted by the HM during application creation.
//
MigrateDataAsyncOperation::MigrateDataAsyncOperation(
    __in RolloutManager & rolloutManager,
    Common::ActivityId const & activityId,
    vector<ApplicationTypeContext> && recoveredApplicationTypeContexts,
    vector<ApplicationContext> && recoveredApplicationContexts,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ProcessRolloutContextAsyncOperationBase(
        rolloutManager,
        Store::ReplicaActivityId(rolloutManager.Replica.PartitionedReplicaId, activityId),
        timeout,
        callback, 
        root)
    , timeout_(timeout)
    , error_(ErrorCodeValue::Success)
    , pendingMigrations_(0)
    , appManifestData_()
    , serviceManifestData_()
    , applicationTypesToMigrate_(move(recoveredApplicationTypeContexts))
    , applicationsToMigrate_(move(recoveredApplicationContexts))
{
}

void MigrateDataAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->MigrateApplicationTypes(thisSPtr);
}

ErrorCode MigrateDataAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<MigrateDataAsyncOperation>(operation)->Error;
}

void MigrateDataAsyncOperation::MigrateApplicationTypes(AsyncOperationSPtr const & thisSPtr)
{
    if (applicationTypesToMigrate_.empty())
    {
        auto storeTx = this->CreateTransaction();

        // Retry on timeout will reset the list so reload
        auto error = storeTx.ReadPrefix(Constants::StoreType_ApplicationTypeContext, L"", applicationTypesToMigrate_);

        if (!error.IsSuccess())
        {
            this->ScheduleMigrateApplicationTypesOrComplete(thisSPtr, error);
            return;
        }
    }

    applicationTypesToMigrate_.erase(remove_if(applicationTypesToMigrate_.begin(), applicationTypesToMigrate_.end(),
        [this](ApplicationTypeContext const & context) { return (context.ManifestDataInStore || !context.IsComplete); }),
        applicationTypesToMigrate_.end());

    if (applicationTypesToMigrate_.empty())
    {
        WriteInfo(TraceComponent, "{0} no application types to migrate", this->TraceId);

        this->MigrateApplications(thisSPtr);
    }
    else
    {
        WriteInfo(TraceComponent, "{0} migrating {1} application types", this->TraceId, applicationTypesToMigrate_.size());

        appManifestData_.clear();
        serviceManifestData_.clear();

        auto error = this->Replica.ImageBuilder.BuildManifestContexts(
            applicationTypesToMigrate_,
            this->RemainingTimeout,
            appManifestData_,
            serviceManifestData_);

        if (error.IsSuccess())
        {
            if (appManifestData_.size() != applicationTypesToMigrate_.size())
            {
                Assert::TestAssert("Application Manifest count mismatch");
                error = ErrorCodeValue::OperationFailed;
            }

            if (error.IsSuccess() && serviceManifestData_.size() != applicationTypesToMigrate_.size())
            {
                Assert::TestAssert("Service Manifest count mismatch");
                error = ErrorCodeValue::OperationFailed;
            }
        }

        if (error.IsSuccess())
        {
            pendingMigrations_.store(static_cast<LONG>(applicationTypesToMigrate_.size()));

            for (size_t ix = 0; ix < applicationTypesToMigrate_.size(); ++ix)
            {
                auto storeTx = this->CreateTransaction();

                applicationTypesToMigrate_[ix].ManifestDataInStore = true;

                storeTx.Update(applicationTypesToMigrate_[ix]);
                storeTx.UpdateOrInsertIfNotFound(appManifestData_[ix]);
                storeTx.UpdateOrInsertIfNotFound(serviceManifestData_[ix]);

                auto operation = StoreTransaction::BeginCommit(
                    move(storeTx),
                    applicationTypesToMigrate_[ix],
                    [this, ix](AsyncOperationSPtr const & operation) { this->OnCommitManifestsComplete(ix, operation, false); },
                    thisSPtr);
                this->OnCommitManifestsComplete(ix, operation, true);
            }
        }
        else
        {
            this->ScheduleMigrateApplicationTypesOrComplete(thisSPtr, error);
        }
    }
}

void MigrateDataAsyncOperation::OnCommitManifestsComplete(size_t index, AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = StoreTransaction::EndCommit(operation);

    WriteInfo(
        TraceComponent, 
        "{0} migration of '{1}:{2}' completed with {3}", 
        this->TraceId, 
        applicationTypesToMigrate_[index].TypeName, 
        applicationTypesToMigrate_[index].TypeVersion, 
        error);

    error_ = ErrorCode::FirstError(error_, error);

    if (--pendingMigrations_ == 0)
    {
        if (error_.IsSuccess())
        {
            this->MigrateApplications(thisSPtr);
        }
        else
        {
            this->ScheduleMigrateApplicationTypesOrComplete(thisSPtr, error_);
        }
    }
}

void MigrateDataAsyncOperation::MigrateApplications(AsyncOperationSPtr const & thisSPtr)
{
    if (applicationsToMigrate_.empty())
    {
        // Retry on timeout will reset the list so reload
        auto error = this->Replica.GetCompletedOrUpgradingApplicationContexts(applicationsToMigrate_, false); // isQuery

        if (!error.IsSuccess())
        {
            this->ScheduleMigrateApplicationsOrComplete(thisSPtr, error);
            return;
        }
    }

    applicationsToMigrate_.erase(remove_if(applicationsToMigrate_.begin(), applicationsToMigrate_.end(),
        [this](ApplicationContext const & context) -> bool { return (context.HasReportedHealthEntity || (!context.IsComplete && !context.IsUpgrading)); }),
        applicationsToMigrate_.end());

    if (applicationsToMigrate_.empty())
    {
        WriteInfo(TraceComponent, "{0} no applications to migrate", this->TraceId);

        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
    else
    {
        WriteInfo(TraceComponent, "{0} migrating {1} applications", this->TraceId, applicationsToMigrate_.size());

        pendingMigrations_.store(static_cast<LONG>(applicationsToMigrate_.size()));

        for (auto it = applicationsToMigrate_.begin(); it != applicationsToMigrate_.end(); ++it)
        {
            this->ReportApplicationPolicy(TraceComponent, thisSPtr, *it);
        }
    }
}

void MigrateDataAsyncOperation::OnReportApplicationPolicyComplete(__in ApplicationContext & context, AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
{
    if (error.IsSuccess())
    {
        auto storeTx = this->CreateTransaction();

        context.SetHasReportedHealthEntity();

        storeTx.Update(context);

        auto operation = StoreTransaction::BeginCommit(
            move(storeTx),
            context,
            [this, &context](AsyncOperationSPtr const & operation) { this->OnCommitApplicationContextComplete(context, operation, false); },
            thisSPtr);
        this->OnCommitApplicationContextComplete(context, operation, true);
    }
    else
    {
        this->FinishMigrateApplications(context, thisSPtr, error);
    }
}

void MigrateDataAsyncOperation::OnCommitApplicationContextComplete(__in ApplicationContext & context, AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = StoreTransaction::EndCommit(operation);

    this->FinishMigrateApplications(context, operation->Parent, error);
}

void MigrateDataAsyncOperation::FinishMigrateApplications(__in ApplicationContext & context, AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
{
    WriteInfo(TraceComponent, "{0} migration of '{1}' completed with {2}", this->TraceId, context.ApplicationName, error);

    error_ = ErrorCode::FirstError(error_, error);

    if (--pendingMigrations_ == 0)
    {
        this->ScheduleMigrateApplicationsOrComplete(thisSPtr, error_);
    }
}

void MigrateDataAsyncOperation::ScheduleMigrateApplicationTypesOrComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
{
    applicationTypesToMigrate_.clear();

    if (this->TryPrepareForRetry(error))
    {
        WriteInfo(TraceComponent, "{0} retry application type migration with new timeout = {1}", this->TraceId, this->RemainingTimeout);

        AcquireExclusiveLock lock(timerLock_);

        retryTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const & timer)
            {
                timer->Cancel();
                this->MigrateApplicationTypes(thisSPtr);
            },
            true); // allow concurrency
        retryTimer_->Change(ManagementConfig::GetConfig().MaxOperationRetryDelay);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void MigrateDataAsyncOperation::ScheduleMigrateApplicationsOrComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
{
    applicationsToMigrate_.clear();

    if (this->TryPrepareForRetry(error))
    {
        WriteInfo(TraceComponent, "{0} retry application migration with new timeout = {1}", this->TraceId, this->RemainingTimeout);

        AcquireExclusiveLock lock(timerLock_);

        retryTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const & timer)
            {
                timer->Cancel();
                this->MigrateApplications(thisSPtr);
            },
            true); // allow concurrency
        retryTimer_->Change(ManagementConfig::GetConfig().MaxOperationRetryDelay);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

bool MigrateDataAsyncOperation::TryPrepareForRetry(ErrorCode const & error)
{
    switch (error.ReadValue())
    {
        case ErrorCodeValue::Timeout:
            // Do not typically expect huge timeout requirements, so increase geometrically
            timeout_ = timeout_.AddWithMaxAndMinValueCheck(ManagementConfig::GetConfig().MinOperationTimeout);

            // Account for scheduling delay
            timeout_ = timeout_.AddWithMaxAndMinValueCheck(ManagementConfig::GetConfig().MaxOperationRetryDelay);

            if (timeout_ > ManagementConfig::GetConfig().MaxDataMigrationTimeout)
            {
                timeout_ = ManagementConfig::GetConfig().MaxDataMigrationTimeout;
            }

            // intentional fall-through
            
        case ErrorCodeValue::StoreWriteConflict:
        case ErrorCodeValue::StoreSequenceNumberCheckFailed:
            this->ResetTimeout(timeout_);
            error_.ReadValue();
            error_.Reset(ErrorCodeValue::Success);

            return true;

        default:
            return false;
    }
}
