// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

std::wstring const SharedLogGuid(L"3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62"); // This is the same as the default application shared log GUID defined in prod\src\ktllogger\constants.cpp
std::wstring const DefaultTestSubDirectoryName(L"KtlLogs");

extern std::wstring GetWorkingDirectory()
{
    std::wstring dir = Common::Directory::GetCurrentDirectoryW();
    dir = Common::Path::Combine(dir, DefaultTestSubDirectoryName);
    return dir;
}

extern void LR_Test_InitializeKtlConfig(
    __in KAllocator & allocator,
    __out KtlLogger::SharedLogSettingsSPtr & sharedLogSettings)
{
    wstring workDir = GetWorkingDirectory();
    auto settings = std::make_unique<KtlLogManager::SharedLogContainerSettings>();
    KString::SPtr sharedLogFileName;
    BOOLEAN res;
    NTSTATUS status;

#if !defined(PLATFORM_UNIX) 
    status = KString::Create(
        sharedLogFileName,
        allocator,
        L"\\??\\");
#else
    status = KString::Create(
        sharedLogFileName,
        allocator);
#endif

    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create sharedLogFileName");

    res = sharedLogFileName->Concat(workDir.c_str());

    ASSERT_IFNOT(
        res,
        "Failed to concatenate workDir {0} with sharedLogFilename {1} due to allocation",
        workDir.c_str(),
        Data::Utilities::ToStringLiteral(*sharedLogFileName));

    res = sharedLogFileName->Concat(KVolumeNamespace::PathSeparator);

    ASSERT_IFNOT(
        res,
        "Failed to concatenate KVolumeNamespace::PathSeparator with sharedLogFilename {0} due to allocation",
        Data::Utilities::ToStringLiteral(*sharedLogFileName));

    res = sharedLogFileName->Concat(L"SharedLog");
    KInvariant(res);

    ASSERT_IFNOT(
        res,
        "Failed to concatenate 'SharedLog' with sharedLogFilename {0} due to allocation",
        Data::Utilities::ToStringLiteral(*sharedLogFileName));

    if (!Common::Directory::Exists(workDir))
    {
        Common::Directory::Create(workDir);
    }

    //  Ensure there is space for the null terminator
    ASSERT_IFNOT(
        sharedLogFileName->LengthInBytes() + sizeof(WCHAR) < 512 * sizeof(WCHAR),
        "sharedLogFileName->LengthInBytes()={0} + sizeof(WCHAR) < 512 * sizeof(WCHAR) = FALSE. sharedLogFileName={1}.",
        sharedLogFileName->LengthInBytes(),
        Data::Utilities::ToStringLiteral(*sharedLogFileName));

    KMemCpySafe(&settings->Path[0], 512 * sizeof(WCHAR), sharedLogFileName->operator PVOID(), sharedLogFileName->LengthInBytes());
    settings->Path[sharedLogFileName->LengthInBytes() / sizeof(WCHAR)] = L'\0'; // set the null terminator
    settings->LogSize = 257 * 1024 * 1024; // shared log lower than this value results in some errors while creating the log

    Common::Guid g;
    bool ret = Common::Guid::TryParse(SharedLogGuid, g);
    ASSERT_IFNOT(ret == true, "Failed to parse shared log id");
    settings->LogContainerId = KtlLogContainerId(KGuid(g.AsGUID()));
    sharedLogSettings = make_shared<KtlLogger::SharedLogSettings>(std::move(settings));
};

using namespace LoggingReplicatorTests;

using namespace std;
using namespace ktl;
using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;
using namespace Common;
using namespace Data::TestCommon;

#define TEST_REPLICA_TAG 'peRT'
StringLiteral const TraceComponent = "TestReplica";
wstring const TestLogFolder = L"log";

TestReplica::TestReplica(
    __in KGuid & pId,
    __in InvalidLogRecords & invalidLogRecords,
    __in bool isPrimary,
    __in_opt TestStateProviderManager * stateManager,
    __in LoggingReplicatorTests::ApiFaultUtility & apiFaultUtility,
    __in KAllocator & allocator)
    : pId_(pId)
    , rId_(FABRIC_INVALID_REPLICA_ID)
    , isPrimary_(isPrimary)
    , testStateManager_(stateManager)
    , allocator_(&allocator)
    , invalidLogRecords_(&invalidLogRecords)
    , apiFaultUtility_(&apiFaultUtility)
    , useFileLogManager_(false)
    , assertInFlushCallback_(true)
    , dedicatedLogPath_(nullptr)
    , healthClient_(TestHealthClient::Create())
{
    physicalCallbackCount_.store(0);
    logicalCallbackCount_.store(0);
    runtimeFolders_ = TestRuntimeFolders::Create(GetWorkingDirectory().c_str(), GetThisAllocator()).RawPtr();
}

TestReplica::~TestReplica()
{
}

TestReplica::SPtr TestReplica::Create(
    __in KGuid & pId,
    __in InvalidLogRecords & invalidLogRecords,
    __in bool isPrimary,
    __in_opt TestStateProviderManager * stateManager,
    __in LoggingReplicatorTests::ApiFaultUtility & apiFaultUtility,
    __in KAllocator & allocator)
{
    TestReplica * value = _new(TEST_REPLICA_TAG, allocator)TestReplica(pId, invalidLogRecords, isPrimary, stateManager, apiFaultUtility, allocator);
    THROW_ON_ALLOCATION_FAILURE(value);

    return TestReplica::SPtr(value);
}

Common::ErrorCode TestReplica::DeleteWorkingDirectory()
{
    // Delete working directory to avoid leaks into shared log disk space
    std::wstring workingDir = GetWorkingDirectory();

    Trace.WriteInfo(
        TraceComponent,
        "Removing {0}",
        workingDir);

    return Common::Directory::Delete_WithRetry(workingDir, true, false);
}

TxnReplicator::IRuntimeFolders::CSPtr TestReplica::GetRuntimeFolders()
{
    return runtimeFolders_;
}

void TestReplica::DelayApis()
{
    apiFaultUtility_->DelayApi(ApiName::ReplicateAsync, Common::TimeSpan::FromMilliseconds(3));
    apiFaultUtility_->DelayApi(ApiName::PrepareCheckpoint, Common::TimeSpan::FromMilliseconds(20));
    apiFaultUtility_->DelayApi(ApiName::PerformCheckpoint, Common::TimeSpan::FromMilliseconds(20));
    apiFaultUtility_->DelayApi(ApiName::CompleteCheckpoint, Common::TimeSpan::FromMilliseconds(20));
    apiFaultUtility_->DelayApi(ApiName::WaitForLogFlushUptoLsn, Common::TimeSpan::FromMilliseconds(10));
}

void TestReplica::Initialize(
    __in int seed)
{
    SyncAwait(InitializeAsync(
        seed,
        false,
        true));
}

void TestReplica::Initialize(
    __in int seed, 
    __in bool skipChangeRoleToPrimary)
{
    Initialize(seed, skipChangeRoleToPrimary, true);
}

void TestReplica::Initialize(
    __in int seed, 
    __in bool skipChangeRoleToPrimary, 
    __in bool delayApis,
    __in bool useTestLogTruncationManager,
    __in TRANSACTIONAL_REPLICATOR_SETTINGS * publicSettings)
{
    SyncAwait(InitializeAsync(
        seed,
        skipChangeRoleToPrimary,
        delayApis,
        useTestLogTruncationManager,
        publicSettings));
}

Awaitable<void> TestReplica::InitializeAsync(
    __in int seed, 
    __in bool skipChangeRoleToPrimary, 
    __in bool delayApis,
    __in bool useTestLogTruncationManager,
    __in TRANSACTIONAL_REPLICATOR_SETTINGS * publicSettings)
{
    KShared$ApiEntry();

    KAllocator & allocator = *allocator_;

    if (rId_ == FABRIC_INVALID_REPLICA_ID) {
        Common::Random r(seed);
        rId_ = r.Next();
    }

    prId_ = PartitionedReplicaId::Create(pId_, rId_, allocator);

    if (delayApis)
    {
        DelayApis();
    }

    perfCounters_ = TRPerformanceCounters::CreateInstance(
        prId_->TracePartitionId.ToString(),
        prId_->ReplicaId);

    TransactionalReplicatorSettingsUPtr tmp = nullptr;

    if (publicSettings != nullptr)
    {
        auto error = TransactionalReplicatorSettings::FromPublicApi(*publicSettings, tmp);
        VERIFY_IS_TRUE(error.IsSuccess());
    }
    
    config_ = TRInternalSettings::Create(move(tmp), make_shared<TransactionalReplicatorConfig>());

    testPartition_ = TestStatefulServicePartition::Create(allocator);
    recoveredOrCopiedCheckpointState_ = RecoveredOrCopiedCheckpointState::Create(allocator);
    recoveredOrCopiedCheckpointState_->Update(0);

    roleContextDrainState_ = RoleContextDrainState::Create(*testPartition_, allocator);

    //state manager could be passed in for secondary test replica (from primary). hence the null check to avoid re-instantiating
    if (testStateManager_ == nullptr) 
    {
        testStateManager_ = TestStateProviderManager::Create(*apiFaultUtility_, allocator);
    }

    versionManager_ = TestLoggingReplicatorToVersionManager::Create(allocator);
    transactionMap_ = TransactionMap::Create(*prId_, allocator);
    testStateReplicator_ = TestStateReplicator::Create(*apiFaultUtility_, allocator);
    TestTransactionReplicator::SPtr txnReplicator = TestTransactionReplicator::Create(allocator);

    co_await CreateLogManager();
    replicatedLogManager_ = ReplicatedLogManager::Create(
        *prId_,
        *logManager_,
        *recoveredOrCopiedCheckpointState_,
        nullptr,
        *roleContextDrainState_,
        *testStateReplicator_,
        *invalidLogRecords_,
        allocator);

    // Using random folder since it will not be used.
    KString::CSPtr mockWorkFolder = CreateFileName(L"IntegrationTests", allocator);

    backupManager_ = BackupManager::Create(
        *prId_,
        *mockWorkFolder,
        *testStateManager_,
        *replicatedLogManager_,
        *logManager_,
        config_,
        healthClient_,
        *invalidLogRecords_,
        allocator);

    if (useTestLogTruncationManager)
    {
        testLogTruncationManager_ = TestLogTruncationManager::Create(seed, *replicatedLogManager_, allocator);

        checkpointManager_ = CheckpointManager::Create(
            *prId_,
            *testLogTruncationManager_,
            *recoveredOrCopiedCheckpointState_,
            *replicatedLogManager_,
            *transactionMap_,
            *testStateManager_,
            *backupManager_,
            config_,
            *invalidLogRecords_,
            perfCounters_,
            healthClient_,
            allocator);
    }
    else
    {
        logTruncationManager_ = LogTruncationManager::Create(*prId_, *replicatedLogManager_, config_, allocator);

        checkpointManager_ = CheckpointManager::Create(
            *prId_,
            *logTruncationManager_,
            *recoveredOrCopiedCheckpointState_,
            *replicatedLogManager_,
            *transactionMap_,
            *testStateManager_,
            *backupManager_,
            config_,
            *invalidLogRecords_,
            perfCounters_,
            healthClient_,
            allocator);
    }

    checkpointManager_->CompletedRecordsProcessor = *this;

    operationProcessor_ = OperationProcessor::Create(
        *prId_,
        *recoveredOrCopiedCheckpointState_,
        *roleContextDrainState_,
        *versionManager_,
        *checkpointManager_,
        *testStateManager_,
        *backupManager_,
        *invalidLogRecords_,
        config_,
        *txnReplicator,
        allocator);

    if (Common::DateTime::Now().Ticks % 2 == 0)
    {
        recordsDispatcher_ = SerialLogRecordsDispatcher::Create(*prId_, *operationProcessor_, config_, allocator);
    }
    else 
    {
        recordsDispatcher_ = ParallelLogRecordsDispatcher::Create(*prId_, *operationProcessor_, config_, allocator);
    }

    ReplicatedLogManager::AppendCheckpointCallback callback(checkpointManager_.RawPtr(), &CheckpointManager::CheckpointIfNecessary);
    replicatedLogManager_->SetCheckpointCallback(callback);

    transactionManager_ = TransactionManager::Create(
        *prId_,
        *recoveredOrCopiedCheckpointState_,
        *transactionMap_,
        *checkpointManager_,
        *replicatedLogManager_,
        *operationProcessor_,
        *invalidLogRecords_,
        config_,
        perfCounters_,
        allocator);

    testTransactionManager_ = TestTransactionManager::Create(*transactionManager_, allocator);
    testBackupRestoreProvider_ = TestBackupRestoreProvider::Create(allocator);
    backupManager_->Initialize(*testBackupRestoreProvider_, *checkpointManager_, *operationProcessor_);

    recoveryManager_ = RecoveryManager::Create(
        *prId_,
        *logManager_,
        *callbackManager_,
        *invalidLogRecords_,
        allocator);

    RecoveryPhysicalLogReader::SPtr recoveryLogReader;
    NTSTATUS status = co_await logManager_->OpenAsync(recoveryLogReader);

    THROW_ON_FAILURE(status);

    auto recoveryInfo = co_await recoveryManager_->OpenAsync(
        false,
        *recoveryLogReader,
        false);

    roleContextDrainState_->OnRecovery();

    status = co_await recoveryManager_->PerformRecoveryAsync(
        *recoveredOrCopiedCheckpointState_,
        *operationProcessor_,
        *checkpointManager_,
        *transactionManager_,
        *recordsDispatcher_,
        *replicatedLogManager_,
        1024,
        healthClient_,
        config_);

    THROW_ON_FAILURE(status);

    roleContextDrainState_->OnRecoveryCompleted();

    if (!skipChangeRoleToPrimary)
    {
        roleContextDrainState_->ChangeRole(isPrimary_ ? FABRIC_REPLICA_ROLE_PRIMARY : FABRIC_REPLICA_ROLE_IDLE_SECONDARY);

        recoveryLogReader->Dispose();
    }

    co_return;
}

Awaitable<void> TestReplica::CreateLogManager()
{
    KShared$ApiEntry();

    KAllocator & allocator = *allocator_;

    // Create the LogManager from the Data::Log namespace
    // This will be shared among a single application host
    Data::Log::LogManager::Create(
        allocator,
        dataLogManager_);

    KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
    LR_Test_InitializeKtlConfig(
        allocator,
        sharedLogSettings);

    if (dedicatedLogPath_ == nullptr)
    {
        dedicatedLogPath_ = KString::Create(GetWorkingDirectory().c_str(), allocator);
    }

    CODING_ERROR_ASSERT(config_ != nullptr);

    if (useFileLogManager_)
    {
        logManager_ = FileLogManager::Create(
            *prId_,
            perfCounters_,
            healthClient_,
            config_,
            allocator).RawPtr();
    }
    else
    {
        KTLLOGGER_SHARED_LOG_SETTINGS slSettings = { L"", L"", 0, 0, 0 };

        KtlLoggerSharedLogSettingsUPtr tmp2;
        KtlLoggerSharedLogSettings::FromPublicApi(slSettings, tmp2);

        TxnReplicator::SLInternalSettingsSPtr slConfig = SLInternalSettings::Create(move(tmp2));

        logManager_ = KLogManager::Create(
            *prId_,
            *runtimeFolders_,
            *dataLogManager_,
            config_,
            slConfig,
            perfCounters_,
            healthClient_,
            allocator).RawPtr();

        NTSTATUS status = co_await dataLogManager_->OpenAsync(CancellationToken::None, sharedLogSettings);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
    }

    CODING_ERROR_ASSERT(logManager_ != nullptr);
    co_await logManager_->InitializeAsync(*dedicatedLogPath_);

    callbackManager_ = PhysicalLogWriterCallbackManager::Create(
        *prId_,
        allocator);
    callbackManager_->FlushCallbackProcessor = *this;

    CODING_ERROR_ASSERT(callbackManager_ != nullptr);
}

void TestReplica::ProcessFlushedRecordsCallback(LoggedRecords const & loggedRecords) noexcept
{
    NTSTATUS status = loggedRecords.LogError;

    for (ULONG i = 0; i < loggedRecords.Count; i++)
    {
        if (status == STATUS_SUCCESS)
        {
            Trace.WriteInfo(TraceComponent, "{0} Flushed Psn: {1}", prId_->TraceId, loggedRecords[i]->Psn);
        }
        else
        {
            Trace.WriteWarning(TraceComponent, "{0} Psn: {1} failed to flush with status: {2}", prId_->TraceId, loggedRecords[i]->Psn, loggedRecords.LogError);

            operationProcessor_->ProcessLogError(
                L"ProcessFlushedRecordsCallback",
                *loggedRecords[i],
                status);
        }

        LogicalLogRecord * logicalRecord = loggedRecords[i]->AsLogicalLogRecord();

        if (logicalRecord != nullptr)
        {
            logicalRecord->ReleaseSerializedLogicalDataBuffers();
        }

        loggedRecords[i]->CompletedFlush(status);
    }

    recordsDispatcher_->DispatchLoggedRecords(loggedRecords);
}

void TestReplica::ProcessedLogicalRecord(__in LogicalLogRecord & logicalRecord) noexcept
{
    operationProcessor_->ProcessedLogicalRecord(logicalRecord);
}

void TestReplica::ProcessedPhysicalRecord(__in PhysicalLogRecord & physicalRecord) noexcept
{
    operationProcessor_->ProcessedPhysicalRecord(physicalRecord);
}

Awaitable<void> TestReplica::CloseAndQuiesceReplica()
{
    KShared$ApiEntry();

    roleContextDrainState_->OnClosing();

    co_await replicatedLogManager_->FlushInformationRecordAsync(
        InformationEvent::Enum::Closed,
        true,
        L"CloseAndQuiesceReplica");

    // Cancel checkpoint timer
    checkpointManager_->CancelPeriodicCheckpointTimer();

    co_await replicatedLogManager_->LastInformationRecord->AwaitProcessing();

    co_await operationProcessor_->WaitForAllRecordsProcessingAsync();

    co_await replicatedLogManager_->InnerLogManager->CloseLogAsync();
    co_return;
}

void TestReplica::EndTest(__in bool reset)
{
    EndTest(reset, false);
}

void TestReplica::EndTest(bool reset, bool cleanupLog)
{
    SyncAwait(EndTestAsync(reset, cleanupLog));
}

Awaitable<void> TestReplica::EndTestAsync(bool reset, bool cleanupLog)
{
    KShared$ApiEntry();

    recoveryManager_->DisposeRecoveryReadStreamIfNeeded();
    if (cleanupLog)
    {
        // Close/Delete the KTLLogManager in Data::LoggingReplicator
        co_await logManager_->DeleteLogAsync();
        logManager_->Dispose();
    }
    else
    {
        co_await logManager_->CloseLogAsync();
        logManager_->Dispose();
    }

    // Close Data::Log::LogManager if enabled
    if (!useFileLogManager_)
    {
        NTSTATUS status = co_await dataLogManager_->CloseAsync(CancellationToken::None);
        VERIFY_ARE_EQUAL(status, STATUS_SUCCESS);
    }

    prId_.Reset();
    dataLogManager_.Reset();

    if (reset)
    {
        recoveryManager_.Reset();
        recoveredOrCopiedCheckpointState_.Reset();
        roleContextDrainState_.Reset();
        testStateManager_.Reset();
        versionManager_.Reset();
        testBackupRestoreProvider_.Reset();
        backupManager_.Reset();
        checkpointManager_.Reset();
        operationProcessor_.Reset();
        recordsDispatcher_.Reset();
        replicatedLogManager_.Reset();
        logManager_.Reset();
        callbackManager_->Dispose();
        callbackManager_.Reset();
        transactionMap_.Reset();
        testLogTruncationManager_.Reset();
        logTruncationManager_.Reset();
        testStateReplicator_.Reset();
        transactionManager_.Reset();
        testTransactionManager_.Reset();
        testPartition_.Reset();
    }

    co_return;
}

CopyContext::SPtr TestReplica::CreateACopyContextWReplicaProgressVector()
{
    return CreateACopyContext(
        *ReplicatedLogManager->ProgressVectorValue,
        ReplicatedLogManager->CurrentLogTailLsn,
        Constants::ZeroLsn,
        ReplicatedLogManager->CurrentLogHeadRecord->Lsn);
}

CopyContext::SPtr TestReplica::CreateACopyContext(
    __in ProgressVector & progressVector,
    __in LONG64 logTailLsn,
    __in LONG64 latestRecoveredAtomicRedoOperationLsn,
    __in LONG64 logHeadRecordLsn)
{
    KAllocator & allocator = *allocator_;
    TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

    IndexingLogRecord::CSPtr expectedLogHeadRecord = IndexingLogRecord::Create(
        LogRecordType::Enum::Indexing,
        10,
        logHeadRecordLsn,
        *util->InvalidRecords->Inv_PhysicalLogRecord,
        allocator).RawPtr();

    return CopyContext::Create(
        *prId_,
        PartitionId->ReplicaId,
        progressVector,
        *expectedLogHeadRecord,
        logTailLsn,
        latestRecoveredAtomicRedoOperationLsn,
        allocator);
}

CopyContext::SPtr TestReplica::CreateACopyContext()
{
    DateTime timeStamp;
    return CreateACopyContext(
        PartitionId->ReplicaId,
        ReplicatedLogManager->CurrentLogTailLsn,
        Constants::ZeroLsn,
        ReplicatedLogManager->CurrentLogTailEpoch.DataLossVersion,
        ReplicatedLogManager->CurrentLogTailEpoch.ConfigurationVersion,
        Constants::ZeroLsn,
        ReplicatedLogManager->CurrentLogHeadRecord->Lsn,
        timeStamp);
}

CopyContext::SPtr TestReplica::CreateACopyContext(
    __in LONG64 replicaId,
    __in LONG64 logTailLsn,
    __in LONG64 latestRecoveredAtomicRedoOperationLsn,
    __in LONG64 epochDataLossNumber,
    __in LONG64 epochConfigurationNumber,
    __in LONG64 startingLogicalSequenceNumber,
    __in LONG64 logHeadRecordLsn,
    __out Common::DateTime & timeStamp)
{
    ProgressVector::SPtr expectedProgressVector = ProgressVector::Create(*allocator_);
    ProgressVectorEntry e1(Epoch(epochDataLossNumber, epochConfigurationNumber), startingLogicalSequenceNumber, replicaId, timeStamp);
    expectedProgressVector->Insert(e1);

    return CreateACopyContext(*expectedProgressVector, logTailLsn, latestRecoveredAtomicRedoOperationLsn, logHeadRecordLsn);
}
