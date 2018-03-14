// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define TR_TEST_TRACE_BEGIN(testName) \
        NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem_); \
        ASSERT_IFNOT(NT_SUCCESS(status), "Status not success: {0}", status); \
        underlyingSystem_->SetStrictAllocationChecks(TRUE); \
        KAllocator & allocator = underlyingSystem_->NonPagedAllocator(); \
        Trace.WriteWarning(TraceComponent, "Begin Test Case: {0}", testName); \
        KFinally([&]() { Trace.WriteWarning(TraceComponent, "Finishing Test Case {0}", testName); KtlSystem::Shutdown(); }); \

// Test headers
#include "../testcommon/TestCommon.Public.h"
#include "./testcommon/TransactionalReplicator.TestCommon.Public.h"
#include "TestHealthClient.h"
#include "../../../test/TestHooks/TestHooks.h"
