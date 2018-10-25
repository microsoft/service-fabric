// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// Helper macros used by the tests
#define TEST_TRACE_BEGIN(testName) \
        NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem_); \
        KInvariant(NT_SUCCESS(status)); \
        \
        underlyingSystem_->SetStrictAllocationChecks(TRUE); \
        allocator_ = &underlyingSystem_->NonPagedAllocator(); \
        \
        Common::Random r; \
        rId_ = r.Next(); \
        pId_.CreateNew(); \
        prId_ = Data::Utilities::PartitionedReplicaId::Create(pId_, rId_, *allocator_); \
        \
        TestCommon::TestSession::WriteInfo(TraceComponent, "{0} Begin Test Case: {1}", prId_->TraceId, testName); \
        \
        Common::Stopwatch stopWatch; \
        stopWatch.Start(); \
        BeginTest(); \
        KFinally([&]() \
        { \
            stopWatch.Stop(); \
            TestCommon::TestSession::WriteInfo(TraceComponent, "{0} Finishing Test Case {1} - Runtime: {2}", prId_->TraceId, testName, stopWatch.Elapsed); \
            EndTest(); \
            \
            prId_ = nullptr; \
            \
            underlyingSystem_->Shutdown(); \
            allocator_ = nullptr; \
            underlyingSystem_ = nullptr; \
        });

#define VERIFY_STATUS_SUCCESS(msg, status) \
{ \
    if (!NT_SUCCESS(status)) \
    { \
        TestCommon::TestSession::WriteError(TraceComponent, "Error status: {0}  Msg: {1}", status, msg); \
        VERIFY_FAIL(L"Error status."); \
    } \
}

#include "../../../test/TestCommon/TestCommon.h"

#include "FakeLogManager.h"
#include "FakePhysicalLog.h"
#include "LogTestBase.h"
