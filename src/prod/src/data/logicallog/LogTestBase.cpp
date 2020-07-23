// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace LogTests
{
    using namespace Common;
    using namespace ktl;
    using namespace Data::Log;
    using namespace Data::Utilities;

    volatile bool LogTestBase::g_UseKtlFilePrefix = true;

    LPCWSTR LogTestBase::GetTestDirectoryPath()
    {
#if !defined(PLATFORM_UNIX)
        if (LogTestBase::g_UseKtlFilePrefix)
        {
            return L"\\??\\c:\\LogTestTemp";
        }
        else
        {
            return L"c:\\LogTestTemp";
        }
#else
        return L"/tmp/logtesttemp/";
#endif
    }

    Common::StringLiteral const TraceComponent("LogTestBase");

    NTSTATUS CreateKtlLogManager(
        __in KtlLoggerMode ktlLoggerMode,
        __in KAllocator& allocator,
        __out KtlLogManager::SPtr& physicalLogManager)
    {
        switch (ktlLoggerMode)
        {
        case KtlLoggerMode::Default:
#if !defined(PLATFORM_UNIX)
            return KtlLogManager::CreateDriver(KTL_TAG_TEST, allocator, physicalLogManager);
#else
            return KtlLogManager::CreateInproc(KTL_TAG_TEST, allocator, physicalLogManager);
#endif
            break;
        case KtlLoggerMode::InProc:
            return KtlLogManager::CreateInproc(KTL_TAG_TEST, allocator, physicalLogManager);
            break;
        case KtlLoggerMode::OutOfProc:
            return KtlLogManager::CreateDriver(KTL_TAG_TEST, allocator, physicalLogManager);
            break;
        default:
            KInvariant(FALSE);
            return STATUS_NOT_IMPLEMENTED;
            break;
        }
    }

    VOID LogTestBase::RestoreEOFState(
        __in Data::Log::ILogicalLog& log,
        __in LONG dataSize,
        __in LONG prefetchSize)
    {
        LONGLONG fixPos = dataSize - prefetchSize;
        NTSTATUS status = SyncAwait(log.TruncateTail(fixPos, CancellationToken::None));
        VERIFY_STATUS_SUCCESS(L"ILogicalLog::TruncateTail", status);

        KBuffer::SPtr fixBuffer;
        PUCHAR b;
        AllocBuffer(prefetchSize, fixBuffer, b);

        BuildDataBuffer(*fixBuffer, fixPos);
        status = SyncAwait(log.AppendAsync(*fixBuffer, 0, prefetchSize, CancellationToken::None));
        VERIFY_STATUS_SUCCESS(L"IeLogicalLog::AppendAsync", status);

        status = SyncAwait(log.FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS(L"ILogicalLog::FlushWithMarkerAsync", status);
    }

    VOID LogTestBase::GenerateUniqueFilename(__out KString::SPtr& filename)
    {
        BOOLEAN res;
        NTSTATUS status = KString::Create(filename, GetThisAllocator());
        KInvariant(NT_SUCCESS(status));

        res = filename->Concat(GetTestDirectoryPath());
        KInvariant(res);
        
        res = filename->Concat(KVolumeNamespace::PathSeparator);
        KInvariant(res);

        KGuid guid;
        guid.CreateNew();

        const KWString guidStr(GetThisAllocator(), guid);
        PWCHAR guidStrPtr = (PWCHAR)guidStr;

        res = filename->Concat(guidStrPtr);
        KInvariant(res);

        res = filename->Concat(L".testlog");
        KInvariant(res);
    }

    VOID LogTestBase::CreateTestDirectory()
    {
        if (!Common::Directory::Exists(GetTestDirectoryPath()))
        {
            Common::ErrorCode errorCode = Common::Directory::Create2(GetTestDirectoryPath());
            VERIFY_IS_TRUE(errorCode.IsSuccess(), L"Common::Directory::Create2");
        }
    }

    VOID LogTestBase::DeleteTestDirectory()
    {
        Common::ErrorCode errorCode = Common::Directory::Delete(GetTestDirectoryPath(), true);
        VERIFY_IS_TRUE(errorCode.IsSuccess(), L"Common::Directory::Delete");
    }

    VOID LogTestBase::AllocBuffer(
        __in LONG length,
        __out KBuffer::SPtr& buffer,
        __out PUCHAR& bufferPtr)
    {
        NTSTATUS status;
        KBuffer::SPtr b;

        status = KBuffer::Create(length,
            b,
            GetThisAllocator(),
            GetThisAllocationTag());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        buffer = Ktl::Move(b);
        bufferPtr = static_cast<PUCHAR>(buffer->GetBuffer());
    }

    VOID LogTestBase::BuildDataBuffer(
        __in KBuffer& buffer,
        __in LONGLONG positionInStream,
        __in LONG entropy,
        __in UCHAR recordSize)
    {
        //
        // Build a data buffer that represents the position in the log. Buffers built
        // are exactly reproducible as they are used for writing and reading
        //
        PUCHAR b = static_cast<PUCHAR>(buffer.GetBuffer());
        ULONG length = buffer.QuerySize();

        for (ULONG i = 0; i < length; i++)
        {
            LONGLONG pos = positionInStream + i;
            LONGLONG value = (pos * pos) + pos + entropy;
            b[i] = static_cast<UCHAR>(value % recordSize);
        }
    }

    VOID LogTestBase::ValidateDataBuffer(
        __in KBuffer& buffer,
        __in ULONG length,
        __in ULONG offsetInBuffer,
        __in LONGLONG positionInStream,
        __in ULONG entropy,
        __in UCHAR recordSize)
    {
        //
        // Build a data buffer that represents the position in the log. Buffers built
        // are exactly reproducible as they are used for writing and reading
        //
        PUCHAR bptr = static_cast<PUCHAR>(buffer.GetBuffer());

        for (ULONG i = 0; i < length; i++)
        {
            LONGLONG position = positionInStream + i;
            LONGLONG value = (position * position) + position + entropy;

            UCHAR b = static_cast<UCHAR>(value % recordSize);

            if (bptr[i + offsetInBuffer] != b)
            {
                TestCommon::TestSession::WriteError(TraceComponent, "Data error {0} expecting {1} at stream offset {2}", bptr[i + offsetInBuffer], b, positionInStream + i);
                VERIFY_IS_TRUE(bptr[i + offsetInBuffer] == b);
            }
        }
    }

    const int NumStreams = 100;

    VOID LogTestBase::OpenManyStreamsTest(__in KtlLoggerMode)
    {
        NTSTATUS status;

        ILogManagerHandle::SPtr logManager;
        status = CreateAndOpenLogManager(logManager);
        VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);

        // Don't care if this fails
        SyncAwait(logManager->DeletePhysicalLogAsync(CancellationToken::None));

        IPhysicalLogHandle::SPtr physicalLog;
        status = CreateDefaultPhysicalLog(*logManager, physicalLog);
        VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);

        KString::SPtr logicalLogName;
        GenerateUniqueFilename(logicalLogName);

        KGuid logicalLogId;
        logicalLogId.CreateNew();

        ILogicalLog::SPtr logicalLog;
        status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);

        KEvent taskComplete(FALSE, FALSE);  // auto reset

        // The original bug causes these tests to deadlock, so post them on the 
        // threadpool to check for failure (rather than waiting for test timeout)

        // Test creating and immediately closing streams
        {
            Common::Threadpool::Post([&]()
            {
                for (int i = 0; i < NumStreams; i++)
                {
                    ILogicalLogReadStream::SPtr readStream;

                    status = logicalLog->CreateReadStream(readStream, 0);
                    if (!NT_SUCCESS(status))
                    {
                        taskComplete.SetEvent();
                        return;
                    }

                    status = SyncAwait(readStream->CloseAsync());
                    if (!NT_SUCCESS(status))
                    {
                        taskComplete.SetEvent();
                        return;
                    }

                    readStream = nullptr;
                }

                status = STATUS_SUCCESS;
                taskComplete.SetEvent();
            });

            bool completed = taskComplete.WaitUntilSet(15000);
            VERIFY_IS_TRUE(completed); // Likely deadlocked if false
            VERIFY_STATUS_SUCCESS("OpenManyStreamsTest_OpenAndImmediatelyClose", status);
        }

        // Close and reopen the log
        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);

        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

        // Test creating and immediately disposing streams
        {
            Common::Threadpool::Post([&]()
            {
                for (int i = 0; i < NumStreams; i++)
                {
                    ILogicalLogReadStream::SPtr readStream;

                    status = logicalLog->CreateReadStream(readStream, 0);
                    if (!NT_SUCCESS(status))
                    {
                        taskComplete.SetEvent();
                        return;
                    }

                    readStream->Dispose();
                    if (!NT_SUCCESS(status))
                    {
                        taskComplete.SetEvent();
                        return;
                    }

                    readStream = nullptr;
                }

                status = STATUS_SUCCESS;
                taskComplete.SetEvent();
            });

            bool completed = taskComplete.WaitUntilSet(15000);
            VERIFY_IS_TRUE(completed); // Likely deadlocked if false
            VERIFY_STATUS_SUCCESS("OpenManyStreamsTest_OpenAndImmediatelyDispose", status);
        }

        // Close and reopen the log
        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);

        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

        // Test creating and immediately destructing streams
        {
            Common::Threadpool::Post([&]()
            {
                for (int i = 0; i < NumStreams; i++)
                {
                    ILogicalLogReadStream::SPtr readStream;

                    status = logicalLog->CreateReadStream(readStream, 0);
                    if (!NT_SUCCESS(status))
                    {
                        taskComplete.SetEvent();
                        return;
                    }

                    readStream = nullptr;
                }

                status = STATUS_SUCCESS;
                taskComplete.SetEvent();
            });

            bool completed = taskComplete.WaitUntilSet(15000);
            VERIFY_IS_TRUE(completed); // Likely deadlocked if false
            VERIFY_STATUS_SUCCESS("OpenManyStreamsTest_OpenAndImmediatelyDestruct", status);
        }

        // Close and reopen the log
        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);

        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

        // Test creating all streams then closing
        {
            // Note: this actually passes with the original bug, as only the streams array 'append' codepath is exercised
            Common::Threadpool::Post([&]()
            {
                ILogicalLogReadStream::SPtr streams[NumStreams];

                for (int i = 0; i < NumStreams; i++)
                {
                    status = logicalLog->CreateReadStream(streams[i], 0);
                    if (!NT_SUCCESS(status))
                    {
                        taskComplete.SetEvent();
                        return;
                    }
                }

                for (int i = 0; i < NumStreams; i++)
                {
                    status = SyncAwait(streams[i]->CloseAsync());
                    streams[i] = nullptr;

                    if (!NT_SUCCESS(status))
                    {
                        taskComplete.SetEvent();
                        return;
                    }
                }

                status = STATUS_SUCCESS;
                taskComplete.SetEvent();
            });

            bool completed = taskComplete.WaitUntilSet(15000);
            VERIFY_IS_TRUE(completed); // Likely deadlocked if false
            VERIFY_STATUS_SUCCESS("OpenManyStreamsTest_OpenAllThenClose", status);
        }

        // Close and reopen the log
        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);

        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

        // Test creating all streams then disposing
        {
            // Note: this actually passes with the original bug, as only the streams array 'append' codepath is exercised
            Common::Threadpool::Post([&]()
            {
                ILogicalLogReadStream::SPtr streams[NumStreams];

                for (int i = 0; i < NumStreams; i++)
                {
                    status = logicalLog->CreateReadStream(streams[i], 0);
                    if (!NT_SUCCESS(status))
                    {
                        taskComplete.SetEvent();
                        return;
                    }
                }

                for (int i = 0; i < NumStreams; i++)
                {
                    streams[i]->Dispose();
                    streams[i] = nullptr;
                }

                status = STATUS_SUCCESS;
                taskComplete.SetEvent();
            });

            bool completed = taskComplete.WaitUntilSet(15000);
            VERIFY_IS_TRUE(completed); // Likely deadlocked if false
            VERIFY_STATUS_SUCCESS("OpenManyStreamsTest_OpenAllThenDispose", status);
        }

        // Close and reopen the log
        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);

        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

        // Test creating all streams then destructing
        {
            // Note: this actually passes with the original bug, as only the streams array 'append' codepath is exercised
            Common::Threadpool::Post([&]()
            {
                ILogicalLogReadStream::SPtr streams[NumStreams];

                for (int i = 0; i < NumStreams; i++)
                {
                    status = logicalLog->CreateReadStream(streams[i], 0);
                    if (!NT_SUCCESS(status))
                    {
                        taskComplete.SetEvent();
                        return;
                    }
                }

                for (int i = 0; i < NumStreams; i++)
                {
                    streams[i] = nullptr;
                }

                status = STATUS_SUCCESS;
                taskComplete.SetEvent();
            });

            bool completed = taskComplete.WaitUntilSet(15000);
            VERIFY_IS_TRUE(completed); // Likely deadlocked if false
            VERIFY_STATUS_SUCCESS("OpenManyStreamsTest_OpenAllThenDestruct", status);
        }

        // Close and reopen the log
        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);

        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

        // Test with random choice of when to cleanup stream
        {
            // Note: this actually passes with the original bug, as only the streams array 'append' codepath is exercised
            Common::Threadpool::Post([&]()
            {
                Common::Random random(NumStreams);
                ILogicalLogReadStream::SPtr streams[NumStreams];

                for (int i = 0; i < NumStreams; i++)
                {
                    status = logicalLog->CreateReadStream(streams[i], 0);
                    if (!NT_SUCCESS(status))
                    {
                        taskComplete.SetEvent();
                        return;
                    }

                    switch (random.Next() % 6)
                    {
                    case 0:
                    case 1:
                    case 2:
                        // let it be cleaned up later
                        break;

                    case 3:
                        // Close now
                        status = SyncAwait(streams[i]->CloseAsync());
                        if (!NT_SUCCESS(status))
                        {
                            taskComplete.SetEvent();
                            return;
                        }
                        streams[i] = nullptr;
                        break;

                    case 4:
                        // Dispose now
                        streams[i]->Dispose();
                        streams[i] = nullptr;
                        break;

                    case 5:
                        // Destruct now
                        streams[i] = nullptr;
                        break;
                    }
                }

                for (int i = 0; i < NumStreams; i++)
                {
                    if (streams[i] != nullptr)
                    {
                        switch (random.Next() % 3)
                        {
                        case 0:
                            // Close
                            status = SyncAwait(streams[i]->CloseAsync());
                            if (!NT_SUCCESS(status))
                            {
                                taskComplete.SetEvent();
                                return;
                            }
                            streams[i] = nullptr;
                            break;

                        case 1:
                            // Dispose
                            streams[i]->Dispose();
                            streams[i] = nullptr;
                            break;

                        case 2:
                            // Destruct
                            streams[i] = nullptr;
                            break;
                        }
                    }
                }

                status = STATUS_SUCCESS;
                taskComplete.SetEvent();
            });

            bool completed = taskComplete.WaitUntilSet(15000);
            VERIFY_IS_TRUE(completed); // Likely deadlocked if false
            VERIFY_STATUS_SUCCESS("OpenManyStreamsTest_RandomCleanup", status);
        }

        // Do the random cleanup test again without reopening the log
        {
            // Note: this actually passes with the original bug, as only the streams array 'append' codepath is exercised
            Common::Threadpool::Post([&]()
            {
                Common::Random random(NumStreams);
                ILogicalLogReadStream::SPtr streams[NumStreams];

                for (int i = 0; i < NumStreams; i++)
                {
                    status = logicalLog->CreateReadStream(streams[i], 0);
                    if (!NT_SUCCESS(status))
                    {
                        taskComplete.SetEvent();
                        return;
                    }

                    switch (random.Next() % 6)
                    {
                    case 0:
                    case 1:
                    case 2:
                        // let it be cleaned up later
                        break;

                    case 3:
                        // Close now
                        status = SyncAwait(streams[i]->CloseAsync());
                        if (!NT_SUCCESS(status))
                        {
                            taskComplete.SetEvent();
                            return;
                        }
                        streams[i] = nullptr;
                        break;

                    case 4:
                        // Dispose now
                        streams[i]->Dispose();
                        streams[i] = nullptr;
                        break;

                    case 5:
                        // Destruct now
                        streams[i] = nullptr;
                        break;
                    }
                }

                for (int i = 0; i < NumStreams; i++)
                {
                    if (streams[i] != nullptr)
                    {
                        switch (random.Next() % 3)
                        {
                        case 0:
                            // Close
                            status = SyncAwait(streams[i]->CloseAsync());
                            if (!NT_SUCCESS(status))
                            {
                                taskComplete.SetEvent();
                                return;
                            }
                            streams[i] = nullptr;
                            break;

                        case 1:
                            // Dispose
                            streams[i]->Dispose();
                            streams[i] = nullptr;
                            break;

                        case 2:
                            // Destruct
                            streams[i] = nullptr;
                            break;
                        }
                    }
                }

                status = STATUS_SUCCESS;
                taskComplete.SetEvent();
            });

            bool completed = taskComplete.WaitUntilSet(15000);
            VERIFY_IS_TRUE(completed); // Likely deadlocked if false
            VERIFY_STATUS_SUCCESS("OpenManyStreamsTest_RandomCleanup", status);
        }

        // Cleanup
        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("PhysicalLog::CloseAsync", status);
        physicalLog = nullptr;

        status = SyncAwait(logManager->DeletePhysicalLogAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::DeletePhysicalLogAsync", status);

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::CloseAsync", status);
        logManager = nullptr;
    }

    VOID LogTestBase::BasicIOTest(__in KtlLoggerMode, __in KGuid const & physicalLogId)
    {
        NTSTATUS status;

        ILogManagerHandle::SPtr logManager;
        status = CreateAndOpenLogManager(logManager);
        VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);

        LogManager* logManagerService = nullptr;
        {
            LogManagerHandle* logManagerServiceHandle = dynamic_cast<LogManagerHandle*>(logManager.RawPtr());
            if (logManagerServiceHandle != nullptr)
            {
                logManagerService = logManagerServiceHandle->owner_.RawPtr();
            }
        }
        VerifyState(
            logManagerService,
            1,
            0,
            TRUE);

        KString::SPtr physicalLogName;
        GenerateUniqueFilename(physicalLogName);

        // Don't care if this fails
        SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));

        IPhysicalLogHandle::SPtr physicalLog;
        status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
        VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);

        VerifyState(
            logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            0,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE);


        KString::SPtr logicalLogName;
        GenerateUniqueFilename(logicalLogName);

        KGuid logicalLogId;
        logicalLogId.CreateNew();

        ILogicalLog::SPtr logicalLog;
        status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);
        VerifyState(
            logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE,
            nullptr,
            -1,
            -1,
            FALSE,
            nullptr,
            FALSE,
            FALSE,
            dynamic_cast<LogicalLog*>(logicalLog.RawPtr()),
            TRUE);
        VERIFY_ARE_EQUAL(0, logicalLog->Length);
        VERIFY_ARE_EQUAL(0, logicalLog->ReadPosition);
        VERIFY_ARE_EQUAL(0, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(-1, logicalLog->HeadTruncationPosition);

        static const LONG MaxLogBlkSize = 128 * 1024;


        // Prove simple recovery
        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);
        VERIFY_ARE_EQUAL(0, logicalLog->Length);
        VERIFY_ARE_EQUAL(0, logicalLog->ReadPosition);
        VERIFY_ARE_EQUAL(0, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(-1, logicalLog->HeadTruncationPosition);

        const int recordSize = 131;

        // Prove we can write bytes
        KBuffer::SPtr x;
        PUCHAR xPtr;
        AllocBuffer(recordSize, x, xPtr);
        BuildDataBuffer(*x, 0, 0, recordSize);
        ValidateDataBuffer(*x, x->QuerySize(), 0, 0, 0, recordSize);

        VERIFY_ARE_EQUAL(0, logicalLog->WritePosition);
        status = SyncAwait(logicalLog->AppendAsync(*x, 0, x->QuerySize(), CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::AppendAsync", status);
        VERIFY_ARE_EQUAL(recordSize, logicalLog->WritePosition);

        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::FlushWithMarkerAsync", status);

        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        // Prove simple (non-null recovery - only one (asn 1) physical record in the log)
        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);
        VERIFY_ARE_EQUAL(recordSize, logicalLog->Length);
        VERIFY_ARE_EQUAL(0, logicalLog->ReadPosition);
        VERIFY_ARE_EQUAL(recordSize, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(-1, logicalLog->HeadTruncationPosition);

        status = SyncAwait(logicalLog->AppendAsync(*x, 0, x->QuerySize(), CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::AppendAsync", status);
        VERIFY_ARE_EQUAL(recordSize * 2, logicalLog->WritePosition);
        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::FlushWithMarkerAsync", status);
        
        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        // Prove simple (non-null recovery)
        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);
        VERIFY_ARE_EQUAL(recordSize * 2, logicalLog->Length);
        VERIFY_ARE_EQUAL(0, logicalLog->ReadPosition);
        VERIFY_ARE_EQUAL(recordSize * 2, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(-1, logicalLog->HeadTruncationPosition);

        // Prove simple read and positioning works
        AllocBuffer(recordSize, x, xPtr);
        LONG bytesRead;
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, x->QuerySize(), 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(recordSize, bytesRead);
        ValidateDataBuffer(*x, x->QuerySize(), 0, 0, 0, recordSize);

        VERIFY_ARE_EQUAL(recordSize, logicalLog->ReadPosition);

        // Prove next sequential physical read works
        AllocBuffer(recordSize, x, xPtr);
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, x->QuerySize(), 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(recordSize, bytesRead);
        ValidateDataBuffer(*x, x->QuerySize(), 0, 0, 0, recordSize);

        VERIFY_ARE_EQUAL(recordSize * 2, logicalLog->ReadPosition);

        // Positioning and reading partial and across physical records work
        int offset = 1;
        status = logicalLog->SeekForRead(offset, Common::SeekOrigin::Enum::Begin);
        VERIFY_STATUS_SUCCESS("LogicalLog::SeekForRead", status);
        AllocBuffer(recordSize, x, xPtr);
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, x->QuerySize(), 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(recordSize, bytesRead);
        ValidateDataBuffer(*x, x->QuerySize(), 0, offset, 0, recordSize);

        VERIFY_ARE_EQUAL(recordSize + offset, logicalLog->ReadPosition);

        // Prove reading a short record works
        x->Zero();
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, x->QuerySize(), 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(recordSize - 1, bytesRead);
        ValidateDataBuffer(*x, recordSize - 1, 0, offset, 0, recordSize);

        VERIFY_ARE_EQUAL(logicalLog->WritePosition, logicalLog->ReadPosition);

        // Prove reading at EOS works
        AllocBuffer(recordSize, x, xPtr);
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, x->QuerySize(), 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(0, bytesRead);
        VERIFY_ARE_EQUAL(logicalLog->WritePosition, logicalLog->ReadPosition);

        // Prove position beyond and just EOS and reading works
        LONGLONG currentPos;
        status = logicalLog->SeekForRead(1, Common::SeekOrigin::Enum::End);
        VERIFY_STATUS_SUCCESS("LogicalLog::SeekForRead", status);
        currentPos = logicalLog->ReadPosition;
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, x->QuerySize(), 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(0, bytesRead);

        LONGLONG eos;
        status = logicalLog->SeekForRead(0, Common::SeekOrigin::Enum::End);
        VERIFY_STATUS_SUCCESS("LogicalLog::SeekForRead", status);
        eos = logicalLog->ReadPosition;
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, x->QuerySize(), 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(0, bytesRead);

        VERIFY_ARE_EQUAL(logicalLog->WritePosition, eos);

        status = logicalLog->SeekForRead(-1, Common::SeekOrigin::Enum::End);
        VERIFY_STATUS_SUCCESS("LogicalLog::SeekForRead", status);
        currentPos = logicalLog->ReadPosition;
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, 1, 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(1, bytesRead);
        ValidateDataBuffer(*x, 1, 0, currentPos, 0, recordSize);

        status = logicalLog->SeekForRead(-1, Common::SeekOrigin::Enum::End);
        VERIFY_STATUS_SUCCESS("LogicalLog::SeekForRead", status);
        currentPos = logicalLog->ReadPosition;
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, 2, 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(1, bytesRead);
        ValidateDataBuffer(*x, 1, 0, currentPos, 0, recordSize);

        status = logicalLog->SeekForRead(-2, Common::SeekOrigin::Enum::End);
        VERIFY_STATUS_SUCCESS("LogicalLog::SeekForRead", status);
        currentPos = logicalLog->ReadPosition;
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, 2, 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(2, bytesRead);
        ValidateDataBuffer(*x, 2, 0, currentPos, 0, recordSize);

        // Prove we can position forward to every location
        currentPos = eos - 1;

        while (currentPos >= 0)
        {
            status = logicalLog->SeekForRead(currentPos, Common::SeekOrigin::Enum::Begin);
            VERIFY_STATUS_SUCCESS("LogicalLog::SeekForRead", status);
            VERIFY_ARE_EQUAL(currentPos, logicalLog->ReadPosition);
            KBuffer::SPtr rBuff;
            status = KBuffer::Create((ULONG)(eos - currentPos), rBuff, GetThisAllocator());
            VERIFY_STATUS_SUCCESS("KBuffer::Create", status);
            status = SyncAwait(logicalLog->ReadAsync(bytesRead, *rBuff, 0, rBuff->QuerySize(), 0, CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
            VERIFY_ARE_EQUAL(rBuff->QuerySize(), (ULONG)bytesRead);
            currentPos--;
        }

        // Prove Stream read abstraction works through a BufferedStream
        // (since we don't have a BufferedStream, use KStream)
        ILogicalLogReadStream::SPtr readStream;
        status = logicalLog->CreateReadStream(readStream, 0);
        VERIFY_STATUS_SUCCESS("LogicalLog::CreateReadStream", status);
        ktl::io::KStream::SPtr bStream = readStream.RawPtr();

        bStream->Position = bStream->Length + 1; // seek to end + 1
        currentPos = bStream->Position;
        ULONG uBytesRead;
        status = SyncAwait(bStream->ReadAsync(*x, uBytesRead, 0, x->QuerySize()));
        VERIFY_STATUS_SUCCESS("KStream::ReadAsync", status);
        VERIFY_ARE_EQUAL(0, uBytesRead);

        bStream->Position = bStream->Length; // seek to end
        eos = bStream->Position;
        VERIFY_ARE_EQUAL(logicalLog->WritePosition, eos);

        bStream->Position = bStream->Length - 1; // seek to end - 1
        currentPos = bStream->Position;
        status = SyncAwait(bStream->ReadAsync(*x, uBytesRead, 0, 1));
        VERIFY_STATUS_SUCCESS("KStream::ReadAsync", status);
        VERIFY_ARE_EQUAL(1, uBytesRead);
        ValidateDataBuffer(*x, 1, 0, currentPos, 0, recordSize);

        bStream->Position = bStream->Length - 1; // seek to end - 1
        currentPos = bStream->Position;
        status = SyncAwait(bStream->ReadAsync(*x, uBytesRead, 0, 2));
        VERIFY_STATUS_SUCCESS("KStream::ReadAsync", status);
        VERIFY_ARE_EQUAL(1, uBytesRead);
        ValidateDataBuffer(*x, 1, 0, currentPos, 0, recordSize);

        bStream->Position = bStream->Length - 2; // seek to end - 2
        currentPos = bStream->Position;
        status = SyncAwait(bStream->ReadAsync(*x, uBytesRead, 0, 2));
        VERIFY_STATUS_SUCCESS("KStream::ReadAsync", status);
        VERIFY_ARE_EQUAL(2, uBytesRead);
        ValidateDataBuffer(*x, 2, 0, currentPos, 0, recordSize);

        currentPos = eos - 1;

        while (currentPos >= 0)
        {
            bStream->Position = currentPos;
            KBuffer::SPtr rBuff;
            status = KBuffer::Create((ULONG)(eos - currentPos), rBuff, GetThisAllocator());
            VERIFY_STATUS_SUCCESS("KBuffer::Create", status);
            status = SyncAwait(bStream->ReadAsync(*rBuff, uBytesRead, 0, rBuff->QuerySize()));
            VERIFY_STATUS_SUCCESS("KStream::ReadAsync", status);
            VERIFY_ARE_EQUAL(rBuff->QuerySize(), uBytesRead);

            currentPos--;
        }

        status = SyncAwait(bStream->CloseAsync());
        VERIFY_STATUS_SUCCESS("KStream::CloseAsync", status);
        bStream = nullptr;
        readStream->Dispose(); // idempotent
        readStream = nullptr;

        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        status = SyncAwait(physicalLog->DeleteLogicalLogOnlyAsync(logicalLogId, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("IPhysicalLogHandle::DeleteLogicalLogOnlyAsync", status);

        // Prove some large I/O
        status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);
        VerifyState(
            logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE,
            nullptr,
            -1,
            -1,
            FALSE,
            nullptr,
            FALSE,
            FALSE,
            dynamic_cast<LogicalLog*>(logicalLog.RawPtr()),
            TRUE);
        VERIFY_ARE_EQUAL(0, logicalLog->Length);
        VERIFY_ARE_EQUAL(0, logicalLog->ReadPosition);
        VERIFY_ARE_EQUAL(0, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(-1, logicalLog->HeadTruncationPosition);

        AllocBuffer(1024 * 1024, x, xPtr);
        BuildDataBuffer(*x, 0, 0, recordSize);

        status = SyncAwait(logicalLog->AppendAsync(*x, 0, x->QuerySize(), CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::AppendAsync", status);
        VERIFY_ARE_EQUAL(1024 * 1024, logicalLog->WritePosition);
        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::FlushWithMarkerAsync", status);

        AllocBuffer(1024 * 1024, x, xPtr);
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, x->QuerySize(), 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(x->QuerySize(), (ULONG)bytesRead);
        ValidateDataBuffer(*x, x->QuerySize(), 0, 0, 0, recordSize);

        // Prove basic head truncation behavior
        status = SyncAwait(logicalLog->TruncateHead(128 * 1024));
        VERIFY_STATUS_SUCCESS("LogicalLog::TruncateHead", status);
        VERIFY_ARE_EQUAL(128 * 1024, logicalLog->HeadTruncationPosition);

        // Force a record out to make sure the head truncation point is captured
        const KStringView testStr1(L"Test String Record");
        AllocBuffer(testStr1.LengthInBytes(), x, xPtr);
        LPCWSTR str = testStr1;
        KMemCpySafe(xPtr, x->QuerySize(), str, testStr1.LengthInBytes());
        status = SyncAwait(logicalLog->AppendAsync(*x, 0, x->QuerySize(), CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::AppendAsync", status);
        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::FlushAsync", status);

        // Prove head truncation point is recovered
        LONGLONG currentLength = logicalLog->WritePosition;
        LONGLONG currentTrucPoint = logicalLog->HeadTruncationPosition;

        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);
        VERIFY_ARE_EQUAL(currentLength, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(currentTrucPoint, logicalLog->HeadTruncationPosition);

        // Prove basic tail truncation behavior
        status = logicalLog->SeekForRead(currentLength - x->QuerySize(), Common::SeekOrigin::Enum::Begin);
        VERIFY_STATUS_SUCCESS("LogicalLog::SeekForRead", status);
        LONGLONG newEos = logicalLog->ReadPosition;
        AllocBuffer(256, x, xPtr);
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, x->QuerySize(), 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        KString::SPtr chars;
        status = KString::Create(chars, GetThisAllocator(), bytesRead / sizeof(WCHAR));
        VERIFY_STATUS_SUCCESS("KString::Create", status);
        chars->SetLength(chars->BufferSizeInChars());
        KMemCpySafe((PBYTE)(PVOID)*chars, chars->LengthInBytes(), xPtr, (ULONG)bytesRead);
        VERIFY_ARE_EQUAL(testStr1, *chars);

        // Chop off the last and part of the 2nd to last records
        newEos = newEos - bytesRead - 1;
        status = SyncAwait(logicalLog->TruncateTail(newEos, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::TruncateTail", status);
        VERIFY_ARE_EQUAL(newEos, logicalLog->WritePosition);

        // Prove we can read valid up to the new EOS but not beyond
        status = logicalLog->SeekForRead(newEos - 128, Common::SeekOrigin::Enum::Begin);
        VERIFY_STATUS_SUCCESS("LogicalLog::SeekForRead", status);
        AllocBuffer(256, x, xPtr);
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, x->QuerySize(), 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(128, bytesRead);
        ValidateDataBuffer(*x, bytesRead, 0, newEos - 128, 0, recordSize);

        // Prove we recover the correct tail position (WritePosition)
        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);
        VERIFY_ARE_EQUAL(newEos, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(currentTrucPoint, logicalLog->HeadTruncationPosition);

        // Prove we can truncate all content via tail truncation
        VERIFY_ARE_NOT_EQUAL(0, logicalLog->Length);
        status = SyncAwait(logicalLog->TruncateTail(currentTrucPoint + 1, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::TruncateTail", status);
        VERIFY_ARE_EQUAL(0, logicalLog->Length);
        VERIFY_ARE_EQUAL(currentTrucPoint + 1, logicalLog->WritePosition);

        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);
        VERIFY_ARE_EQUAL(currentTrucPoint + 1, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(0, logicalLog->Length);
        VERIFY_ARE_EQUAL(currentTrucPoint, logicalLog->HeadTruncationPosition);

        // Add proof for truncating all but one byte - read it for the right value
        LONGLONG currentWritePos = logicalLog->WritePosition;
        AllocBuffer(1024 * 1024, x, xPtr);
        BuildDataBuffer(*x, currentWritePos, 0, recordSize);

        status = SyncAwait(logicalLog->AppendAsync(*x, 0, x->QuerySize(), CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::AppendAsync", status);
        VERIFY_ARE_EQUAL(currentWritePos + (1024 * 1024), logicalLog->WritePosition);
        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::FlushWithMarkerAsync", status);

        status = logicalLog->SeekForRead(currentWritePos, Common::SeekOrigin::Enum::Begin);
        VERIFY_STATUS_SUCCESS("LogicalLog::SeekForRead", status);
        VERIFY_ARE_EQUAL(currentWritePos, logicalLog->ReadPosition);
        AllocBuffer(1024 * 1024, x, xPtr);
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, x->QuerySize(), 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(x->QuerySize(), (ULONG)bytesRead);
        ValidateDataBuffer(*x, x->QuerySize(), 0, currentWritePos, 0, recordSize);

        status = SyncAwait(logicalLog->TruncateHead(currentWritePos + (1024 * 513)));
        VERIFY_STATUS_SUCCESS("LogicalLog::TruncateHead", status);
        VERIFY_ARE_EQUAL(currentWritePos + (1024 * 513), logicalLog->HeadTruncationPosition);

        status = logicalLog->SeekForRead(currentWritePos + (1024 * 513) + 1, Common::SeekOrigin::Enum::Begin);
        VERIFY_STATUS_SUCCESS("LogicalLog::SeekForRead", status);
        VERIFY_ARE_EQUAL(currentWritePos + (1024 * 513) + 1, logicalLog->ReadPosition);
        AllocBuffer((ULONG)logicalLog->Length, x, xPtr);
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, x->QuerySize(), 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(x->QuerySize(), (ULONG)bytesRead);
        ValidateDataBuffer(*x, x->QuerySize(), 0, currentWritePos + (1024 * 513) + 1, 0, recordSize);

        status = SyncAwait(logicalLog->TruncateTail(currentWritePos + (1024 * 513) + 2, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::TruncateTail", status);
        VERIFY_ARE_EQUAL(1, logicalLog->Length);

        status = logicalLog->SeekForRead(currentWritePos + (1024 * 513) + 1, Common::SeekOrigin::Enum::Begin);
        VERIFY_STATUS_SUCCESS("LogicalLog::SeekForRead", status);
        VERIFY_ARE_EQUAL(currentWritePos + (1024 * 513) + 1, logicalLog->ReadPosition);
        ((PBYTE)x->GetBuffer())[0] = ~(((PBYTE)x->GetBuffer())[0]);
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *x, 0, 1, 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::ReadAsync", status);
        VERIFY_ARE_EQUAL(1, bytesRead);
        ValidateDataBuffer(*x, 1, 0, currentWritePos + (1024 * 513) + 1, 0, recordSize);

        // Prove close down
        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("PhysicalLog::CloseAsync", status);
        physicalLog = nullptr;

        status = SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::DeletePhysicalLogAsync", status);

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::CloseAsync", status);
        logManager = nullptr;
    }

    VOID LogTestBase::WriteAtTruncationPointsTest(__in KtlLoggerMode)
    {
        NTSTATUS status;

        ILogManagerHandle::SPtr logManager;
        status = CreateAndOpenLogManager(logManager);
        VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);

        LogManager* logManagerService = nullptr;
        {
            LogManagerHandle* logManagerServiceHandle = dynamic_cast<LogManagerHandle*>(logManager.RawPtr());
            if (logManagerServiceHandle != nullptr)
            {
                logManagerService = logManagerServiceHandle->owner_.RawPtr();
            }
        }
        VerifyState(
            logManagerService,
            1,
            0,
            TRUE);

        KString::SPtr physicalLogName;
        GenerateUniqueFilename(physicalLogName);

        KGuid physicalLogId;
        physicalLogId.CreateNew();

        // Don't care if this fails
        SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));

        IPhysicalLogHandle::SPtr physicalLog;
        status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
        VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);
        VerifyState(logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            0,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE);


        KGuid logicalLogId;
        logicalLogId.CreateNew();
        KString::SPtr logicalLogFileName;
        GenerateUniqueFilename(logicalLogFileName);
        
        ILogicalLog::SPtr logicalLog;
        status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogFileName, logicalLog);
        VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);
        VerifyState(
            logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE,
            nullptr,
            -1,
            -1,
            FALSE,
            nullptr,
            FALSE,
            FALSE,
            dynamic_cast<LogicalLog*>(logicalLog.RawPtr()),
            TRUE);
        VERIFY_IS_TRUE(logicalLog->IsFunctional);
        VERIFY_ARE_EQUAL(0, logicalLog->Length);
        VERIFY_ARE_EQUAL(0, logicalLog->ReadPosition);
        VERIFY_ARE_EQUAL(0, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(-1, logicalLog->HeadTruncationPosition);

        // Prove simple recovery

        status = SyncAwait(logicalLog->CloseAsync());
        VERIFY_STATUS_SUCCESS("logicalLog->CloseAsync", status);
        logicalLog = nullptr;

        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogFileName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);
        VERIFY_IS_TRUE(logicalLog->IsFunctional);
        VERIFY_ARE_EQUAL(0, logicalLog->Length);
        VERIFY_ARE_EQUAL(0, logicalLog->ReadPosition);
        VERIFY_ARE_EQUAL(0, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(-1, logicalLog->HeadTruncationPosition);


        //
        // Test 1: Write all 1's from 0 - 999, truncate tail (newest data) at 500, write 2's at 500 - 599
        // and read back verifying that 0 - 499 are 1's and 500-599 are 2's. Read back at 499, 500 and 501
        // and verify that those are correct.
        //
        KBuffer::SPtr buffer1;
        PUCHAR buffer1Ptr;
        AllocBuffer(1000, buffer1, buffer1Ptr);
        for (int i = 0; i < 1000; i++)
        {
            buffer1Ptr[i] = 1;
        }

        status = SyncAwait(logicalLog->AppendAsync(*buffer1, 0, buffer1->QuerySize(), CancellationToken::None));
        VERIFY_STATUS_SUCCESS("AppendAsync", status);
        VERIFY_ARE_EQUAL(1000, logicalLog->WritePosition);

        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("FlushWithMarker", status);

        status = SyncAwait(logicalLog->TruncateTail(500, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("TruncateTail", status);
        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("FlushWithMarker", status);
        VERIFY_ARE_EQUAL(500, logicalLog->WritePosition);

        KBuffer::SPtr buffer2;
        PUCHAR buffer2Ptr;
        AllocBuffer(100, buffer2, buffer2Ptr);
        for (int i = 0; i < 100; i++)
        {
            buffer2Ptr[i] = 2;
        }

        status = SyncAwait(logicalLog->AppendAsync(*buffer2, 0, buffer2->QuerySize(), CancellationToken::None));
        VERIFY_STATUS_SUCCESS("AppendAsync", status);
        VERIFY_ARE_EQUAL(600, logicalLog->WritePosition);

        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("FlushWithMarker", status);

        KBuffer::SPtr bufferRead;
        PUCHAR bufferReadPtr;
        AllocBuffer(600, bufferRead, bufferReadPtr);
        LONG bytesRead;
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *bufferRead, 0, 600, 0, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("Read", status);
        for (int i = 0; i < 500; i++)
        {
            VERIFY_ARE_EQUAL(1, bufferReadPtr[i]);
        }
        for (int i = 500; i < 600; i++)
        {
            VERIFY_ARE_EQUAL(2, bufferReadPtr[i]);
        }

        bufferReadPtr[0] = 0;
        status = logicalLog->SeekForRead(499, SeekOrigin::Begin);
        VERIFY_STATUS_SUCCESS("SeekForRead", status);
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *bufferRead, 0, 1, 0, CancellationToken::None));
        VERIFY_ARE_EQUAL(1, bufferReadPtr[0]);

        bufferReadPtr[0] = 0;
        status = logicalLog->SeekForRead(500, SeekOrigin::Begin);
        VERIFY_STATUS_SUCCESS("SeekForRead", status);
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *bufferRead, 0, 1, 0, CancellationToken::None));
        VERIFY_ARE_EQUAL(2, bufferReadPtr[0]);

        bufferReadPtr[0] = 0;
        status = logicalLog->SeekForRead(501, SeekOrigin::Begin);
        VERIFY_STATUS_SUCCESS("SeekForRead", status);
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *bufferRead, 0, 1, 0, CancellationToken::None));
        VERIFY_ARE_EQUAL(2, bufferReadPtr[0]);


        //
        // Test 2: Truncate head (oldest data) at 500 and try to truncate tail (newest data) to 490 and expect failure.
        // Truncate tail back to 500 and then write and then read back
        //
        status = SyncAwait(logicalLog->TruncateHead(499));
        VERIFY_STATUS_SUCCESS("TruncateHead", status);

        status = SyncAwait(logicalLog->TruncateTail(490, CancellationToken::None));
        VERIFY_IS_FALSE(NT_SUCCESS(status));

        status = SyncAwait(logicalLog->TruncateTail(500, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("TruncateTail", status);
        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("FlushWithMarker", status);

        KBuffer::SPtr buffer3;
        PUCHAR buffer3Ptr;
        AllocBuffer(50, buffer3, buffer3Ptr);
        for (int i = 0; i < 50; i++)
        {
            buffer3Ptr[i] = 3;
        }

        status = SyncAwait(logicalLog->AppendAsync(*buffer3, 0, buffer3->QuerySize(), CancellationToken::None));
        VERIFY_STATUS_SUCCESS("AppendAsync", status);
        VERIFY_ARE_EQUAL(550, logicalLog->WritePosition);

        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("FlushWithMarker", status);

        KBuffer::SPtr bufferRead3;
        PUCHAR bufferRead3Ptr;
        AllocBuffer(50, bufferRead3, bufferRead3Ptr);
        for (int i = 0; i < 50; i++)
        {
            bufferRead3Ptr[i] = 0;
        }

        status = logicalLog->SeekForRead(500, SeekOrigin::Begin);
        VERIFY_STATUS_SUCCESS("SeekForRead", status);
        status = SyncAwait(logicalLog->ReadAsync(bytesRead, *bufferRead3, 0, 50, 0, CancellationToken::None));

        for (int i = 0; i < 50; i++)
        {
            VERIFY_ARE_EQUAL(3, bufferRead3Ptr[i]);
        }


        //
        // Test 3: truncate tail all the way back to the head
        //
        auto headTruncationPosition = logicalLog->HeadTruncationPosition;
        status = SyncAwait(logicalLog->TruncateHead(headTruncationPosition + 1));
        VERIFY_STATUS_SUCCESS("TruncateHead", status);

        // Prove close down
        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("PhysicalLog::CloseAsync", status);
        physicalLog = nullptr;

        status = SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::DeletePhysicalLogAsync", status);

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::CloseAsync", status);
        logManager = nullptr;
    }

    VOID LogTestBase::RemoveFalseProgressTest(__in KtlLoggerMode)
    {
        // Main line test

        NTSTATUS status;

        ILogManagerHandle::SPtr logManager;
        status = CreateAndOpenLogManager(logManager);
        VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);

        LogManager* logManagerService = nullptr;
        {
            LogManagerHandle* logManagerServiceHandle = dynamic_cast<LogManagerHandle*>(logManager.RawPtr());
            if (logManagerServiceHandle != nullptr)
            {
                logManagerService = logManagerServiceHandle->owner_.RawPtr();
            }
        }
        VerifyState(
            logManagerService,
            1,
            0,
            TRUE);

        KString::SPtr physicalLogName;
        GenerateUniqueFilename(physicalLogName);

        KGuid physicalLogId;
        physicalLogId.CreateNew();

        // Don't care if this fails
        SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));

        IPhysicalLogHandle::SPtr physicalLog;
        status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
        VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);
        VerifyState(logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            0,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE);


        KGuid logicalLogId;
        logicalLogId.CreateNew();
        KString::SPtr logicalLogFileName;
        GenerateUniqueFilename(logicalLogFileName);

        ILogicalLog::SPtr logicalLog;
        status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogFileName, logicalLog);
        VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);
        VerifyState(
            logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE,
            nullptr,
            -1,
            -1,
            FALSE,
            nullptr,
            FALSE,
            FALSE,
            dynamic_cast<LogicalLog*>(logicalLog.RawPtr()),
            TRUE);
        VERIFY_IS_TRUE(logicalLog->IsFunctional);
        VERIFY_ARE_EQUAL(0, logicalLog->Length);
        VERIFY_ARE_EQUAL(0, logicalLog->ReadPosition);
        VERIFY_ARE_EQUAL(0, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(-1, logicalLog->HeadTruncationPosition);

        //
        // Test 1: Write 100000 bytes and then close the log. Reopen and then truncate
        //         tail at 80000. Next write and flush 1000 more. Verify no failure on last 
        //         write.
        //
        KBuffer::SPtr buffer1;
        PUCHAR buffer1Ptr;
        AllocBuffer(1000, buffer1, buffer1Ptr);
        for (int i = 0; i < 1000; i++)
        {
            buffer1Ptr[i] = 1;
        }

        for (int i = 0; i < 100; i++)
        {
            status = SyncAwait(logicalLog->AppendAsync(*buffer1, 0, buffer1->QuerySize(), CancellationToken::None));
            VERIFY_STATUS_SUCCESS("AppendAsync", status);
            VERIFY_ARE_EQUAL((i + 1) * 1000, logicalLog->WritePosition);
        }

        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("FlushWithMarker", status);

        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogFileName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

        status = SyncAwait(logicalLog->TruncateTail(80000, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("TruncateTail", status);

        status = SyncAwait(logicalLog->AppendAsync(*buffer1, 0, buffer1->QuerySize(), CancellationToken::None));
        VERIFY_STATUS_SUCCESS("AppendAsync", status);
        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("FlushWithMarker", status);

        // Prove close down
        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("PhysicalLog::CloseAsync", status);
        physicalLog = nullptr;

        status = SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::DeletePhysicalLogAsync", status);

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::CloseAsync", status);
        logManager = nullptr;
    }

    VOID LogTestBase::ReadAheadCacheTest(__in KtlLoggerMode)
    {
        NTSTATUS status;

        ILogManagerHandle::SPtr logManager;
        status = CreateAndOpenLogManager(logManager);
        VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);

        LogManager* logManagerService = nullptr;
        {
            LogManagerHandle* logManagerServiceHandle = dynamic_cast<LogManagerHandle*>(logManager.RawPtr());
            if (logManagerServiceHandle != nullptr)
            {
                logManagerService = logManagerServiceHandle->owner_.RawPtr();
            }
        }
        VerifyState(
            logManagerService,
            1,
            0,
            TRUE);

        KString::SPtr physicalLogName;
        GenerateUniqueFilename(physicalLogName);

        KGuid physicalLogId;
        physicalLogId.CreateNew();

        // Don't care if this fails
        SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None)); 

        IPhysicalLogHandle::SPtr physicalLog;
        status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
        VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);

        VerifyState(
            logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            0,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE);

        KString::SPtr logicalLogName;
        GenerateUniqueFilename(logicalLogName);

        KGuid logicalLogId;
        logicalLogId.CreateNew();

        ILogicalLog::SPtr logicalLog;
        status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);
        VerifyState(
            logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE,
            nullptr,
            -1,
            -1,
            FALSE,
            nullptr,
            FALSE,
            FALSE,
            dynamic_cast<LogicalLog*>(logicalLog.RawPtr()),
            TRUE);
        VERIFY_ARE_EQUAL(0, logicalLog->Length);
        VERIFY_ARE_EQUAL(0, logicalLog->ReadPosition);
        VERIFY_ARE_EQUAL(0, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(-1, logicalLog->HeadTruncationPosition);

        static const LONG MaxLogBlkSize = 1024 * 1024;

        //
        // Write a nice pattern in the log which would be easy to validate the data
        //
        LONG dataSize = 16 * 1024 * 1024;     // 16MB
        LONG chunkSize = 1024;
        LONG prefetchSize = 1024 * 1024;
        LONG loops = dataSize / chunkSize;

        KBuffer::SPtr bufferBigK, buffer1K;
        PUCHAR bufferBig, buffer1;

        AllocBuffer(2 * prefetchSize, bufferBigK, bufferBig);
        AllocBuffer(chunkSize, buffer1K, buffer1);

        for (LONG i = 0; i < loops; i++)
        {
            LONGLONG pos = i * chunkSize;
            BuildDataBuffer(*buffer1K, pos);
            //
            // TODO: Fix buffer1K to be correct size. Add 1 to work around bug in KFileStream::WriteAsync
            //       where a full KBuffer cannot be written
            //
            status = SyncAwait(logicalLog->AppendAsync(*buffer1K, 0, buffer1K->QuerySize(), CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::AppendAsync", status);
        }

        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::FlushWithMarkerAsync", status);

        //
        // Verify length
        //
        VERIFY_IS_TRUE(logicalLog->GetLength() == dataSize);
        VERIFY_IS_TRUE(logicalLog->GetWritePosition() == dataSize);

        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        //
        // Reopen to verify length
        //
        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

        //
        // Verify length
        //
        VERIFY_IS_TRUE(logicalLog->GetLength() == dataSize);
        VERIFY_IS_TRUE(logicalLog->GetWritePosition() == dataSize);

        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        //
        // Test 1: Sequentially read through file and verify that data is correct
        //
        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 1: Sequentially read through file and verify that data is correct");
            ILogicalLogReadStream::SPtr stream;

            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            status = logicalLog->CreateReadStream(stream, prefetchSize);
            VERIFY_STATUS_SUCCESS("LogicalLog::CreateReadStream", status);
            VERIFY_IS_TRUE(stream->GetLength() == logicalLog->GetLength());

            for (LONG i = 0; i < loops; i++)
            {
                ULONG read;
                status = SyncAwait(stream->ReadAsync(*buffer1K, read, 0, chunkSize));
                VERIFY_STATUS_SUCCESS("LogicalLogReadStream::ReadAsync", status);

                LONGLONG pos = i * chunkSize;
                ValidateDataBuffer(*buffer1K, read, 0, pos);
            }

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            logicalLog = nullptr;
        }

        //
        // Test 2: Read at the end of the file to trigger a prefetch read beyond the end of the file is where the 
        //         last prefetch isn't a full record.
        //
        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 2: Read at the end of the file to trigger a prefetch read beyond the end of the file is where the last prefetch isn't a full record.");
            ILogicalLogReadStream::SPtr stream;
            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            status = logicalLog->CreateReadStream(stream, prefetchSize);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(stream->GetLength() == logicalLog->GetLength());

            LONGLONG pos = dataSize - (prefetchSize + (prefetchSize / 2));
            stream->SetPosition(pos/*, SeekOrigin.Begin*/);

            ULONG read;
            status = SyncAwait(stream->ReadAsync(*buffer1K, read, 0, chunkSize));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            ValidateDataBuffer(*buffer1K, read, 0, pos);

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            logicalLog = nullptr;
        }

        //
        // Test 3: Read at the end of the file to trigger a prefetch read that is exactly the end of the file.
        //
        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 3: Read at the end of the file to trigger a prefetch read that is exactly the end of the file.");
            ILogicalLogReadStream::SPtr stream;
            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            status = logicalLog->CreateReadStream(stream, prefetchSize);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(stream->GetLength() == logicalLog->GetLength());

            LONGLONG pos = dataSize - 2 * prefetchSize;
            stream->SetPosition(pos/*, SeekOrigin.Begin*/);

            ULONG read;
            status = SyncAwait(stream->ReadAsync(*buffer1K, read, 0, chunkSize));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            ValidateDataBuffer(*buffer1K, read, 0, pos);

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            logicalLog = nullptr;
        }

        //
        // Test 4: Read at end of file where read isn't a full record and prefetch read is out of bounds.
        //
        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 4: Read at end of file where read isn't a full record and prefetch read is out of bounds.");
            ILogicalLogReadStream::SPtr stream;
            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            status = logicalLog->CreateReadStream(stream, prefetchSize);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(stream->GetLength() == logicalLog->GetLength());

            LONGLONG pos = dataSize - (prefetchSize / 2);
            stream->SetPosition(pos/*, SeekOrigin.Begin*/);
            ULONG read;
            status = SyncAwait(stream->ReadAsync(*buffer1K, read, 0, chunkSize));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            ValidateDataBuffer(*buffer1K, read, 0, pos);

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            logicalLog = nullptr;
        }

        //
        // Test 5: Read beyond end of file.
        //
        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 5: Read beyond end of file.");
            ILogicalLogReadStream::SPtr stream;
            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            status = logicalLog->CreateReadStream(stream, prefetchSize);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(stream->GetLength() == logicalLog->GetLength());

            LONGLONG pos = dataSize - 1;
            stream->SetPosition(pos /*, SeekOrigin.Begin*/);
            ULONG read;
            status = SyncAwait(stream->ReadAsync(*buffer1K, read, 0, chunkSize));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            ValidateDataBuffer(*buffer1K, read, 0, pos);

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            logicalLog = nullptr;
        }

        //
        // Test 6: Random access reads of less than a prefetch record size and verify that data is correct
        //
        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 6: Random access reads of less than a prefetch record size and verify that data is correct");
            Common::Random random((int)GetTickCount64());

            ILogicalLogReadStream::SPtr stream;
            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            status = logicalLog->CreateReadStream(stream, prefetchSize);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(stream->GetLength() == logicalLog->GetLength());

            // TODO: replace 32 with 1024. Workaround for Rich's bug
            for (LONG i = 0; i < 32; i++)
            {
                ULONG len = random.Next() % (prefetchSize - 1);
                KBuffer::SPtr buffer;
                PUCHAR b;
                AllocBuffer(len, buffer, b);

                //
                // Pick a random place but do not go beyond end of file
                // or into the space already truncated head
                //
                LONGLONG r = random.Next();
                LONGLONG m = (dataSize - (prefetchSize + 4096 + len));
                LONGLONG pos = (r % m);
                pos = pos + (prefetchSize + 4096);
                stream->SetPosition(pos/*, SeekOrigin.Begin*/);

                // TODO: Remove this when I feel good about the math
                CODING_ERROR_ASSERT(pos >= (prefetchSize + 4096));

                TestCommon::TestSession::WriteInfo(TraceComponent, "pos: {0}, len: {1}, dataSize {2}", pos, len, dataSize);

                ULONG read;
                status = SyncAwait(stream->ReadAsync(*buffer, read, 0, len));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                CODING_ERROR_ASSERT(len == read);
                VERIFY_IS_TRUE(len == read);

                ValidateDataBuffer(*buffer, read, 0, pos);
            }

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logicalLog = nullptr;
        }

        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 6a: Specific read pattern");
            ILogicalLogReadStream::SPtr stream;
            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            status = logicalLog->CreateReadStream(stream, prefetchSize);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(stream->GetLength() == logicalLog->GetLength());

            LONG count = 56;
            KArray<LONG> pos1(GetThisAllocator(), count, count);
            KArray<LONG> len1(GetThisAllocator(), count, count);
            pos1.SetCount(count);
            len1.SetCount(count);

            pos1[0] = 6643136;
            len1[0] = 284158;
            pos1[1] = 8749250;
            len1[1] = 719631;
            pos1[2] = 7968798;
            len1[2] = 217852;
            pos1[3] = 5589085;
            len1[3] = 37828;
            pos1[4] = 8951191;
            len1[4] = 658471;
            pos1[5] = 2036603;
            len1[5] = 779626;
            pos1[6] = 13515405;
            len1[6] = 753509;
            pos1[7] = 2026546;
            len1[7] = 127354;
            pos1[8] = 12065124;
            len1[8] = 485889;
            pos1[9] = 2922527;
            len1[9] = 748745;
            pos1[10] = 12362362;
            len1[10] = 324286;
            pos1[11] = 15079374;
            len1[11] = 411635;
            pos1[12] = 308588;
            len1[12] = 515382;
            pos1[13] = 7394979;
            len1[13] = 535056;
            pos1[14] = 9878541;
            len1[14] = 445104;
            pos1[15] = 6615835;
            len1[15] = 924314;
            pos1[16] = 11835666;
            len1[16] = 259902;
            pos1[17] = 8862263;
            len1[17] = 949164;
            pos1[18] = 5386414;
            len1[18] = 517708;
            pos1[19] = 11259477;
            len1[19] = 675057;
            pos1[20] = 14395727;
            len1[20] = 973734;
            pos1[21] = 9391625;
            len1[21] = 1021568;
            pos1[22] = 298673;
            len1[22] = 858879;
            pos1[23] = 11966559;
            len1[23] = 170338;
            pos1[24] = 5228510;
            len1[24] = 331768;
            pos1[25] = 12273354;
            len1[25] = 711678;
            pos1[26] = 13006046;
            len1[26] = 865901;
            pos1[27] = 8447558;
            len1[27] = 519960;
            pos1[28] = 12824484;
            len1[28] = 10713;
            pos1[29] = 3156718;
            len1[29] = 55952;
            pos1[30] = 9152582;
            len1[30] = 1018119;
            pos1[31] = 365742;
            len1[31] = 953577;
            pos1[32] = 5241993;
            len1[32] = 706563;
            pos1[33] = 32648;
            len1[33] = 791071;
            pos1[34] = 8868714;
            len1[34] = 983844;
            pos1[35] = 2129328;
            len1[35] = 658494;
            pos1[36] = 246918;
            len1[36] = 101502;
            pos1[37] = 6577103;
            len1[37] = 266030;
            pos1[38] = 12713793;
            len1[38] = 814882;
            pos1[39] = 7880614;
            len1[39] = 107285;
            pos1[40] = 12539585;
            len1[40] = 898134;
            pos1[41] = 1772169;
            len1[41] = 145642;
            pos1[42] = 1857848;
            len1[42] = 971131;
            pos1[43] = 6954824;
            len1[43] = 620271;
            pos1[44] = 6379690;
            len1[44] = 231407;
            pos1[45] = 11807777;
            len1[45] = 576393;
            pos1[46] = 7501047;
            len1[46] = 291417;
            pos1[47] = 5570240;
            len1[47] = 763674;
            pos1[48] = 15955814;
            len1[48] = 1591;
            pos1[49] = 7792962;
            len1[49] = 967045;
            pos1[50] = 2646688;
            len1[50] = 277177;
            pos1[51] = 12045117;
            len1[51] = 820311;
            pos1[52] = 8775100;
            len1[52] = 723454;
            pos1[53] = 13038687;
            len1[53] = 699561;
            pos1[54] = 1578657;
            len1[54] = 591083;
            pos1[55] = 3419389;
            len1[55] = 778550;

            for (LONG i = 0; i < count; i++)
            {
                ULONG len = len1[i];
                KBuffer::SPtr buffer;
                PUCHAR b;
                AllocBuffer(len, buffer, b);

                //
                // Pick a random place but do not go beyond end of file
                //
                LONGLONG pos = pos1[i];
                stream->SetPosition(pos/*, SeekOrigin.Begin*/);

                ULONG read;
                status = SyncAwait(stream->ReadAsync(*buffer, read, 0, len));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_TRUE(len == read);

                ValidateDataBuffer(*buffer, read, 0, pos);
            }

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logicalLog = nullptr;
        }

        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 6b: Specific read pattern");
            ILogicalLogReadStream::SPtr stream;
            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            status = logicalLog->CreateReadStream(stream, prefetchSize);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(stream->GetLength() == logicalLog->GetLength());

            for (LONG i = 0; i < 4; i++)
            {
                ULONG len = 996198;
                KBuffer::SPtr buffer;
                PUCHAR b;
                AllocBuffer(len, buffer, b);

                //
                // Pick a random place but do not go beyond end of file
                //
                LONGLONG pos = 15117795;
                stream->SetPosition(pos/*, SeekOrigin.Begin*/);

                ULONG read;
                status = SyncAwait(stream->ReadAsync(*buffer, read, 0, len));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_TRUE(len == read);

                ValidateDataBuffer(*buffer, read, 0, pos);
            }

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            logicalLog = nullptr;
        }

        //
        // Test 7: Random access reads of more than a prefetch record size and verify that data is correct
        //
        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 7: Random access reads of more than a prefetch record size and verify that data is correct");
            Common::Random random((int)GetTickCount64());

            ILogicalLogReadStream::SPtr stream;
            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            status = logicalLog->CreateReadStream(stream, prefetchSize);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(stream->GetLength() == logicalLog->GetLength());

            // TODO: replace 32 with 1024 when Rich fixes
            for (LONG i = 0; i < 32; i++)
            {
                ULONG len = random.Next() % (3 * prefetchSize);

                KBuffer::SPtr buffer;
                PUCHAR b;
                AllocBuffer(len, buffer, b);

                //
                // Pick a random place but do not go beyond end of file
                //
                LONGLONG r = random.Next();
                LONGLONG m = (dataSize - (prefetchSize + 4096 + len));
                LONGLONG pos = (r % m);
                pos = pos + (prefetchSize + 4096);

                // TODO: Remove this when I feel good about the math
                CODING_ERROR_ASSERT(pos >= (prefetchSize + 4096));

                stream->SetPosition(pos/*, SeekOrigin.Begin*/);

                ULONG read;
                status = SyncAwait(stream->ReadAsync(*buffer, read, 0, len));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_TRUE(len == read);

                ValidateDataBuffer(*buffer, read, 0, pos);
            }

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            logicalLog = nullptr;
        }


        //
        // Test 8: Read section of file and then truncate head in the middle of the read buffer. Ensure reading truncated 
        //         space fails and untruncated succeeds
        //
        //
        // Test 9: Read section of file and then truncate head in the middle of the prefetch read buffer. Ensure reading truncated 
        //         space fails and untruncated succeeds
        //
        {
            LONGLONG pos;
            ULONG read;

            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 9: Read section of file and then truncate head in the middle of the prefetch read buffer. Ensure reading truncated space fails and untruncated succeeds");
            ILogicalLogReadStream::SPtr stream;
            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            status = logicalLog->CreateReadStream(stream, prefetchSize);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(stream->GetLength() == logicalLog->GetLength());

            //
            // Perform gratuitous read to ensure read stream is
            // initialized
            //
            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, chunkSize));

            //
            // truncate head in the middle of the prefetch buffer
            //
            TestCommon::TestSession::WriteInfo(TraceComponent, "Truncate head at {0}", prefetchSize + 4096);
            status = SyncAwait(logicalLog->TruncateHead(prefetchSize + 4096));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

#if 0  // Test only for KtlLogger
            //
            // reads should not work
            //
            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, chunkSize));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == 0);

            pos = 4096;
            stream->SetPosition(pos/*, SeekOrigin.Begin*/);
            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, chunkSize));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == 0);

            pos = prefetchSize + 256;
            stream->SetPosition(pos/*, SeekOrigin.Begin*/);
            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, chunkSize));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == 0);
#endif

            //
            // read in untruncated space
            //
            pos = prefetchSize + 4096 + 512;
            stream->SetPosition(pos/*, SeekOrigin.Begin*/);
            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, chunkSize));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            ValidateDataBuffer(*bufferBigK, read, 0, pos);

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            logicalLog = nullptr;
        }

        //
        // Test 10: Read section of file and then truncate tail in the middle of the read buffer. Ensure reading truncated 
        //          space fails and untruncated succeeds
        //
        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 10: Read section of file and then truncate tail in the middle of the read buffer. Ensure reading truncated space fails and untruncated succeeds");
            ILogicalLogReadStream::SPtr stream;
            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            status = logicalLog->CreateReadStream(stream, prefetchSize);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(stream->GetLength() == logicalLog->GetLength());

            //
            // Test 10a: Fill read buffer and truncate tail ahead of it. Verify that only data before truncate tail
            //           is read and is correct
            //
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 10a: Fill read buffer and truncate tail ahead of it. Verify that only data before truncate tail is read and is correct");

            LONGLONG pos = dataSize - (16384 + 4096);
            LONGLONG tailTruncatePos = pos + (8192 + 4096);

            stream->SetPosition(pos/*, SeekOrigin.Begin*/);

            ULONG read;
            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, 4096));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            ValidateDataBuffer(*bufferBigK, read, 0, pos);

            pos += read;

            status = SyncAwait(logicalLog->TruncateTail(tailTruncatePos, CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, 16384));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == 8192);
            ValidateDataBuffer(*bufferBigK, read, 0, pos);

            //
            // Overwrite end of file to bring back to original state
            //
            RestoreEOFState(*logicalLog, dataSize, prefetchSize);

            //
            // Test 10b: Fill read buffer and truncate before current read pointer. Verify no data read
            //
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 10b: Fill read buffer and truncate before current read pointer. Verify no data read");
            pos = dataSize - (16384 + 4096);
            stream->SetPosition(pos/*, SeekOrigin.Begin*/);

            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, 4096));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == 4096);

            ValidateDataBuffer(*bufferBigK, read, 0, pos);

            pos += read;

            tailTruncatePos = pos - 128;
            status = SyncAwait(logicalLog->TruncateTail(tailTruncatePos, CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, 4096));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == 0);

            //
            // Overwrite end of file to bring back to original state
            //
            RestoreEOFState(*logicalLog, dataSize, prefetchSize);

            //
            // Test 10c: Fill read buffer and overwrite it with different data. Continue reading and verify that
            //           new data is read
            //
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 10c: Fill read buffer and overwrite it with different data. Continue reading and verify that new data is read");
            pos = dataSize - (16384 + 4096);
            stream->SetPosition(pos/*, SeekOrigin.Begin*/);

            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, 4096));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == 4096);

            ValidateDataBuffer(*bufferBigK, read, 0, pos);

            pos += read;

            tailTruncatePos = pos - 128;
            status = SyncAwait(logicalLog->TruncateTail(tailTruncatePos, CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            LONGLONG fillSize = dataSize - tailTruncatePos;
            KBuffer::SPtr fillBuffer;
            PUCHAR b;
            AllocBuffer(static_cast<LONG>(fillSize), fillBuffer, b);
            BuildDataBuffer(*fillBuffer, tailTruncatePos, 5);

            status = SyncAwait(logicalLog->AppendAsync(*fillBuffer, 0, (LONG)fillSize, CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, 256));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == 256);

            ValidateDataBuffer(*bufferBigK, read, 0, pos, 5);

            pos += read;

            //
            // Overwrite end of file to bring back to original state
            //
            RestoreEOFState(*logicalLog, dataSize, prefetchSize);

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            logicalLog = nullptr;
        }

        //
        // Test 11: Read section of file and then truncate tail in the middle of the prefetch read buffer. Ensure reading truncated 
        //          space fails and untruncated succeeds
        //
        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 11: Read section of file and then truncate tail in the middle of the prefetch read buffer. Ensure reading truncated space fails and untruncated succeeds");
            ILogicalLogReadStream::SPtr stream;
            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            status = logicalLog->CreateReadStream(stream, prefetchSize);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(stream->GetLength() == logicalLog->GetLength());

            //
            // Test 11a: Fill read buffer and truncate tail ahead of it. Verify that only data before truncate tail
            //           is read and is correct
            //
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 11a: Fill read buffer and truncate tail ahead of it. Verify that only data before truncate tail is read and is correct");
            LONGLONG pos = dataSize - (prefetchSize + 16384 + 4096);
            LONGLONG tailTruncatePos = pos + (prefetchSize + 8192 + 4096);

            stream->SetPosition(pos/*, SeekOrigin.Begin*/);

            ULONG read;
            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, 4096));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            ValidateDataBuffer(*bufferBigK, read, 0, pos);

            pos += read;

            status = SyncAwait(logicalLog->TruncateTail(tailTruncatePos, CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, 16384 + prefetchSize));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == static_cast<ULONG>(8192 + prefetchSize));
            ValidateDataBuffer(*bufferBigK, read, 0, pos);

            //
            // Overwrite end of file to bring back to original state
            //
            RestoreEOFState(*logicalLog, dataSize, prefetchSize);

            //
            // Test 11c: Fill read buffer and overwrite it with different data. Continue reading and verify that
            //           new data is read
            //
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 11c: Fill read buffer and overwrite it with different data. Continue reading and verify that new data is read");
            pos = dataSize - (prefetchSize + 16384 + 4096);
            stream->SetPosition(pos/*, SeekOrigin.Begin*/);

            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, 4096));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == 4096);

            ValidateDataBuffer(*bufferBigK, read, 0, pos);

            pos += read;

            tailTruncatePos = (pos + prefetchSize) - 128;
            status = SyncAwait(logicalLog->TruncateTail(tailTruncatePos, CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            LONGLONG fillSize = dataSize - tailTruncatePos;
            KBuffer::SPtr fillBuffer;
            PUCHAR b;

            AllocBuffer(static_cast<ULONG>(fillSize), fillBuffer, b);

            BuildDataBuffer(*fillBuffer, tailTruncatePos, 5);
            status = SyncAwait(logicalLog->AppendAsync(*fillBuffer, 0, (LONG)fillSize, CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            LONGLONG read1Len = tailTruncatePos - pos;

            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, (LONG)read1Len));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == (ULONG)read1Len);
            ValidateDataBuffer(*bufferBigK, read, 0, pos);

            pos += read;

            LONGLONG read2Len = dataSize - tailTruncatePos;

            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, (LONG)read2Len));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == (ULONG)read2Len);
            ValidateDataBuffer(*bufferBigK, read, 0, pos, 5);

            pos += read;

            //
            // Overwrite end of file to bring back to original state
            //
            RestoreEOFState(*logicalLog, dataSize, prefetchSize);

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            logicalLog = nullptr;
        }

        //
        // Test 12: Write at the end of the file within the read buffer and verify that read succeeds with correct data
        //
        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 12: Write at the end of the file within the read buffer and verify that read succeeds with correct data");
            ILogicalLogReadStream::SPtr stream;
            ULONG read;

            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            status = logicalLog->CreateReadStream(stream, prefetchSize);
            VERIFY_STATUS_SUCCESS("LogicalLog::CreateReadStream", status);
            VERIFY_IS_TRUE(stream->GetLength() == logicalLog->GetLength());

            //
            // Perform gratuitous read to ensure read stream is
            // initialized
            //
            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, chunkSize));

            //
            // Truncate tail to make space at the end of the file and then read at the end of the file to fill
            // read buffer. Write at end of file and then continue to read the new data.
            //
            LONGLONG pos = dataSize - (16384 + 4096);
            LONGLONG tailTruncatePos = pos + 4096;
            status = SyncAwait(logicalLog->TruncateTail(tailTruncatePos, CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            stream->SetPosition(pos/*, SeekOrigin.Begin*/);

            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, 2048));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == 2048);

            ValidateDataBuffer(*bufferBigK, read, 0, pos);

            pos += read;

            LONGLONG fillSize = dataSize - tailTruncatePos;
            KBuffer::SPtr fillBuffer;
            PUCHAR b;
            AllocBuffer(static_cast<ULONG>(fillSize + 1), fillBuffer, b);

            BuildDataBuffer(*fillBuffer, tailTruncatePos, 7);
            status = SyncAwait(logicalLog->AppendAsync(*fillBuffer, 0, (LONG)fillSize, CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, 4096));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == 4096);

            ValidateDataBuffer(*bufferBigK, 2048, 0, pos);
            ValidateDataBuffer(*bufferBigK, read - 2048, 2048, pos + 2048, 7);

            pos += read;

            //
            // Overwrite end of file to bring back to original state
            //
            RestoreEOFState(*logicalLog, dataSize, prefetchSize);

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            logicalLog = nullptr;
        }

        //
        // Test 13: Write at the end of the file within the prefetch buffer and verify that read succeeds with correct data
        //
        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Test 13: Write at the end of the file within the prefetch buffer and verify that read succeeds with correct data");
            ILogicalLogReadStream::SPtr stream;
            ULONG read;

            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            status = logicalLog->CreateReadStream(stream, prefetchSize);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(stream->GetLength() == logicalLog->GetLength());

            //
            // Perform gratuitous read to ensure read stream is
            // initialized
            //
            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, chunkSize));

            //
            // Truncate tail to make space at the end of the file and then read at the end of the file to fill
            // read buffer. Write at end of file and then continue to read the new data.
            //
            LONGLONG pos = dataSize - (16384 + 4096 + prefetchSize);
            LONGLONG tailTruncatePos = pos + 4096 + prefetchSize;
            status = SyncAwait(logicalLog->TruncateTail(tailTruncatePos, CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            stream->SetPosition(pos/*, SeekOrigin.Begin*/);

            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, 2048));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(read == 2048);
            ValidateDataBuffer(*bufferBigK, read, 0, pos);

            pos += read;

            LONGLONG fillSize = dataSize - tailTruncatePos;
            KBuffer::SPtr fillBuffer;
            PUCHAR b;
            AllocBuffer(static_cast<ULONG>(fillSize), fillBuffer, b);

            BuildDataBuffer(*fillBuffer, tailTruncatePos, 9);
            status = SyncAwait(logicalLog->AppendAsync(*fillBuffer, 0, (LONG)fillSize, CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            ULONG readExpected = (16384 + 2048 + prefetchSize);

            status = SyncAwait(stream->ReadAsync(*bufferBigK, read, 0, readExpected));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(read == readExpected);
            LONG firstRead = prefetchSize + 2048;
            ValidateDataBuffer(*bufferBigK, firstRead, 0, pos);
            ValidateDataBuffer(*bufferBigK, readExpected - firstRead, firstRead, pos + firstRead, 9);
            pos += read;

            //
            // Overwrite end of file to bring back to original state
            //
            RestoreEOFState(*logicalLog, dataSize, prefetchSize);

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            logicalLog = nullptr;

            status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("PhysicalLog::CloseAsync", status);
            physicalLog = nullptr;

            //* Cleanup
            status = SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogManager::DeletePhysicalLogAsync", status);

            status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogManager::CloseAsync", status);
            logManager = nullptr;
        }
    }

    VOID LogTestBase::TruncateInDataBufferTest(__in KtlLoggerMode ktlLoggerMode)
    {
        NTSTATUS status;

        ILogManagerHandle::SPtr logManager;
        status = CreateAndOpenLogManager(logManager);
        VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);

        LogManager* logManagerService = nullptr;
        {
            LogManagerHandle* logManagerServiceHandle = dynamic_cast<LogManagerHandle*>(logManager.RawPtr());
            if (logManagerServiceHandle != nullptr)
            {
                logManagerService = logManagerServiceHandle->owner_.RawPtr();
            }
        }
        VerifyState(
            logManagerService,
            1,
            0,
            TRUE);

        KString::SPtr physicalLogName;
        GenerateUniqueFilename(physicalLogName);

        KGuid physicalLogId;
        physicalLogId.CreateNew();

        // Don't care if this fails
        SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));

        IPhysicalLogHandle::SPtr physicalLog;
        status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
        VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);
        VerifyState(logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            0,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE);


        KGuid logicalLogId;
        logicalLogId.CreateNew();
        KString::SPtr logicalLogFileName;
        GenerateUniqueFilename(logicalLogFileName);

        ILogicalLog::SPtr logicalLog;
        status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogFileName, logicalLog);
        VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);
        VerifyState(
            logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE,
            nullptr,
            -1,
            -1,
            FALSE,
            nullptr,
            FALSE,
            FALSE,
            dynamic_cast<LogicalLog*>(logicalLog.RawPtr()),
            TRUE);
        VERIFY_IS_TRUE(logicalLog->IsFunctional);
        VERIFY_ARE_EQUAL(0, logicalLog->Length);
        VERIFY_ARE_EQUAL(0, logicalLog->ReadPosition);
        VERIFY_ARE_EQUAL(0, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(-1, logicalLog->HeadTruncationPosition);

        //
        // Test 1: Write a whole bunch of stuff to the log and then flush to be sure the logical log
        //         write buffer is empty. Next write less than a write buffer's worth to fill the write buffer
        //         with some data but do not flush. TruncateTail at an offset that is in the write buffer and
        //         verify success.
        //
        KBuffer::SPtr buffer1;
        PUCHAR buffer1Ptr;
        AllocBuffer(1000, buffer1, buffer1Ptr);
        for (int i = 0; i < 1000; i++)
        {
            buffer1Ptr[i] = 1;
        }

        for (int i = 0; i < 100; i++)
        {
            status = SyncAwait(logicalLog->AppendAsync(*buffer1, 0, buffer1->QuerySize(), CancellationToken::None));
            VERIFY_STATUS_SUCCESS("AppendAsync", status);
            VERIFY_ARE_EQUAL((i + 1) * 1000, logicalLog->WritePosition);
        }
        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("FlushWithMarker", status);

        status = SyncAwait(logicalLog->AppendAsync(*buffer1, 0, buffer1->QuerySize(), CancellationToken::None));
        VERIFY_STATUS_SUCCESS("AppendAsync", status);
        auto truncateTailPosition = logicalLog->WritePosition - 50;

        status = SyncAwait(logicalLog->TruncateTail(truncateTailPosition, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("TruncateTail", status);

        status = SyncAwait(logicalLog->CloseAsync());
        VERIFY_STATUS_SUCCESS("logicalLog->CloseAsync", status);
        logicalLog = nullptr;

        //
        // prove truncate tail succeeded
        //
        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogFileName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

        VERIFY_ARE_EQUAL(truncateTailPosition, logicalLog->WritePosition);

        status = SyncAwait(logicalLog->CloseAsync());
        VERIFY_STATUS_SUCCESS("logicalLog->CloseAsync", status);
        logicalLog = nullptr;


        //
        // Test 2: Prove that TruncateHead eventually happens
        //
        status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogFileName, logicalLog);
        VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

        for (int i = 0; i < 100; i++)
        {
            status = SyncAwait(logicalLog->AppendAsync(*buffer1, 0, buffer1->QuerySize(), CancellationToken::None));
            VERIFY_STATUS_SUCCESS("AppendAsync", status);
        }
        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("FlushWithMarker", status);

        status = SyncAwait(logicalLog->AppendAsync(*buffer1, 0, buffer1->QuerySize(), CancellationToken::None));
        VERIFY_STATUS_SUCCESS("AppendAsync", status);
        auto truncateHeadPosition = logicalLog->WritePosition - 50;

        status = SyncAwait(logicalLog->FlushAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("FlushAsync", status);
        status = SyncAwait(logicalLog->TruncateHead(truncateHeadPosition));
        VERIFY_STATUS_SUCCESS("TruncateHead", status);

        //
        // Write after the truncate head to help force the truncate to happen
        //
        status = SyncAwait(logicalLog->AppendAsync(*buffer1, 0, buffer1->QuerySize(), CancellationToken::None));
        VERIFY_STATUS_SUCCESS("AppendAsync", status);
        status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("FlushWithMarker", status);

        //
        // prove HeadTruncationPosition cannot be moved backwards
        //
        status = SyncAwait(logicalLog->TruncateHead(truncateHeadPosition - 1));
        VERIFY_STATUS_SUCCESS("TruncateHead", status);
        VERIFY_ARE_EQUAL(truncateHeadPosition, logicalLog->HeadTruncationPosition);

        KNt::Sleep(60 * 1000);  // todo: Can I make this shorter????

        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("PhysicalLog::CloseAsync", status);
        physicalLog = nullptr;

        //
        // prove truncate head succeeded
        //
        KtlLogManager::SPtr physicalLogManager;

        status = CreateKtlLogManager(ktlLoggerMode, GetThisAllocator(), physicalLogManager);

        VERIFY_STATUS_SUCCESS("KtlLogManager::Create", status);
        status = SyncAwait(physicalLogManager->OpenLogManagerAsync(nullptr, nullptr));
        VERIFY_STATUS_SUCCESS("OpenLogManagerAsync", status);

        KtlLogContainer::SPtr physicalLogContainer;
        KtlLogManager::AsyncOpenLogContainer::SPtr openLogContainerContext;
        status = physicalLogManager->CreateAsyncOpenLogContainerContext(openLogContainerContext);
        VERIFY_STATUS_SUCCESS("CreateAsyncOpenLogContainerContext", status);
        status = SyncAwait(openLogContainerContext->OpenLogContainerAsync(*physicalLogName, physicalLogId, nullptr, physicalLogContainer));
        VERIFY_STATUS_SUCCESS("OpenLogContainer", status);

        KtlLogStream::SPtr physicalLogStream;
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openLogStreamContext;
        status = physicalLogContainer->CreateAsyncOpenLogStreamContext(openLogStreamContext);
        VERIFY_STATUS_SUCCESS("CreateAsyncOpenLogStreamContext", status);
        ULONG maxMS;
        status = SyncAwait(openLogStreamContext->OpenLogStreamAsync(logicalLogId, &maxMS, physicalLogStream, nullptr));
        VERIFY_STATUS_SUCCESS("OpenLogStream", status);

        KtlLogStream::AsyncQueryRecordRangeContext::SPtr queryRecordRangeContext;
        status = physicalLogStream->CreateAsyncQueryRecordRangeContext(queryRecordRangeContext);
        VERIFY_STATUS_SUCCESS("CreateAsyncQueryRecordRangeContext", status);
        
        KSynchronizer synch;
        RvdLogAsn truncationLsn;
        queryRecordRangeContext->StartQueryRecordRange(nullptr, nullptr, &truncationLsn, nullptr, synch.AsyncCompletionCallback());
        status = synch.WaitForCompletion();
        VERIFY_STATUS_SUCCESS("QueryRecordRange", status);
        VERIFY_ARE_NOT_EQUAL(static_cast<RvdLogAsn>(0), truncationLsn);

        status = SyncAwait(physicalLogStream->CloseAsync(nullptr));
        VERIFY_STATUS_SUCCESS("CloseAsync", status);
        physicalLogStream = nullptr;
        status = SyncAwait(physicalLogContainer->CloseAsync(nullptr));
        VERIFY_STATUS_SUCCESS("CloseAsync", status);
        physicalLogContainer = nullptr;
        status = SyncAwait(physicalLogManager->CloseLogManagerAsync(nullptr));
        VERIFY_STATUS_SUCCESS("CloseLogManagerAsync", status);
        physicalLogManager = nullptr;

        status = SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::DeletePhysicalLogAsync", status);

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::CloseAsync", status);
        logManager = nullptr;
    }

    VOID LogTestBase::SequentialAndRandomStreamTest(__in KtlLoggerMode)
    {
        NTSTATUS status;

        ILogManagerHandle::SPtr logManager;
        status = CreateAndOpenLogManager(logManager);
        VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);

        LogManager* logManagerService = nullptr;
        {
            LogManagerHandle* logManagerServiceHandle = dynamic_cast<LogManagerHandle*>(logManager.RawPtr());
            if (logManagerServiceHandle != nullptr)
            {
                logManagerService = logManagerServiceHandle->owner_.RawPtr();
            }
        }
        VerifyState(
            logManagerService,
            1,
            0,
            TRUE);

        KString::SPtr physicalLogName;
        GenerateUniqueFilename(physicalLogName);

        KGuid physicalLogId;
        physicalLogId.CreateNew();

        // Don't care if this fails
        SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));

        IPhysicalLogHandle::SPtr physicalLog;
        status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
        VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);
        VerifyState(logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            0,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE);


        KGuid logicalLogId;
        logicalLogId.CreateNew();
        KString::SPtr logicalLogFileName;
        GenerateUniqueFilename(logicalLogFileName);

        ILogicalLog::SPtr logicalLog;
        status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogFileName, logicalLog);
        VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);
        VerifyState(
            logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE,
            nullptr,
            -1,
            -1,
            FALSE,
            nullptr,
            FALSE,
            FALSE,
            dynamic_cast<LogicalLog*>(logicalLog.RawPtr()),
            TRUE);
        VERIFY_IS_TRUE(logicalLog->IsFunctional);
        VERIFY_ARE_EQUAL(0, logicalLog->Length);
        VERIFY_ARE_EQUAL(0, logicalLog->ReadPosition);
        VERIFY_ARE_EQUAL(0, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(-1, logicalLog->HeadTruncationPosition);


        //
        // Write a lot of stuff to the log in short records and then read it back sequentially
        // using a random access stream and a sequential access stream. Expect the sequential
        // access stream to run much faster
        //


        KBuffer::SPtr buffer1;
        PUCHAR buffer1Ptr;
        AllocBuffer(1000, buffer1, buffer1Ptr);
        for (int i = 0; i < 1000; i++)
        {
            buffer1Ptr[i] = 1;
        }

        for (int i = 0; i < 10000; i++)
        {
            status = SyncAwait(logicalLog->AppendAsync(*buffer1, 0, buffer1->QuerySize(), CancellationToken::None));
            VERIFY_STATUS_SUCCESS("AppendAsync", status);
            VERIFY_ARE_EQUAL((i + 1) * 1000, logicalLog->WritePosition);
            status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("FlushWithMarkerAsync", status);
        }
        TestCommon::TestSession::WriteInfo(TraceComponent, "Data written....");

        ILogicalLogReadStream::SPtr rsStream;
        Common::Stopwatch rTime;
        Common::Stopwatch sTime;
        Common::Random r;
        KBuffer::SPtr buffer2;
        PUCHAR buffer2Ptr;
        AllocBuffer(10000, buffer2, buffer2Ptr);

        //
        // Verify random faster for random access
        //
        status = logicalLog->CreateReadStream(rsStream, 1024 * 1024);
        VERIFY_STATUS_SUCCESS("CreateReadStream", status);
        
        sTime.Start();
        for (int i = 0; i < 1000; i++)
        {
            LONGLONG offset = r.Next(999 * 1000);
            rsStream->SetPosition(offset);
            ULONG bytesRead;
            status = SyncAwait(rsStream->ReadAsync(*buffer2, bytesRead, 0, buffer2->QuerySize()));
            VERIFY_STATUS_SUCCESS("ReadAsync", status);
        }
        sTime.Stop();

        logicalLog->SetSequentialAccessReadSize(*rsStream, 0);
        rsStream->SetPosition(0);

        rTime.Start();
        for (int i = 0; i < 1000; i++)
        {
            LONGLONG offset = r.Next(999 * 1000);
            rsStream->SetPosition(offset);
            ULONG bytesRead;
            status = SyncAwait(rsStream->ReadAsync(*buffer2, bytesRead, 0, buffer2->QuerySize()));
            VERIFY_STATUS_SUCCESS("ReadAsync", status);
        }
        rTime.Stop();

        TestCommon::TestSession::WriteInfo(TraceComponent, "Sequential Time: {0}  Random Time: {1}", sTime.ElapsedMilliseconds, rTime.ElapsedMilliseconds);

        VERIFY_IS_TRUE(rTime.ElapsedMilliseconds < sTime.ElapsedMilliseconds);

        status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
        logicalLog = nullptr;

        status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("PhysicalLog::CloseAsync", status);
        physicalLog = nullptr;

        status = SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::DeletePhysicalLogAsync", status);

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::CloseAsync", status);
        logManager = nullptr;
    }

    ktl::Awaitable<NTSTATUS> LogTestBase::TruncateLogicalLogs(
        __in ILogicalLog::SPtr logicalLogs[],
        __in int numLogicalLogs)
    {
        static KAsyncEvent truncateLock(FALSE, TRUE);

        AsyncLockBlock(truncateLock)
        {
            for (int i = 0; i < numLogicalLogs; i++)
            {
                ILogicalLog& logicalLog = *logicalLogs[i];
                LONGLONG toTruncate = logicalLog.GetWritePosition();
                NTSTATUS status = co_await logicalLog.TruncateHead(toTruncate);
                if (status == K_STATUS_API_CLOSED)
                {
                    TestCommon::TestSession::WriteInfo(TraceComponent, "TruncateHead failure (already closed) {0}", status);
                }
                else if (!NT_SUCCESS(status))
                {
                    TestCommon::TestSession::WriteInfo(TraceComponent, "TruncateHead failure. toTruncate: {0}, status: {1}", toTruncate, status);
                    KInvariant(FALSE);
                }
                else
                {
                    TestCommon::TestSession::WriteInfo(TraceComponent, "TruncateHead success. toTruncate: {0}", toTruncate);
                }
            }
        }

        co_return STATUS_SUCCESS;
    }

    ktl::Awaitable<void> LogTestBase::ReadWriteCloseRaceWorker(
        __in ILogicalLog& logicalLog,
        __in int maxSize,
        __inout volatile ULONGLONG & numTruncates,
        __in int fileSize,
        __in int taskId,
        __in CancellationToken const & cancellationToken)
    {
        LONGLONG bytes = 0;
        Common::Random rnd;
        NTSTATUS status;
        ILogicalLog::SPtr thisLogicalLog = &logicalLog;
        LONGLONG amountWritten = 0;

        KFinally([taskId, &bytes] { TestCommon::TestSession::WriteInfo(TraceComponent, "Exit task {0} bytes {1}", taskId, bytes); });

        do
        {
            KBuffer::SPtr recordBuffer;
            auto toWrite = rnd.Next(80, maxSize);
            status = KBuffer::Create(toWrite, recordBuffer, GetThisAllocator(), GetThisAllocationTag());
            VERIFY_STATUS_SUCCESS("KBuffer::Create", status);
            
            status = co_await logicalLog.AppendAsync(*recordBuffer, 0, recordBuffer->QuerySize(), CancellationToken::None);
            if (status == K_STATUS_API_CLOSED)
            {
                TestCommon::TestSession::WriteInfo(TraceComponent, "Task {0} AppendAsync failure (already closed) {1}", taskId, status);
                co_return;
            }
            else if (!NT_SUCCESS(status))
            {
                TestCommon::TestSession::WriteInfo(TraceComponent, "Task {0} AppendAsync failure {1}", taskId, status);
                KInvariant(FALSE);
            }
            
            bytes += toWrite;
            amountWritten +=  max(toWrite, 4 * 1024); // small writes will still reserve 4k

            status = co_await logicalLog.FlushWithMarkerAsync(CancellationToken::None);
            if (status == K_STATUS_API_CLOSED)
            {
                TestCommon::TestSession::WriteInfo(TraceComponent, "Task {0} FlushWithMarkerAsync failure (already closed) {1}", taskId, status);
                co_return;
            }
            else if (!NT_SUCCESS(status))
            {
                TestCommon::TestSession::WriteInfo(TraceComponent, "Task {0} FlushWithMarkerAsync failure {1}", taskId, status);
                KInvariant(FALSE);
            }

            if (amountWritten >= (fileSize / 2))
            {
                ULONGLONG currentTruncateIteration = InterlockedIncrement64((volatile LONGLONG*) &numTruncates);

                status = co_await TruncateLogicalLogs(&thisLogicalLog, 1);
                if (!NT_SUCCESS(status))
                {
                    TestCommon::TestSession::WriteInfo(TraceComponent, "Task {0} TruncateLogicalLogs iteration {1} failure {2}", taskId, currentTruncateIteration, status);
                    KInvariant(FALSE);
                }
                else
                {
                    TestCommon::TestSession::WriteInfo(TraceComponent, "Task {0} TruncateLogicalLogs iteration {1} success.", taskId, currentTruncateIteration);
                }
            }

            LONG bytesRead;
            status = co_await logicalLog.ReadAsync(bytesRead, *recordBuffer, 0, recordBuffer->QuerySize(), 0, CancellationToken::None);
            if (status == K_STATUS_API_CLOSED)
            {
                TestCommon::TestSession::WriteInfo(TraceComponent, "Task {0} ReadAsync failure (already closed) {1}", taskId, status);
                co_return;
            }
            else if (status == STATUS_NOT_FOUND)
            {
                KInvariant(numTruncates > 0);
                TestCommon::TestSession::WriteWarning(TraceComponent, "Task {0} ReadAsync failure (not found) {1}. Expected when the logs were just fully truncated by this (or another) thread.", taskId, status);
            }
            else if (!NT_SUCCESS(status))
            {
                TestCommon::TestSession::WriteInfo(TraceComponent, "Task {0} ReadAsync failure {1}", taskId, status);
                KInvariant(FALSE);
            }
        } while (!cancellationToken.IsCancellationRequested);

        co_return;
    }

    VOID LogTestBase::ReadWriteCloseRaceTest(__in KtlLoggerMode)
    {
        NTSTATUS status;

        ILogManagerHandle::SPtr logManager;
        status = CreateAndOpenLogManager(logManager);
        VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);

        LogManager* logManagerService = nullptr;
        {
            LogManagerHandle* logManagerServiceHandle = dynamic_cast<LogManagerHandle*>(logManager.RawPtr());
            if (logManagerServiceHandle != nullptr)
            {
                logManagerService = logManagerServiceHandle->owner_.RawPtr();
            }
        }
        VerifyState(
            logManagerService,
            1,
            0,
            TRUE);

        KString::SPtr physicalLogName;
        GenerateUniqueFilename(physicalLogName);

        KGuid physicalLogId;
        physicalLogId.CreateNew();

        // Don't care if this fails
        SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));

        IPhysicalLogHandle::SPtr physicalLog;
        status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
        VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);

        const int NumAwaitables = 16;
        const int NumCloseAwaitables = NumAwaitables / 2;
        const int MaxLogBlockSize = 128 * 1024;
        CancellationTokenSource::SPtr cts;
        status = CancellationTokenSource::Create(GetThisAllocator(), GetThisAllocationTag(), cts);
        VERIFY_STATUS_SUCCESS("CancellationTokenSource::Create", status);
        Awaitable<void> awaitables[NumAwaitables];
        ILogicalLog::SPtr logicalLogs[NumAwaitables];

        for (int i = 0; i < NumAwaitables; i++)
        {
            KGuid logicalLogId;
            logicalLogId.CreateNew();

            TestCommon::TestSession::WriteInfo(TraceComponent, "Log {0} is {1}", i, Common::Guid(logicalLogId).ToString().c_str());

            KString::SPtr logFilename;
            GenerateUniqueFilename(logFilename);

            status = CreateLogicalLog(*physicalLog, logicalLogId, *logFilename, logicalLogs[i]); // todo: override other params
            VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);

            status = SyncAwait(logicalLogs[i]->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("CloseAsync", status);

            logicalLogs[i] = nullptr;

            status = OpenLogicalLog(*physicalLog, logicalLogId, *logFilename, logicalLogs[i]);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);
        }

        const int fileSize = 1024 * 1024 * 100; // todo: define this in a single place
        volatile ULONGLONG numTruncates = 0;
        for (int i = 0; i < NumAwaitables; i++)
        {
            ILogicalLog::SPtr log = logicalLogs[i];
            CancellationToken token = cts->Token;

            awaitables[i] = ReadWriteCloseRaceWorker(
                *log,
                MaxLogBlockSize,
                numTruncates,
                fileSize,
                i,
                token);
        }

        KNt::Sleep(15 * 1000);

        TestCommon::TestSession::WriteInfo(TraceComponent, "Closing logs");

        Awaitable<NTSTATUS> closeAwaitables[NumCloseAwaitables];

        for (int i = 0; i < NumCloseAwaitables; i++)
        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Async closing log {0}", i);
            closeAwaitables[i] = logicalLogs[i]->CloseAsync(CancellationToken::None);
        }

        for (int i = 0; i < NumCloseAwaitables; i++)
        {
            status = SyncAwait(closeAwaitables[i]);
            std::string msg("error closing task " + std::to_string(i));
            VERIFY_STATUS_SUCCESS(msg.c_str(), status);
        }
        TestCommon::TestSession::WriteInfo(TraceComponent, "Finished async closing logs");

        for (int i = NumCloseAwaitables; i < NumAwaitables; i++)
        {
            TestCommon::TestSession::WriteInfo(TraceComponent, "Sync closing log {0}", i);
            SyncAwait(logicalLogs[i]->CloseAsync(CancellationToken::None));
        }
        TestCommon::TestSession::WriteInfo(TraceComponent, "Finished sync closing logs");

        for (int i = 0; i < NumAwaitables; i++)
        {
            VERIFY_IS_FALSE(logicalLogs[i]->IsFunctional);
            VERIFY_IS_FALSE(dynamic_cast<LogicalLog*>(logicalLogs[i].RawPtr()) == nullptr ? FALSE : static_cast<LogicalLog*>(logicalLogs[i].RawPtr())->IsOpen());
            logicalLogs[i] = nullptr;
        }

        KNt::Sleep(250);

        // todo: is this necessary?  All tasks should have errored out by now or will on the next iteration.
        TestCommon::TestSession::WriteInfo(TraceComponent, "Canceling tokens");
        cts->Cancel();

        TestCommon::TestSession::WriteInfo(TraceComponent, "Wait for tasks");
        for (int i = 0; i < NumAwaitables; i++)
        {
            SyncAwait(awaitables[i]);
        }
        TestCommon::TestSession::WriteInfo(TraceComponent, "Tasks completed");

        status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("PhysicalLog::CloseAsync", status);
        physicalLog = nullptr;

        status = SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::DeletePhysicalLogAsync", status);

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::CloseAsync", status);
        logManager = nullptr;
    }

    VOID LogTestBase::UPassthroughErrorsTest(__in KtlLoggerMode)
    {
        NTSTATUS status;

        ILogManagerHandle::SPtr logManager;
        status = CreateAndOpenLogManager(logManager);
        VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);

        LogManager* logManagerService = nullptr;
        {
            LogManagerHandle* logManagerServiceHandle = dynamic_cast<LogManagerHandle*>(logManager.RawPtr());
            if (logManagerServiceHandle != nullptr)
            {
                logManagerService = logManagerServiceHandle->owner_.RawPtr();
            }
        }
        VerifyState(
            logManagerService,
            1,
            0,
            TRUE);

        KString::SPtr physicalLogName;
        GenerateUniqueFilename(physicalLogName);

        KGuid physicalLogId;
        physicalLogId.CreateNew();

        // Don't care if this fails
        SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));

        IPhysicalLogHandle::SPtr physicalLog;
        status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
        VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);
        VerifyState(logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            0,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE);


        KGuid logicalLogId;
        logicalLogId.CreateNew();
        KString::SPtr logicalLogFileName;
        GenerateUniqueFilename(logicalLogFileName);

        ILogicalLog::SPtr logicalLog;
        status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogFileName, logicalLog);
        VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);
        VerifyState(
            logManagerService,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()) != nullptr ? &(static_cast<PhysicalLogHandle&>(*physicalLog).Owner) : nullptr,
            1,
            1,
            TRUE,
            dynamic_cast<PhysicalLogHandle*>(physicalLog.RawPtr()),
            TRUE,
            nullptr,
            -1,
            -1,
            FALSE,
            nullptr,
            FALSE,
            FALSE,
            dynamic_cast<LogicalLog*>(logicalLog.RawPtr()),
            TRUE);
        VERIFY_IS_TRUE(logicalLog->IsFunctional);
        VERIFY_ARE_EQUAL(0, logicalLog->Length);
        VERIFY_ARE_EQUAL(0, logicalLog->ReadPosition);
        VERIFY_ARE_EQUAL(0, logicalLog->WritePosition);
        VERIFY_ARE_EQUAL(-1, logicalLog->HeadTruncationPosition);


        // Delete the logical log, then close it
        status = SyncAwait(physicalLog->DeleteLogicalLogOnlyAsync(logicalLogId, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("IPhysicalLogHandle::DeleteLogicalLogOnlyAsync", status);

        status = SyncAwait(logicalLog->CloseAsync());
        VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);


        // cleanup
        status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("PhysicalLog::CloseAsync", status);
        physicalLog = nullptr;

        status = SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::DeletePhysicalLogAsync", status);

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        VERIFY_STATUS_SUCCESS("LogManager::CloseAsync", status);
        logManager = nullptr;
    }
}
