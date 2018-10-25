// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define TESTFILELOGICALLOG_TAG 'xLLF'

namespace LogTests
{
    using namespace std;
    using namespace ktl;
    using namespace Data::Log;
    using namespace Data::Utilities;

    Common::StringLiteral const TraceComponent("FileLogicalLogTest");

    class FileLogicalLogTest : public LogTestBase
    {
        K_FORCE_SHARED(FileLogicalLogTest)

    public:

        KDeclareDefaultCreate(FileLogicalLogTest, context, TESTFILELOGICALLOG_TAG);

        NTSTATUS CreateAndOpenLogManager(__out ILogManagerHandle::SPtr& logManager) override
        {
            NTSTATUS status;

            FakeLogManager::SPtr fakeManager;
            status = FakeLogManager::Create(fakeManager, GetThisAllocator());
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            logManager = ILogManagerHandle::SPtr(fakeManager.RawPtr());
            return STATUS_SUCCESS;
        }

        NTSTATUS CreateDefaultPhysicalLog(
            __in ILogManagerHandle&,
            __out IPhysicalLogHandle::SPtr& physicalLog) override
        {
            NTSTATUS status;

            FakePhysicalLog::SPtr fakeLog;
            status = FakePhysicalLog::Create(fakeLog, GetThisAllocator());
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            physicalLog = IPhysicalLogHandle::SPtr(fakeLog.RawPtr());

            return STATUS_SUCCESS;
        }

        NTSTATUS CreatePhysicalLog(
            __in ILogManagerHandle& logManager,
            __in KGuid const &,
            __in KString const &,
            __out IPhysicalLogHandle::SPtr& physicalLog) override
        {
            return CreateDefaultPhysicalLog(logManager, physicalLog);
        }

        NTSTATUS OpenDefaultPhysicalLog(
            __in ILogManagerHandle&,
            __out IPhysicalLogHandle::SPtr& physicalLog) override
        {
            NTSTATUS status;

            FakePhysicalLog::SPtr fakeLog;
            status = FakePhysicalLog::Create(fakeLog, GetThisAllocator());
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            physicalLog = IPhysicalLogHandle::SPtr(fakeLog.RawPtr());
            return STATUS_SUCCESS;
        }

        NTSTATUS OpenPhysicalLog(
            __in ILogManagerHandle& logManager,
            __in KGuid const &,
            __in KString const &,
            __out IPhysicalLogHandle::SPtr& physicalLog) override
        {
            return OpenDefaultPhysicalLog(logManager, physicalLog);
        }

        NTSTATUS CreateLogicalLog(
            __in IPhysicalLogHandle& physicalLog,
            __in KGuid const & id,
            __in KString const & path,
            __out ILogicalLog::SPtr& logicalLog) override
        {
            NTSTATUS status;

            if (!firstCreate_)
            {
                return STATUS_OBJECT_NAME_COLLISION;
            }
            firstCreate_ = FALSE;

            KWString filename(GetThisAllocator(), path);
            status = filename.Status();
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            KSynchronizer sync;
            KVolumeNamespace::DeleteFileOrDirectory(filename, GetThisAllocator(), sync.AsyncCompletionCallback());
            status = sync.WaitForCompletion();
            // todo?: trace status

            return OpenLogicalLog(physicalLog, id, path, logicalLog);
        }

        NTSTATUS OpenLogicalLog(
            __in IPhysicalLogHandle&,
            __in KGuid const &,
            __in KString const & path,
            __out ILogicalLog::SPtr& logicalLog) override
        {
            NTSTATUS status;
            FileLogicalLog::SPtr fileLogicalLog;

            status = FileLogicalLog::Create(GetThisAllocator(), fileLogicalLog);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            KWString filename(GetThisAllocator(), path);
            status = filename.Status();
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            status = SyncAwait(fileLogicalLog->OpenAsync(filename, CancellationToken::None));
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            logicalLog = ILogicalLog::SPtr(fileLogicalLog.RawPtr());

            return STATUS_SUCCESS;
        }

        VOID VerifyState(
            __in_opt LogManager* logManager = nullptr,
            __in int logManagerHandlesCountExpected = -1,
            __in int logManagerLogsCountExpected = -1,
            __in BOOLEAN logManagerIsLoadedExpected = FALSE,
            __in_opt PhysicalLog* physicalLog0 = nullptr,
            __in int physicalLog0HandlesCountExpected = -1,
            __in int physicalLog0LogsCountExpected = -1,
            __in BOOLEAN physicalLog0IsOpenExpected = FALSE,
            __in_opt PhysicalLogHandle* physicalLogHandle0 = nullptr,
            __in BOOLEAN physicalLogHandle0IsFunctionalExpected = FALSE,
            __in_opt PhysicalLog* physicalLog1 = nullptr,
            __in int physicalLog1HandlesCountExpected = -1,
            __in int physicalLog1LogsCountExpected = -1,
            __in BOOLEAN physicalLog1IsOpenExpected = FALSE,
            __in_opt PhysicalLogHandle* physicalLogHandle1 = nullptr,
            __in BOOLEAN physicalLogHandle1IsFunctionalExpected = FALSE,
            __in BOOLEAN physicalLogHandlesOwnersEqualExpected = FALSE,
            __in_opt LogicalLog* logicalLog = nullptr,
            __in BOOLEAN logicalLogIsFunctionalExpected = FALSE) override
        {
            UNREFERENCED_PARAMETER(logManager);
            UNREFERENCED_PARAMETER(logManagerHandlesCountExpected);
            UNREFERENCED_PARAMETER(logManagerLogsCountExpected);
            UNREFERENCED_PARAMETER(logManagerIsLoadedExpected);
            UNREFERENCED_PARAMETER(physicalLog0);
            UNREFERENCED_PARAMETER(physicalLog0HandlesCountExpected);
            UNREFERENCED_PARAMETER(physicalLog0LogsCountExpected);
            UNREFERENCED_PARAMETER(physicalLog0IsOpenExpected);
            UNREFERENCED_PARAMETER(physicalLog1);
            UNREFERENCED_PARAMETER(physicalLog1HandlesCountExpected);
            UNREFERENCED_PARAMETER(physicalLog1LogsCountExpected);
            UNREFERENCED_PARAMETER(physicalLog1IsOpenExpected);
            UNREFERENCED_PARAMETER(physicalLogHandle0);
            UNREFERENCED_PARAMETER(physicalLogHandle0IsFunctionalExpected);
            UNREFERENCED_PARAMETER(physicalLogHandle1);
            UNREFERENCED_PARAMETER(physicalLogHandle1IsFunctionalExpected);
            UNREFERENCED_PARAMETER(physicalLogHandlesOwnersEqualExpected);
            UNREFERENCED_PARAMETER(logicalLog);
            UNREFERENCED_PARAMETER(logicalLogIsFunctionalExpected);
        }

    private:

        BOOLEAN firstCreate_ = TRUE;
    };
    
    KDefineDefaultCreate(FileLogicalLogTest, context);

    FileLogicalLogTest::FileLogicalLogTest() {}
    FileLogicalLogTest::~FileLogicalLogTest() {}

    class FileLogicalLogTests
    {
    protected:

        VOID BeginTest()
        {
            NTSTATUS status = FileLogicalLogTest::Create(testContext_, *allocator_);
            VERIFY_STATUS_SUCCESS("FileLogicalLogTest::Create", status);

            testContext_->CreateTestDirectory();
        }

        VOID EndTest()
        {
            testContext_->DeleteTestDirectory();
            testContext_ = nullptr;
        };

        FileLogicalLogTests() {}
        ~FileLogicalLogTests() {}
        
        FileLogicalLogTest::SPtr testContext_;
        Common::CommonConfig config; // load the config object as its needed for the tracing to work
        ::FABRIC_REPLICA_ID rId_;
        KGuid pId_;
        Data::Utilities::PartitionedReplicaId::SPtr prId_;
        KtlSystem* underlyingSystem_;
        KAllocator* allocator_;
    };

    BOOST_FIXTURE_TEST_SUITE(FileLogicalLogTestsSuite, FileLogicalLogTests)

    // todo: add when file read stream close is fixed
    /*BOOST_AUTO_TEST_CASE(FileLogicalLog_OpenManyStreams)
    {
        TEST_TRACE_BEGIN("FileLogicalLog_OpenManyStreams")
        {
            testContext_->OpenManyStreamsTest();
        }
    }*/

    BOOST_AUTO_TEST_CASE(FileLogicalLog_ReadAheadCacheTest)
    {
        TEST_TRACE_BEGIN("FileLogicalLog_ReadAheadCacheTest")
        {
            testContext_->ReadAheadCacheTest(KtlLoggerMode::Default);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
