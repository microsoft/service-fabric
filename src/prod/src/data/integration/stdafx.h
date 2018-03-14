// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define REPLICA_TAG 'peRT'
#define TEST_BACKUP_CALLBACK_TAG 'bcBT'

// Public headers.
#include "../txnreplicator/TransactionalReplicator.Public.h"
#include "../tstore/Store.Public.h"

#include <SfStatus.h>

// External dependencies
// Test headers
#include "../testcommon/TestCommon.Public.h"
#include "../txnreplicator/testcommon/TransactionalReplicator.TestCommon.Public.h"

// Helper macro's used by the tests which need a partitioned replica id trace type
#define TEST_TRACE_BEGIN(testName) \
        NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem_); \
        ASSERT_IFNOT(NT_SUCCESS(status), "Status not success: {0}", status); \
        underlyingSystem_->SetStrictAllocationChecks(TRUE); \
        underlyingSystem_->SetDefaultSystemThreadPoolUsage(FALSE); \
        int seed = GetTickCount(); \
        Common::Random random(seed); \
        Trace.WriteWarning(TraceComponent, "Begin Test Case: {0} with seed {1}", testName, seed); \
        KFinally([&]() { Trace.WriteWarning(TraceComponent, "Finishing Test Case {0}", testName); EndTest(); KtlSystem::Shutdown(); }); \

namespace Data
{
    namespace Integration
    {
        using ::_delete;
    }
}

#include "TestBackupCallbackHandler.h"
#include "Replica.h"
#include "UpgradeStateProviderFactory.h"
