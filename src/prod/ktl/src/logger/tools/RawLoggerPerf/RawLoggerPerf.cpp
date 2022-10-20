#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>

#include "..\..\inc\KtlPhysicalLog.h"

ULONG automationBlockSizes[] = { 1, 4, 16, 32, 64, 128, 256, 512, 1024 };
const ULONG automationBlockCount = (sizeof(automationBlockSizes) / sizeof(ULONG));


#define VERIFY_IS_TRUE(cond, msg) if (! (cond)) { printf("Failure %s (0x%x) in %s at line %d\n",  msg, status, __FILE__, __LINE__);  }

#define ALLOCATION_TAG 'LLKT'

KAllocator* g_Allocator;

VOID SetupRawCoreLoggerTests(
    )
{
    NTSTATUS status;
    KtlSystem* System;

    status = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
                                   &System);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "KtlSystemInitialize");
    if (!NT_SUCCESS(status))
    {
        ExitProcess((UINT)-1);
    }
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    KDbgMirrorToDebugger = TRUE;

    System->SetStrictAllocationChecks(TRUE);
}

VOID CleanupRawCoreLoggerTests(
    )
{
    EventUnregisterMicrosoft_Windows_KTL();

    KtlSystem::Shutdown();
}

class WorkerAsync : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(WorkerAsync);

    public:
        virtual VOID StartIt(
            __in PVOID Parameters,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

    protected:
        virtual VOID FSMContinue(
            __in NTSTATUS Status,
            __in KAsyncContextBase& Async
            )
        {
            UNREFERENCED_PARAMETER(Status);
            UNREFERENCED_PARAMETER(Async);

            Complete(STATUS_SUCCESS);
        }

        virtual VOID OnReuse() = 0;

        virtual VOID OnCompleted()
        {
        }

        virtual VOID OnCancel()
        {
        }

    private:
        VOID OnStart()
        {
            _Completion.Bind(this, &WorkerAsync::OperationCompletion);
            FSMContinue(STATUS_SUCCESS, *this);
        }

        VOID OperationCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
        )
        {
            UNREFERENCED_PARAMETER(ParentAsync);
            FSMContinue(Async.Status(), Async);
        }

    protected:
        KAsyncContextBase::CompletionCallback _Completion;

};

WorkerAsync::WorkerAsync()
{
}

WorkerAsync::~WorkerAsync()
{
}

class WriteFileAsync : public WorkerAsync
{
    K_FORCE_SHARED(WriteFileAsync);

    public:
        static NTSTATUS
        Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out WriteFileAsync::SPtr& Context
        )
        {
            NTSTATUS status;
            WriteFileAsync::SPtr context;

            context = _new(AllocationTag, Allocator) WriteFileAsync();
            if (context == nullptr)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                return(status);
            }

            status = context->Status();
            if (! NT_SUCCESS(status))
            {
                return(status);
            }

            context->_LogStream = nullptr;
            context->_RecordToWrite = nullptr;
            context->_LastCompletedAsn = 0;
            context->_InFlightAsn = 0;

            Context = context.RawPtr();

            return(STATUS_SUCCESS);
        }

        VOID StartIt(
            __in PVOID Parameters,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
        {
            StartParameters* startParameters = (StartParameters*)Parameters;

            _State = Initial;
            _LogStream = startParameters->LogStream;
            _RecordToWrite = startParameters->RecordToWrite;
            _MetadataBuffer = startParameters->MetadataBuffer;
            _Asn = startParameters->Asn;
            _IoCount = startParameters->IoCount;
            _TruncateCount = startParameters->TruncateCount;
            _TruncateInterval = startParameters->TruncateInterval;
            _AsyncCount = startParameters->AsyncCount;
            _Asyncs = startParameters->Asyncs;

            Start(ParentAsyncContext, CallbackPtr);
        }

        struct StartParameters
        {
            RvdLogStream::SPtr LogStream;
            KIoBuffer::SPtr RecordToWrite;
            KBuffer::SPtr MetadataBuffer;
            ULONGLONG* Asn;
            ULONGLONG* IoCount;
            ULONGLONG* TruncateCount;
            ULONGLONG TruncateInterval;
            ULONG AsyncCount;
            WriteFileAsync::SPtr* Asyncs;
        };


    protected:
        VOID FSMContinue(
            __in NTSTATUS Status,
            __in KAsyncContextBase& Async
        ) override
        {
            UNREFERENCED_PARAMETER(Async);

            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }

            switch(_State)
            {
                case Initial:
                {
                    KInvariant(_LastCompletedAsn == 0);
                    KInvariant(_InFlightAsn == 0);

                    Status = _LogStream->CreateAsyncWriteContext(_WriteContext);
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }

                    _State = WriteRecord;

                    // Fall through
                }

                case WriteRecord:
                {
                    ULONGLONG version = 1;
                    ULONGLONG recordAsn = InterlockedIncrement(_Asn);

                    _LastCompletedAsn = _InFlightAsn;

                    if ((recordAsn % _TruncateInterval) == 0)
                    {
                        RvdLogAsn highestPersistedAsn = 0;

                        for (ULONG i = 0; i < _AsyncCount; i++)
                        {
                            WriteFileAsync::SPtr a = _Asyncs[i];
                            KInvariant((LONGLONG)(a->GetLastCompletedAsn()) >= 0);
                            highestPersistedAsn.SetIfLarger(a->GetLastCompletedAsn());
                        }

                        InterlockedIncrement(_TruncateCount);
                        _LogStream->Truncate(highestPersistedAsn,
                                             highestPersistedAsn);
                    }

                    InterlockedIncrement(_IoCount);

                    _InFlightAsn = recordAsn;
                    _WriteContext->Reuse();
                    _WriteContext->StartWrite(recordAsn,
                                             version,
                                             _MetadataBuffer,
                                             _RecordToWrite,
                                             NULL,    // ParentAsync
                                             _Completion);
                    break;
                }

                case TimeToQuit:
                {
                    _State = Completed;

                    // Fall through
                }

                case Completed:
                {
                    Complete(STATUS_SUCCESS);
                    return;
                }

                default:
                {
                    Status = STATUS_UNSUCCESSFUL;
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;
                }
            }

            return;
        }

        VOID OnReuse() override
        {
        }

        VOID OnCompleted() override
        {
            _LogStream = nullptr;
            _RecordToWrite = nullptr;
            _MetadataBuffer = nullptr;
            _WriteContext = nullptr;
        }

        VOID OnCancel() override
        {
            _State = TimeToQuit;
        }

    private:
        enum  FSMState { Initial=0, WriteRecord=1, TimeToQuit = 2, Completed=3 };
        FSMState _State;

        //
        // Parameters
        //
        RvdLogStream::SPtr _LogStream;
        KIoBuffer::SPtr _RecordToWrite;
        KBuffer::SPtr _MetadataBuffer;
        PULONGLONG _Asn;
        PULONGLONG _IoCount;
        PULONGLONG _TruncateCount;
        ULONGLONG _TruncateInterval;
        ULONG _AsyncCount;
        WriteFileAsync::SPtr* _Asyncs;

        //
        // Internal
        //
        RvdLogStream::AsyncWriteContext::SPtr _WriteContext;
        ULONGLONG _LastCompletedAsn;
        ULONGLONG _InFlightAsn;

    public:
        ULONGLONG GetLastCompletedAsn() { return _LastCompletedAsn; };
};

WriteFileAsync::WriteFileAsync()
{
}

WriteFileAsync::~WriteFileAsync()
{
}

NTSTATUS
VerifyRawRecordCallback(
    __in_bcount(MetaDataBufferSize) UCHAR const *const MetaDataBuffer,
    __in ULONG MetaDataBufferSize,
    __in const KIoBuffer::SPtr& IoBuffer
)
{
    UNREFERENCED_PARAMETER(MetaDataBuffer);
    UNREFERENCED_PARAMETER(MetaDataBufferSize);
    UNREFERENCED_PARAMETER(IoBuffer);

    return(STATUS_SUCCESS);
}

NTSTATUS CoreLoggerPerfTest(
    __in PWCHAR LogFilePath,
    __in ULONG RecordSize,
    __in ULONG LogGB,
    __in ULONG NumberThreads,
    __in ULONGLONG TruncateInterval,
    __in ULONG TimeToRunInSeconds,
    __out float& TotalMBPerSecond,
    __out float& TotalIOPerSecond
    )
{
    NTSTATUS status;
    KSynchronizer activateSync;
    KSynchronizer sync;
    RvdLogManager::SPtr logManager;

    WriteFileAsync::StartParameters startParameters;
    ULONGLONG asn = 0;
    ULONGLONG ioCount = 0;
    ULONGLONG truncateCount = 0;
    ULONGLONG finalIoCount = 0;
    ULONGLONG finalTruncateCount = 0;
    ULONGLONG startTicks = 0;
    ULONGLONG endTicks = 0;
    ULONGLONG totalTicks = 0;

    const ULONG MAX_THREADS = 1024;
    WriteFileAsync::SPtr asyncs[MAX_THREADS];
    KSynchronizer syncs[MAX_THREADS];

    KIoBuffer::SPtr dataIoBuffer;
    PVOID dataBuffer;
    KBuffer::SPtr metadataBuffer;

    KGuid logContainerGuid;
    RvdLogId logContainerId;
    RvdLogManager::AsyncCreateLog::SPtr createLogOp;
    RvdLog::SPtr logContainer;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<RvdLogId>(logContainerGuid);

    KGuid logStreamGuid;
    RvdLogStreamId logStreamId;
    RvdLogStream::SPtr logStream;

    logStreamGuid.CreateNew();
    logStreamId = static_cast<RvdLogStreamId>(logStreamGuid);

    NTSTATUS failedStatus = STATUS_SUCCESS;

    KStringView logFilePath(LogFilePath);
    KWString testLogType(KtlSystem::GlobalNonPagedAllocator(), L"TestLogType");

    //
    // Delete any old log
    //
    KWString s(*g_Allocator, LogFilePath);
    status = s.Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "KWString");
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    status = KVolumeNamespace::DeleteFileOrDirectory(s, *g_Allocator, sync, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "DeleteFileOrDirectory");
    status = sync.WaitForCompletion();

    //
    // Get a log manager
    //
    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "RvdLogManager Create");
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = logManager->Activate(nullptr, activateSync);
    VERIFY_IS_TRUE(K_ASYNC_SUCCESS(status), "RvdLogManager Activate");
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    logManager->RegisterVerificationCallback(logContainerGuid,          // Actually LogStreamType
                                             &VerifyRawRecordCallback);

    //
    // Create a container and then 2 streams
    //

    RvdLog::AsyncCreateLogStreamContext::SPtr createStreamOp;

    status = logManager->CreateAsyncCreateLogContext(createLogOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateAsyncCreateLog");
    if (!NT_SUCCESS(status))
    {
        failedStatus = status;
        goto done;
    }

    LONGLONG OneGB = 1024 * 0x100000;
    LONGLONG logSize = LogGB * OneGB ;   // 4GB

    createLogOp->StartCreateLog(
        logFilePath,
        logContainerId,
        testLogType,
        logSize,
        0,          // Use default max streams
        RecordSize*4,
        logContainer,
        nullptr,
        sync);

    status = sync.WaitForCompletion();
    createLogOp = nullptr;
    VERIFY_IS_TRUE(NT_SUCCESS(status), "StartCreateLog");
    if (!NT_SUCCESS(status))
    {
        failedStatus = status;
        goto done;
    }


    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateAsyncCreateLogStream");
    if (!NT_SUCCESS(status))
    {
        failedStatus = status;
        goto done;
    }

    createStreamOp->StartCreateLogStream(logStreamId,
                                         logContainerGuid,          // Actually LogStreamType
                                            logStream,
                                            nullptr,
                                            sync);
    status = sync.WaitForCompletion();
    createStreamOp = nullptr;
    VERIFY_IS_TRUE(NT_SUCCESS(status), "StartCreateLogStream");
    if (!NT_SUCCESS(status))
    {
        failedStatus = status;
        goto done;
    }

    status = KBuffer::Create(0x100,
                             metadataBuffer,
                            *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Create metadataIoBuffer");
    if (!NT_SUCCESS(status))
    {
        failedStatus = status;
        goto done;
    }

    status = KIoBuffer::CreateSimple(RecordSize,
                                     dataIoBuffer,
                                     dataBuffer,
                                     *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Create dataIoBuffer");
    if (!NT_SUCCESS(status))
    {
        failedStatus = status;
        goto done;
    }

    //
    // Start writer threads
    //
    if (NumberThreads > MAX_THREADS)
    {
        printf("Maximum number of threads is %d\n", MAX_THREADS);
        NumberThreads = MAX_THREADS;
    }


    startParameters.LogStream = logStream;
    startParameters.RecordToWrite = dataIoBuffer;
    startParameters.MetadataBuffer = metadataBuffer;
    startParameters.Asn = &asn;
    startParameters.IoCount = &ioCount;
    startParameters.TruncateCount = &truncateCount;
    startParameters.TruncateInterval = TruncateInterval;
    startParameters.Asyncs = asyncs;
    startParameters.AsyncCount = NumberThreads;

    for (ULONG i = 0; i < NumberThreads; i++)
    {
        status = WriteFileAsync::Create(*g_Allocator, ALLOCATION_TAG, asyncs[i]);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "Create WriteFileAsync");
        if (!NT_SUCCESS(status))
        {
            failedStatus = status;
            goto done;
        }
    }

    for (ULONG i = 0; i < NumberThreads; i++)
    {
        asyncs[i]->StartIt(&startParameters,
                           NULL,                     // Parent
                           syncs[i]);
    }

    //
    // Wait 15 seconds to warm up
    //
    KNt::Sleep(15 * 1000);

    //
    // Run for a period of time
    //
    InterlockedExchange(&truncateCount, 0);
    startTicks = GetTickCount64();
    InterlockedExchange(&ioCount, 0);

    for (ULONG seconds = 0; seconds < TimeToRunInSeconds; seconds++)
    {
        KNt::Sleep(1000);

        for (ULONG i = 0; i < NumberThreads; i++)
        {
            status = syncs[i].WaitForCompletion(0);
            VERIFY_IS_TRUE((NT_SUCCESS(status) || status == STATUS_IO_TIMEOUT), "WriteFileAsync Completion");
            if ((!NT_SUCCESS(status)) && (status != STATUS_IO_TIMEOUT))
            {
                failedStatus = status;
                goto cancel;
            }
        }
    }

    finalIoCount = InterlockedExchange(&ioCount, 0);
    endTicks = GetTickCount64();
    finalTruncateCount = InterlockedExchange(&truncateCount, 0);

cancel:
    //
    // Cancel and wait for asyncs to complete
    //
    for (ULONG i = 0; i < NumberThreads; i++)
    {
        asyncs[i]->Cancel();
    }

    for (ULONG i = 0; i < NumberThreads; i++)
    {
        status = syncs[i].WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status), "WriteFileAsync Completion");
        if (!NT_SUCCESS(status))
        {
            failedStatus = status;
        }
        asyncs[i] = nullptr;
    }

done:
    startParameters.LogStream = nullptr;
    startParameters.RecordToWrite = nullptr;
    startParameters.MetadataBuffer = nullptr;

    logStream = nullptr;
    logContainer = nullptr;
    KNt::Sleep(250);

    RvdLogManager::AsyncDeleteLog::SPtr deleteLogOp;

    status = logManager->CreateAsyncDeleteLogContext(deleteLogOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateAsyncDeleteStream");

    for (ULONG i = 0; i < 5; i++)
    {
        KNt::Sleep(1000);
        deleteLogOp->Reuse();
        deleteLogOp->StartDeleteLog(KStringView(LogFilePath), nullptr, sync);
        status = sync.WaitForCompletion();
        if (NT_SUCCESS(status))
        {
            break;
        }
    }

    if (! NT_SUCCESS(status))
    {
        printf("Error %x deleting %ws, delete manually\n", status, LogFilePath);
    }

    logManager->Deactivate();
    activateSync.WaitForCompletion();
    logManager.Reset();


    if (NT_SUCCESS(failedStatus))
    {
        totalTicks = endTicks - startTicks;
        ULONGLONG totalSeconds = totalTicks / 1000;
        ULONGLONG totalBytes = finalIoCount * RecordSize;
        ULONGLONG totalRawBytes = finalIoCount * (RecordSize + 0x1000);
        float totalMBPerSecond = ((float)totalBytes / (float)(0x100000)) / (float)totalSeconds;
        float totalIOPerSecond = (float)finalIoCount / (float)totalSeconds;

        printf("Total number IO: %I64d in %I64d seconds\n", finalIoCount, totalSeconds);
        printf("%I64d truncations for %I64d truncation interval\n", finalTruncateCount, TruncateInterval);
        printf("%I64d total bytes in log records or %I64d bytes/second or %f MB/second\n", totalBytes, (totalBytes / totalSeconds), totalMBPerSecond);
        printf("%I64d total bytes written (including metadata) or %I64d bytes/second or %I64d MB/second\n", totalRawBytes, (totalRawBytes / totalSeconds), (totalRawBytes / (0x100000)) / totalSeconds);
        printf("%f IO/second\n", totalIOPerSecond);

        TotalMBPerSecond = totalMBPerSecond;
        TotalIOPerSecond = totalIOPerSecond;
    }

    return(failedStatus);
}

/*
<?xml version="1.0" encoding="utf-8"?>
<PerformanceResults>
<PerformanceResult>
<Context>
<Environment Name="MachineName" Value="DANVER2" />
<Parameter Name="BlockSize" Value="1" />
<Parameter Name="ForegroundQueueSIze" Value="32" />
<Parameter Name="BackgroundQueueSize" Value="0" />
</Context>
<Measurements>
<Measurement Name="RawDiskPerformance">
<Value Metric="MBPerSecond" Value="3421.16921758783" When="2/13/2015 2:44:04 AM" />
</Measurement>
</Measurements>
</PerformanceResult>
</PerformanceResults>
*/

NTSTATUS AddStringAttribute(
    __in KIMutableDomNode::SPtr dom,
    __in LPCWCHAR Name,
    __in LPCWCHAR Value
    )
{
    NTSTATUS status;

    KVariant value;
    value = KVariant::Create(Value, KtlSystem::GlobalNonPagedAllocator());
    status = value.Status();

    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    status = dom->SetAttribute(KIDomNode::QName(Name), value);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS AddULONGAttribute(
    __in KIMutableDomNode::SPtr dom,
    __in LPCWCHAR Name,
    __in ULONG Value
    )
{
    NTSTATUS status;
    KVariant value;
    WCHAR str[MAX_PATH];
    HRESULT hr;

    hr = StringCchPrintf(str, MAX_PATH, L"%d", Value);

    value = KVariant::Create(str, KtlSystem::GlobalNonPagedAllocator());
    status = value.Status();

    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = dom->SetAttribute(KIDomNode::QName(Name), value);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS AddFloatAttribute(
    __in KIMutableDomNode::SPtr dom,
    __in LPCWCHAR Name,
    __in float Value
    )
{
    NTSTATUS status;
    KVariant value;
    WCHAR str[MAX_PATH];
    HRESULT hr;

    hr = StringCchPrintf(str, MAX_PATH, L"%f", Value);

    value = KVariant::Create(str, KtlSystem::GlobalNonPagedAllocator());
    status = value.Status();

    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = dom->SetAttribute(KIDomNode::QName(Name), value);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
PrintResults(
__in PWCHAR ResultsXml,
__in PWCHAR TestName,
__in ULONG ResultCount,
__in ULONG* BlockSizes,
__in_ecount(ResultCount) float* MBPerSecond
)
{
    NTSTATUS status;
    ULONG error;
    HRESULT hr;
    KIMutableDomNode::SPtr domRoot;

    // TODO: include all parameters for the run

    status = KDom::CreateEmpty(domRoot, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = domRoot->SetName(KIMutableDomNode::QName(L"PerformanceResults"));
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    for (ULONG i = 0; i < ResultCount; i++)
    {
        KIMutableDomNode::SPtr performanceResult;
        status = domRoot->AddChild(KIDomNode::QName(L"PerformanceResult"), performanceResult);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr context;
        status = performanceResult->AddChild(KIDomNode::QName(L"Context"), context);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr environment;
        status = context->AddChild(KIDomNode::QName(L"Environment"), environment);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddStringAttribute(environment, L"Name", L"MachineName");
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        BOOL b;
        WCHAR computerName[MAX_PATH];
        ULONG computerNameLen = MAX_PATH;
        b = GetComputerName(computerName, &computerNameLen);
        if (!b)
        {
            error = GetLastError();
            return(error);
        }

        status = AddStringAttribute(environment, L"Value", computerName);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr parameter1;;
        status = context->AddChild(KIDomNode::QName(L"Parameter"), parameter1);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddStringAttribute(parameter1, L"Name", L"BlockSizeInKB");
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddULONGAttribute(parameter1, L"Value", BlockSizes[i] * 4);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr measurements;
        status = performanceResult->AddChild(KIDomNode::QName(L"Measurements"), measurements);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr measurement;
        status = measurements->AddChild(KIDomNode::QName(L"Measurement"), measurement);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddStringAttribute(measurement, L"Name", TestName);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr value;
        status = measurement->AddChild(KIDomNode::QName(L"Value"), value);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddStringAttribute(value, L"Metric", L"MBPerSecond");
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddFloatAttribute(value, L"Value", MBPerSecond[i]);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        SYSTEMTIME systemTime;
        WCHAR time[MAX_PATH];
        GetSystemTime(&systemTime);
        hr = StringCchPrintf(time, MAX_PATH, L"%02d/%02d%/%d %02d:%02d:%02d",
            systemTime.wMonth, systemTime.wDay, systemTime.wYear,
            systemTime.wHour, systemTime.wMinute, systemTime.wSecond);

        status = AddStringAttribute(value, L"When", time);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }
    }

    KString::SPtr domKString;
    status = KDom::ToString(domRoot, KtlSystem::GlobalNonPagedAllocator(), domKString);

    HANDLE file;

    file = CreateFile(ResultsXml, GENERIC_ALL, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == NULL)
    {
        error = GetLastError();
        return(error);
    }

    LPCWSTR domString = (LPCWSTR)*domKString;
    ULONG bytesWritten;

    error = WriteFile(file, domString, (ULONG)(wcslen(domString) * sizeof(WCHAR)), &bytesWritten, NULL);
    CloseHandle(file);

    if (error != ERROR_SUCCESS)
    {
        return(error);
    }

    return(STATUS_SUCCESS);
}


int
wmain(int argc, wchar_t** args)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG recordSize = 256 * 4096;                  // Default 1MB
    PWCHAR logFilePath = NULL;
    PWCHAR resultsXml = NULL;
    ULONG numberThreads = 16;
    ULONG timeToRunInSeconds = 60;
    ULONG logGB = 64;
    ULONGLONG truncateInterval = 256;
    BOOLEAN automaticMode = FALSE;

    printf("Raw Logger Performance test\n");
    printf("RawLoggerPerf -r:<record size in 4K> -l:<log size in GB> -f:<Path for log file> -n:<Number Threads> -t:<TimeToRunInSeconds> -c:<truncate interval> -a:<Results XML>\n");
    printf("    Note that path for log files must be an absolute path in the form of \\??\\c:\\temp\n");
    printf("\n");
    //
    // Parse Parameters
    //
    for (int i = 1; i < argc; i++)
    {
        PWCHAR a = args[i];
        WCHAR option;
        PWCHAR value;
        size_t len;

        len  = wcslen(a);
        if ((len < 3) || (a[0] != L'-') || (a[2] != L':'))
        {
            printf("Invalid Parameter %ws\n", a);
            return(-1);
        }

        option = towlower(a[1]);
        value = &a[3];

        switch(option)
        {
            case L'r':
            {
                ULONG x;
                x = _wtoi(value);
                if (x == 0)
                {
                    printf("Invalid parameter value %ws\n", a);
                    return(-1);
                }

                recordSize = x * 4096;

                break;
            }

            case L'l':
            {
                ULONG x;
                x = _wtoi(value);
                if (x == 0)
                {
                    printf("Invalid parameter value %ws\n", a);
                    return(-1);
                }

                logGB = x;

                break;
            }

            case L'f':
            {
                logFilePath = value;
                break;
            }

            case L'a':
            {
                resultsXml = value;
                automaticMode = TRUE;
                break;
            }

            case L'n':
            {
                ULONG x;
                x = _wtoi(value);
                if (x == 0)
                {
                    printf("Invalid parameter value %ws\n", a);
                    return(-1);
                }

                numberThreads = x;
                break;
            }

            case L't':
            {
                ULONG x;
                x = _wtoi(value);
                if (x == 0)
                {
                    printf("Invalid parameter value %ws\n", a);
                    return(-1);
                }

                timeToRunInSeconds = x;
                break;
            }

            case L'c':
            {
                ULONG x;
                x = _wtoi(value);
                if (x == 0)
                {
                    printf("Invalid parameter value %ws\n", a);
                    return(-1);
                }

                truncateInterval = x;
                break;
            }

            default:
            {
                printf("Invalid Parameter %ws\n", a);
                return(-1);
            }
        }
    }

    if (logFilePath == NULL)
    {
        printf("Invalid Parameters - must specify fully qualified log file of the form \\??\\x:\\Directory\n");
        return(-1);
    }

    if (truncateInterval < (4*numberThreads))
    {
        truncateInterval = 4 * numberThreads;
        printf("Truncate interval must be at least 4 times number threads\n\n");
    }

    printf("LogFilePath: %ws\n", logFilePath);
    if (automaticMode)
    {
        printf("    BlockSizeIn4K: ");
        for (ULONG i = 0; i < automationBlockCount; i++)
        {
            printf("%d, ", automationBlockSizes[i]);
        }
        printf("\n");
        printf("    Results XML: %ws\n", resultsXml);
    }
    else
    {
        printf("RecordSize: %d\n", recordSize);
    }
    printf("LogSize: %d GB\n", logGB);
    printf("NumberThreads: %d\n", numberThreads);
    printf("TruncateInterval: %I64d\n", truncateInterval);
    printf("TimeToRunInSeconds: %d\n", timeToRunInSeconds);
    printf("\n");

    SetupRawCoreLoggerTests();

    if (automaticMode)
    {
        float mbPerSecond[automationBlockCount];
        float ioPerSecond[automationBlockCount];

        for (ULONG i = 0; i < automationBlockCount; i++)
        {
            status = CoreLoggerPerfTest(logFilePath, recordSize, logGB, numberThreads, truncateInterval, timeToRunInSeconds, mbPerSecond[i], ioPerSecond[i]);
            if (! NT_SUCCESS(status))
            {
                break;
            }
        }

        PrintResults(resultsXml, L"RawLoggerPerformance", automationBlockCount, automationBlockSizes, mbPerSecond);
    }
    else
    {
        float mbPerSecond, ioPerSecond;

        status = CoreLoggerPerfTest(logFilePath, recordSize, logGB, numberThreads, truncateInterval, timeToRunInSeconds, mbPerSecond, ioPerSecond);
    }

    CleanupRawCoreLoggerTests();
    return(status);
}



