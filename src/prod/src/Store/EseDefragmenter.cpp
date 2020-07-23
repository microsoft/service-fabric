// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;

StringLiteral const TraceComponent("Defragmenter");

EseDefragmenter::EseDefragmenter(__in EseLocalStore & eseStore)
    : ComponentRoot()
    , eseStore_(eseStore)
    , settings_(eseStore.Settings)
    , traceId_(eseStore.TraceId)
    , session_()
    , database_()
    , lock_()
    , isCanceled_(false)
    , defragTimer_()
{
}

shared_ptr<EseDefragmenter> EseDefragmenter::CreateAndStart(__in EseLocalStore & eseStore)
{
    auto defragmenter = shared_ptr<EseDefragmenter>(new EseDefragmenter(eseStore));

    defragmenter->Start();

    return defragmenter;
}

EseDefragmenter::~EseDefragmenter()
{
}

void EseDefragmenter::Start()
{
    // Use pending transaction counts to prevent EseLocalStore destruction.
    // Another option would be to convert EseLocalStore into a ComponentRoot,
    // but the transaction count model already exists.
    //
    this->IncrementLocalStoreRefCount();

    this->DoStart();
}

void EseDefragmenter::Cancel()
{
    {
        AcquireWriteLock lock(lock_);

        if (defragTimer_)
        {
            defragTimer_->Cancel();
        }

        isCanceled_ = true;

        WriteInfo(
            TraceComponent,
            "{0} defragmenter canceled",
            this->TraceId);
    }

    // EseLocalStore is free to destruct at this point
    // once any pending callback completes
    //
    this->DecrementLocalStoreRefCount();
}

void EseDefragmenter::DoStart()
{
    TimeSpan maxDefragFrequency;
    {
        AcquireReadLock lock(lock_);

        if (isCanceled_)
        {
            return;
        }

        maxDefragFrequency = settings_.MaxDefragFrequency;

        auto error = this->InitializeSession_ReadLock();
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} initialize defragmentation session+database failed: error={1}",
                this->TraceId,
                error);
        }

        // Do this after initializing ESE session and database so that explicit
        // calls to StartDefragmentation() are still supported.
        //
        if (maxDefragFrequency <= TimeSpan::Zero)
        {
            WriteInfo(
                TraceComponent,
                "{0} auto defragmentation is disabled: MaxDefragFrequency={1}",
                this->TraceId,
                maxDefragFrequency);

            return;
        }

        if (!this->IsJetDbidInSupportedRange())
        {
            WriteInfo(
                TraceComponent,
                "{0} Disabling defragmentation: JET_DBID=[{1}] is > 255",
                this->TraceId,
                database_->DatabaseId);

            return;
        }
    }

    if (!session_)
    {
        this->ScheduleStartRetry();

        return;
    }

    defragTimer_.reset();

    // First defrag triggers immediately upon opening, which
    // will be more or less no-op by ESE if the DB is already
    // defragmented.
    //
    this->ScheduleDefragmentation(TimeSpan::Zero);
}

// Leverages EseLocalStore logic that waits for pending transaction count
// in dtor to keep EseLocalStore alive when there are outstanding callbacks
//
class EseDefragmenter::LocalStoreRoot
{
    DENY_COPY(LocalStoreRoot);

public:

    static shared_ptr<LocalStoreRoot> Create(__in EseDefragmenter & owner)
    {
        return shared_ptr<LocalStoreRoot>(new LocalStoreRoot(owner));
    }

    ~LocalStoreRoot()
    {
        owner_.DecrementLocalStoreRefCount();
    }

private:

    LocalStoreRoot(__in EseDefragmenter & owner) : owner_(owner)
    {
        owner_.IncrementLocalStoreRefCount();
    }

private:

    EseDefragmenter & owner_;
};

void EseDefragmenter::ScheduleStartRetry()
{
    auto root = LocalStoreRoot::Create(*this);
    this->ScheduleTimerCallback(
        [this, root](TimerSPtr const &) { this->DoStart(); },
        TimeSpan::FromSeconds(StoreConfig::GetConfig().DefragRetryDelayInSeconds),
        L"DoStart");
}

void EseDefragmenter::ScheduleDefragmentation(TimeSpan const targetDelay)
{
    auto root = LocalStoreRoot::Create(*this);
    this->ScheduleTimerCallback(
        [this, root](TimerSPtr const &) { this->DefragmentationCallback(); },
        targetDelay,
        L"DefragmentationCallback");
}

void EseDefragmenter::ScheduleDefragmentationRetry()
{
    this->ScheduleDefragmentation(TimeSpan::FromSeconds(StoreConfig::GetConfig().DefragRetryDelayInSeconds));
}

void EseDefragmenter::DefragmentationCallback()
{
    ErrorCode error;
    TimeSpan maxDefragFrequency = TimeSpan::Zero;

    {
        AcquireReadLock lock(lock_);

        if (isCanceled_)
        {
            return;
        }

        maxDefragFrequency = settings_.MaxDefragFrequency;

        size_t dbSizeBytes = 0;
        error = eseStore_.EstimateDbSizeBytes(dbSizeBytes);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} failed to estimate DB size: error={1}",
                this->TraceId,
                error);
        }
        else
        {
            auto defragThresholdMB = settings_.DefragThresholdInMB;

            if (dbSizeBytes / (1024 * 1024) < static_cast<size_t>(defragThresholdMB))
            {
                WriteInfo(
                    TraceComponent,
                    "{0} skipping defragmentation: {1} bytes < threshold={2} MB",
                    this->TraceId,
                    dbSizeBytes,
                    defragThresholdMB);
            }
            else
            {
                WriteInfo(
                    TraceComponent,
                    "{0} starting defragmentation: {1} bytes >= threshold={2} MB",
                    this->TraceId,
                    dbSizeBytes,
                    defragThresholdMB);

                error = this->StartDefragmentation();
            }
        }
    } // isCanceled_ lock

    if (error.IsSuccess())
    {
        this->ScheduleDefragmentation(maxDefragFrequency);
    }
    else
    {
        this->ScheduleDefragmentationRetry();
    }
}

// Automatic defragmentation (triggered by a scan) can also be
// enabled by specifying JET_bitRetrieveHintTableScanForward during table
// creation, but it seems to be less aggressive and does not result
// in as much an improvement as an explicit defragment call using
// JET_bitDefragmentBTree. The latter comes very close to the performance
// achieved by a full offline compaction.
//
ErrorCode EseDefragmenter::StartDefragmentation()
{
    auto sessionId = session_->SessionId;
    auto dbId = database_->DatabaseId;

    if (!this->IsJetDbidInSupportedRange())
    {
        WriteInfo(
            TraceComponent,
            "{0} skipping defragmentation: JET_DBID=[{1}] is > 255",
            this->TraceId,
            dbId);

        return ErrorCodeValue::Success;
    }

    auto jetError = CALL_ESE_NOTHROW(
        JetDefragment2(
            sessionId,
            dbId,
            Constants::TableName->c_str(),
            NULL, // unlimited max # of passes
            NULL, // unlimited max # of seconds
            //
            // Defragmentation callback does not seem to get called (possibly only supported by newer ESE versions).
            // Leave callback here for potential debugging, but do not rely on it.
            //
            DefragmentationCompletedCallback,
            JET_bitDefragmentBTree));

    if (JET_errSuccess != jetError) 
    { 
        return eseStore_.JetToErrorCode(jetError);
    }

    WriteInfo(
        TraceComponent,
        "{0} started database defragmentation: session={1} db={2}",
        this->TraceId,
        sessionId,
        dbId);

    return ErrorCodeValue::Success;
}

ErrorCode EseDefragmenter::StopDefragmentation()
{
    unsigned long passes = 0;
    unsigned long elapsed = 0;

    auto sessionId = session_->SessionId;
    auto dbId = database_->DatabaseId;

    auto jetError = CALL_ESE_NOTHROW(
        JetDefragment2(
            sessionId,
            dbId,
            Constants::TableName->c_str(),
            &passes,
            &elapsed,
            NULL,
            JET_bitDefragmentBatchStop));

    if (JET_wrnDefragNotRunning == jetError)
    {
        WriteInfo(
            TraceComponent,
            "{0} defragmentation already stopped: db={1}",
            this->TraceId,
            dbId);

        jetError = JET_errSuccess;
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} database defragmentation stopped: error={1} passes={2} elapsed={3} db={4}",
            this->TraceId,
            jetError,
            passes,
            elapsed,
            dbId);
    }

    return eseStore_.JetToErrorCode(jetError);
}

void EseDefragmenter::IncrementLocalStoreRefCount()
{
    eseStore_.OnConstructTransaction();
}

void EseDefragmenter::DecrementLocalStoreRefCount()
{
    eseStore_.OnDestructTransaction();
}

ErrorCode EseDefragmenter::InitializeSession_ReadLock()
{
    auto tempSession = EseSession::CreateSPtr(eseStore_.instance_);
    auto jetError = tempSession->Initialize();

    if (JET_errSuccess != jetError)
    {
        WriteInfo(
            TraceComponent,
            "{0} create defragmentation session failed: error={1}",
            this->TraceId,
            jetError);

        return eseStore_.JetToErrorCode(jetError);
    }

    auto tempDatabase = EseDatabase::CreateSPtr(tempSession);
    jetError = tempDatabase->InitializeOpen(Path::Combine(eseStore_.Directory, eseStore_.FileName));

    if (JET_errSuccess != jetError) 
    { 
        WriteInfo(
            TraceComponent,
            "{0} attach defragmentation database failed: error={1}",
            this->TraceId,
            jetError);

        return eseStore_.JetToErrorCode(jetError); 
    }

    session_ = move(tempSession);
    database_ = move(tempDatabase);

    return eseStore_.JetToErrorCode(jetError);
}

bool EseDefragmenter::IsJetDbidInSupportedRange()
{
    return (database_->DatabaseId <= 255);
}

void EseDefragmenter::ScheduleTimerCallback(
    Timer::TimerCallback const & timerCallback, 
    TimeSpan const targetDelay,
    wstring const & debugTag)
{
    {
        AcquireReadLock lock(lock_);

        if (isCanceled_)
        {
            return;
        }

        if (!defragTimer_)
        {
            defragTimer_ = Timer::Create(TimerTagDefault, timerCallback);
        }
    }

    WriteInfo(
        TraceComponent,
        "{0} scheduling {1} in {2}",
        this->TraceId,
        debugTag,
        targetDelay);

    defragTimer_->Change(targetDelay);
}

JET_ERR EseDefragmenter::DefragmentationCompletedCallback(
    JET_SESID sessionId,
    JET_DBID dbId,
    JET_TABLEID tableId,
    JET_CBTYP cbType,
    void *,
    void *,
    void *,
    JET_API_PTR)
{
    Trace.WriteInfo(
        TraceComponent,
        "DefragmentationCompletedCallback: type={0} session={1} db={2} table={3}",
        cbType,
        sessionId,
        dbId,
        tableId);

    return JET_errSuccess;
}
