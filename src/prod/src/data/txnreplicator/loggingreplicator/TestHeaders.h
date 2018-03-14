// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

# pragma once

// Tags for backup tests
#define TEST_LOGRECORDS 'srLT' // TestLogRecordS 
#define TEST_BACKUP_RESTORE_PROVIDER_TAG 'prBT' // TestBackRestoreProvider
#define TEST_REPLICATED_LOG_MANAGER_TAG 'mLRT' // TestReplicatedLogManager
#define TEST_LOG_MANAGER_TAG 'gMLT' // Test Log ManaGer 
#define TEST_PHYSICAL_LOG_READER_TAG 'rLPT' // Test Physical Log Reader

// Helper macro's used by the tests which need a partitioned replica id trace type
#define TEST_TRACE_BEGIN(testName) \
        NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem_); \
        ASSERT_IFNOT(NT_SUCCESS(status), "Status not success: {0}", status); \
        underlyingSystem_->SetStrictAllocationChecks(TRUE); \
        underlyingSystem_->SetDefaultSystemThreadPoolUsage(FALSE); \
        KAllocator & allocator = underlyingSystem_->NonPagedAllocator(); \
        int seed = GetTickCount(); \
        Common::Random r(seed); \
        rId_ = r.Next(); \
        pId_.CreateNew(); \
        prId_ = PartitionedReplicaId::Create(pId_, rId_, allocator); \
        Trace.WriteWarning(TraceComponent, "{0} Begin Test Case: {1} with seed {2}", prId_->TraceId, testName, seed); \
        KFinally([&]() { Trace.WriteWarning(TraceComponent, "{0} Finishing Test Case {1}", prId_->TraceId, testName); EndTest(); KtlSystem::Shutdown(); }); \
        
#include "../../testcommon/TestCommon.Public.h"

#include "ApiFaultUtility.h"
#include "TestOperation.h"
#include "TestCopyStreamConverter.h"
#include "TestStateStream.h"
#include "TestStateReplicator.h"
#include "TestGroupCommitValidationResult.h"
#include "TestOperationProcessor.h"
#include "TestTransaction.h"
#include "TestTransactionGenerator.h"
#include "TestLoggingReplicatorToVersionManager.h"
#include "TestStateProviderManager.h"
#include "TestLogRecordUtility.h"
#include "TestCheckpointManager.h"
#include "TestLogTruncationManager.h"
#include "TestTransactionManager.h"
#include "TestVersionProvider.h"
#include "VersionManagerTestBase.h"

#include "TestLogRecords.h"
#include "TestBackupRestoreProvider.h"
#include "TestReplicatedLogManager.h"
#include "TestBackupCallbackHandler.h"
#include "TestPhysicalLogReader.h"
#include "TestLogManager.h"
#include "TestHealthClient.h"
#include "TestReplica.h"
#include "TestTransactionChangeHandler.h"
#include "TestTransactionReplicator.h"
