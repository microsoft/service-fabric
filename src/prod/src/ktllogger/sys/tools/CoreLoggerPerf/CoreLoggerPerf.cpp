// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>

#include <ktllogger.h>
#include <klogicallog.h>

#include "..\tests\closesync.h"

#ifdef UDRIVER
#include "..\..\ktlshim\ktllogshimkernel.h"
#endif

#define VERIFY_IS_TRUE(cond, msg) if (! (cond)) { printf("Failure %s (0x%x) in %s at line %d\n",  msg, status, __FILE__, __LINE__); ExitProcess(static_cast<UINT>(-1)); }

#define ALLOCATION_TAG 'LLKT'
 
KAllocator* g_Allocator;

ULONG automationBlockSizes[] = { 64, 128, 256, 512, 1024 };
const ULONG automationBlockCount = (sizeof(automationBlockSizes) / sizeof(ULONG));

VOID SetupRawCoreLoggerTests(
    )
{
    NTSTATUS status;
    KtlSystem* System;

    status = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
                                   &System);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "KtlSystemInitialize");
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    KDbgMirrorToDebugger = TRUE;

    System->SetStrictAllocationChecks(TRUE);

#ifdef UDRIVER
    //
    // For UDRIVER, need to perform work done in PNP Device Add
    //
    status = FileObjectTable::CreateAndRegisterOverlayManager(*g_Allocator, ALLOCATION_TAG);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateAndRegisterOverlayManager");
#endif  
}

VOID CleanupRawCoreLoggerTests(
    )
{
#ifdef UDRIVER
    //
    // For UDRIVER need to perform work done in PNP RemoveDevice
    //
    NTSTATUS status;
    
    status = FileObjectTable::StopAndUnregisterOverlayManager(*g_Allocator);
    KInvariant(NT_SUCCESS(status));
#endif
    
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

        struct StartParameters
        {
            KtlLogStream::SPtr LogStream;
            KIoBuffer::SPtr RecordToWrite;
            KIoBuffer::SPtr MetadataIoBuffer;
            ULONGLONG* HighestCompletedAsn;
            ULONGLONG* Asn;
            ULONGLONG* IoCount;
        };

        VOID StartIt(
            __in PVOID Parameters,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
        {
            StartParameters* startParameters = (StartParameters*)Parameters;

            _State = Initial;
            _LogStream = startParameters->LogStream;
            _RecordToWrite = startParameters->RecordToWrite;
            _MetadataIoBuffer = startParameters->MetadataIoBuffer;
            _Asn = startParameters->Asn;
            _IoCount = startParameters->IoCount;

            Start(ParentAsyncContext, CallbackPtr);
        }

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
                    Status = _LogStream->CreateAsyncWriteContext(_WriteContext);
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }

                    _State = WriteRecord;
                    _InFlightAsn = 0;

                    // Fall through
                }

                case WriteRecord:
                {
                    _LastCompletedAsn = _InFlightAsn;

                    ULONGLONG version = 1;
                    ULONGLONG recordAsn = InterlockedIncrement(_Asn);
        
                    InterlockedIncrement(_IoCount);

                    _InFlightAsn = recordAsn;
                    _WriteContext->Reuse();
                    _WriteContext->StartWrite(recordAsn,
                                             version,
                                             _LogStream->QueryReservedMetadataSize(),
                                             _MetadataIoBuffer,
                                             _RecordToWrite,
                                             NULL,    // Reservation
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
        KtlLogStream::SPtr _LogStream;
        KIoBuffer::SPtr _RecordToWrite;
        KIoBuffer::SPtr _MetadataIoBuffer;
        PULONGLONG _Asn;
        PULONGLONG _IoCount;

        //
        // Internal
        //
        KtlLogStream::AsyncWriteContext::SPtr _WriteContext;
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

class TruncateAsync : public WorkerAsync
{
    K_FORCE_SHARED(TruncateAsync);

    public:
        static NTSTATUS
        Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out TruncateAsync::SPtr& Context
        )
        {
            NTSTATUS status;
            TruncateAsync::SPtr context;
            
            context = _new(AllocationTag, Allocator) TruncateAsync();
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
            context->_NotificationContext = nullptr;
            context->_TruncateContext = nullptr;

            Context = context.RawPtr();

            return(STATUS_SUCCESS);     
        }

        struct StartParameters
        {
            KtlLogStream::SPtr LogStream;
            ULONG AsyncCount;
            WriteFileAsync::SPtr* Asyncs;
        };
        
        VOID StartIt(
            __in PVOID Parameters,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
        {
            StartParameters* startParameters = (StartParameters*)Parameters;

            _State = Initial;
            _LogStream = startParameters->LogStream;
            _AsyncCount = startParameters->AsyncCount;
            _Asyncs = startParameters->Asyncs;

            Start(ParentAsyncContext, CallbackPtr);
        }

    protected:
        VOID FSMContinue(
            __in NTSTATUS Status,
            __in KAsyncContextBase& Async
        ) override
        {
            UNREFERENCED_PARAMETER(Async);

            if (! NT_SUCCESS(Status) && (Status != STATUS_CANCELLED))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }

            switch(_State)
            {
                case Initial:
                {
                    Status = _LogStream->CreateAsyncStreamNotificationContext(_NotificationContext);
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }

                    // Fall through
                }

                case WaitForNotification:
                {
                    if (_State != Initial)
                    {
                        KtlLogAsn lowestPersistedAsn = KtlLogAsn::Max();

                        for (ULONG i = 0; i < _AsyncCount; i++)
                        {
                            if (_Asyncs[i]->GetLastCompletedAsn() < lowestPersistedAsn.Get())
                            {
                                lowestPersistedAsn.Set(_Asyncs[i]->GetLastCompletedAsn());
                            }
                        }

                        //
                        // Truncate as much as possible
                        //
                        _TruncateContext = nullptr;
                        Status = _LogStream->CreateAsyncTruncateContext(_TruncateContext);
                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);
                            break;
                        }

                        _TruncateContext->Truncate(lowestPersistedAsn,
                                                   lowestPersistedAsn);
                    }

                    //
                    // Wait for the stream to get 50% full
                    //
                    _State = WaitForNotification;
                    _NotificationContext->Reuse();
                    _NotificationContext->StartWaitForNotification(KtlLogStream::AsyncStreamNotificationContext::LogSpaceUtilization,
                                                                  5,
                                                                  NULL,
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
            _State = Initial;
            _LogStream = nullptr;
            _NotificationContext = nullptr;
            _TruncateContext = nullptr;
        }

        VOID OnCompleted() override
        {
            _LogStream = nullptr;
            _Asyncs = NULL;
        }

        VOID OnCancel() override
        {
            _State = TimeToQuit;
            _NotificationContext->Cancel();
        }

    private:
        enum  FSMState { Initial=0, WaitForNotification=1, TimeToQuit = 2, Completed=3 };
        FSMState _State;

        //
        // Parameters
        //
        KtlLogStream::SPtr _LogStream;
        ULONG _AsyncCount;
        WriteFileAsync::SPtr* _Asyncs;

        //
        // Internal
        //
        KtlLogStream::AsyncStreamNotificationContext::SPtr _NotificationContext;
        KtlLogStream::AsyncTruncateContext::SPtr _TruncateContext;
};

TruncateAsync::TruncateAsync()
{
}

TruncateAsync::~TruncateAsync()
{
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

NTSTATUS CoreLoggerPerfTest(
    __in PWCHAR LogFilePath,
    __in PWCHAR LogStreamPath,
    __in ULONG RecordSize,
    __in ULONG LogGB,
    __in BOOLEAN WriteToDedicatedOnly,
    __in ULONG NumberThreads,
    __in ULONG TimeToRunInSeconds,
    __out float& TotalMBPerSecond,
    __out float& TotalIOPerSecond
    )
{
    NTSTATUS status;
    KtlLogManager::SPtr logManager;
    KServiceSynchronizer        serviceSync;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;
    KSynchronizer sync;

    //
    // Delete any old log
    //
    KWString s(*g_Allocator, LogFilePath);
    status = s.Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "KWString");
    status = KVolumeNamespace::DeleteFileOrDirectory(s, *g_Allocator, sync, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "DeleteFileOrDirectory");
    status = sync.WaitForCompletion();

    KWString s1(*g_Allocator, LogStreamPath);
    status = s1.Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "KWString");
    status = KVolumeNamespace::DeleteFileOrDirectory(s1, *g_Allocator, sync, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "DeleteFileOrDirectory");
    status = sync.WaitForCompletion();

#ifdef UPASSTHROUGH
    status = KtlLogManager::CreateInproc(ALLOCATION_TAG, *g_Allocator, logManager);
#else
    status = KtlLogManager::CreateDriver(ALLOCATION_TAG, *g_Allocator, logManager);
#endif
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Create KTLLogManager failed");
    
    status = logManager->StartOpenLogManager(NULL, // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status), "StartOpenLogManager");
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "LogManager open failed");

    //
    // create a log container
    //
    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);
    
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    KtlLogContainer::SPtr logContainer;
    LONGLONG OneGB = 1024 * 0x100000; 
    LONGLONG logSize = LogGB * OneGB ;   // 4GB

    status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateAsyncCreateLogContainerContext");

    //
    // Create log container
    //
    KString::SPtr logFilePath = KString::Create(LogFilePath, *g_Allocator, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateLogFilePath");
    createContainerAsync->StartCreateLogContainer(*logFilePath,
                                            logContainerId,
                                            logSize,
                                             0,            // Max Number Streams
                                             RecordSize*4,            // Max Record Size
                                            logContainer,
                                            NULL,         // ParentAsync
                                            sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateLogContainer");

    //
    // Create Stream within it
    //
    KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
    KtlLogStream::SPtr logStream;
            
    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateLogStreamContext");

    KString::SPtr logStreamPath = KString::Create(LogStreamPath, *g_Allocator, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateLogStreamPath");

    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    
    KBuffer::SPtr securityDescriptor = nullptr;
    createStreamAsync->StartCreateLogStream(logStreamId,
                                                nullptr,           // Alias
                                                KString::CSPtr(logStreamPath.RawPtr()),
                                                securityDescriptor,
                                                0x1000,
                                                logSize,
                                                RecordSize*4,
                                                logStream,
                                                NULL,    // ParentAsync
                                            sync);
            
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateLogStream");


    if (WriteToDedicatedOnly)
    {
        KtlLogStream::AsyncIoctlContext::SPtr ioctl;
        ULONG result;
        KBuffer::SPtr outBuffer;
        status = KBuffer::Create(sizeof(KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation),
                                 outBuffer,
                                 *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "Create outBuffer");

        status = logStream->CreateAsyncIoctlContext(ioctl);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateAsyncIoctlContext");

        ioctl->StartIoctl(KLogicalLogInformation::WriteOnlyToDedicatedLog,
                            NULL,
                            result,
                            outBuffer,
                            NULL,
                            sync);
        status =  sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status), "Ioctl Completion");
    }


    KIoBuffer::SPtr dataIoBuffer;
    PVOID dataBuffer;
    KIoBuffer::SPtr metadataIoBuffer;
    PVOID metadataBuffer;

    status = KIoBuffer::CreateSimple(0x1000,
                                     metadataIoBuffer,
                                     metadataBuffer,
                                     *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Create metadataIoBuffer");

    status = KIoBuffer::CreateSimple(RecordSize,
                                     dataIoBuffer,
                                     dataBuffer,
                                     *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Create dataIoBuffer");

    //
    // Start writer threads
    //
    const ULONG MAX_THREADS = 1024;
    WriteFileAsync::SPtr asyncs[MAX_THREADS];
    KSynchronizer syncs[MAX_THREADS];
    
    if (NumberThreads > MAX_THREADS)
    {
        printf("Maximum number of threads is %d\n", MAX_THREADS);
        NumberThreads = MAX_THREADS;
    }

    WriteFileAsync::StartParameters startParameters;
    ULONGLONG asn = 0;
    ULONGLONG ioCount;
    ULONGLONG finalIoCount;
    ULONGLONG startTicks;
    ULONGLONG endTicks;
    ULONGLONG totalTicks;

    startParameters.LogStream = logStream;
    startParameters.RecordToWrite = dataIoBuffer;
    startParameters.MetadataIoBuffer = metadataIoBuffer;
    startParameters.Asn = &asn;
    startParameters.IoCount = &ioCount;

    for (ULONG i = 0; i < NumberThreads; i++)
    {
        status = WriteFileAsync::Create(*g_Allocator, ALLOCATION_TAG, asyncs[i]);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "Create WriteFileAsync");

    }

    for (ULONG i = 0; i < NumberThreads; i++)
    {
        asyncs[i]->StartIt(&startParameters,
                           NULL,                     // Parent
                           syncs[i]);
    }

    //
    // Start truncation thread
    //
    TruncateAsync::SPtr truncateAsync;
    KSynchronizer truncateSync;
    TruncateAsync::StartParameters truncateStartParameters;

    status = TruncateAsync::Create(*g_Allocator, ALLOCATION_TAG, truncateAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Create TruncateAsync");

    truncateStartParameters.LogStream = logStream;
    truncateStartParameters.AsyncCount = NumberThreads;
    truncateStartParameters.Asyncs = asyncs;
    truncateAsync->StartIt(&truncateStartParameters,
                           NULL,     // Parent
                           truncateSync);


    //
    // Wait 15 seconds to warm up
    //
    KNt::Sleep(15 * 1000);

    //
    // Run for a period of time
    //
    startTicks = GetTickCount64();
    InterlockedExchange(&ioCount, 0);

    for (ULONG seconds = 0; seconds < TimeToRunInSeconds; seconds++)
    {
        KNt::Sleep(1000);
        
        for (ULONG i = 0; i < NumberThreads; i++)
        {
            status = syncs[i].WaitForCompletion(0);
            VERIFY_IS_TRUE((NT_SUCCESS(status) || status == STATUS_IO_TIMEOUT), "WriteFileAsync Completion");
        }
    }   
    
    finalIoCount = InterlockedExchange(&ioCount, 0);
    endTicks = GetTickCount64();

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
    }

    truncateAsync->Cancel();
    status = truncateSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "TruncateAsync Completion");

    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;

    logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

    status = closeStreamSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CLose Stream");

    logContainer->StartClose(NULL,
                        closeContainerSync.CloseCompletionCallback());
    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Close Container");     

    KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;

    status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Create DeleteContainerContext");
    deleteContainerAsync->StartDeleteLogContainer(*logFilePath,
                                            logContainerId,
                                            NULL,         // ParentAsync
                                            sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Delete log container");     

    //
    // Now close the log manager and verify that it closes promptly.
    // The bug that is being regressed causes the close to stop responding.
    //
    status = logManager->StartCloseLogManager(NULL, // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Start close log manager");

    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Log manager close");
    logManager= nullptr;


    totalTicks = endTicks - startTicks;
    ULONGLONG totalSeconds = totalTicks / 1000;
    ULONGLONG totalBytes = finalIoCount * RecordSize;
    ULONGLONG totalRawBytes = finalIoCount * (RecordSize + 0x1000);
    float totalMBPerSecond;
    float totalIOPerSecond;

    totalMBPerSecond = ((float)totalBytes / (float)(0x100000)) / (float)totalSeconds;
    totalIOPerSecond = (float)finalIoCount / (float)totalSeconds;
    printf("Total number IO: %I64d in %I64d seconds\n", finalIoCount, totalSeconds);
    printf("%I64d total bytes in log records or %I64d bytes/second or %f MB/second\n", totalBytes, (totalBytes / totalSeconds),  totalMBPerSecond);
    printf("%I64d total bytes written (including metadata) or %I64d bytes/second or %I64d MB/second\n", totalRawBytes, (totalRawBytes / totalSeconds),  (totalRawBytes / (0x100000)) / totalSeconds);
    printf("%f IO/second\n", totalIOPerSecond);


    // TODO: Compute !
    TotalMBPerSecond = totalMBPerSecond;
    TotalIOPerSecond = totalIOPerSecond;

    return(STATUS_SUCCESS);
}



int
wmain(int argc, wchar_t** args)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG recordSize = 256 * 4096;                  // Default 1MB
    BOOLEAN writeToDedicatedOnly = TRUE;            // Default to dedicated only
    PWCHAR logFilePath = NULL;
    PWCHAR logStreamPath = NULL;
    ULONG numberThreads = 16;
    ULONG timeToRunInSeconds = 60;
    ULONG logGB = 64;
    BOOLEAN automaticMode = FALSE;
    PWCHAR resultsXml = NULL;

    printf("Core Logger Performance test\n");
    printf("CoreLoggerPerf -r:<record size in 4K>  -l:<log size in GB> -f:<Path for log container file> -s:<Path for log stream file> -d:<true to write to dedicated log only> -n:<Number Threads> -t:<TimeToRunInSeconds>  -a:<Results XML>\n");
    printf("    Note that path for log files must be an absolute path in the form of \\\\??\\\\c:\\temp\n");
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

            case L's':
            {
                logStreamPath = value;
                break;
            }

            case L'd':
            {
                if (_wcsnicmp(value, L"false", (sizeof(L"false") / sizeof(WCHAR))) == 0)
                {
                    writeToDedicatedOnly = FALSE;
                } 
                else if (_wcsnicmp(value, L"true", (sizeof(L"true") / sizeof(WCHAR))) == 0)
                {
                    writeToDedicatedOnly = TRUE;
                } 
                else 
                {
                    printf("Invalid parameter value %ws\n", a);
                    return(-1);
                }

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

            case L'a':
            {
                resultsXml = value;
                automaticMode = TRUE;
                break;
            }

            default:
            {
                printf("Invalid Parameter %ws\n", a);
                return(-1);
            }
        }
    }

    if ((logFilePath == NULL) || (logStreamPath == NULL))
    {
        printf("Invalid Parameters - must specify fully qualified log file and log stream paths of the form \\??\\x:\\Directory\n");
        return(-1);
    }

    printf("LogFilePath: %ws\n", logFilePath);
    printf("LogStreamPath: %ws\n", logStreamPath);
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
    printf("WriteToDedicatedOnly: %s\n", writeToDedicatedOnly ? "true" : "false");
    printf("NumberThreads: %d\n", numberThreads);
    printf("TimeToRunInSeconds: %d\n", timeToRunInSeconds);
    printf("\n");

    SetupRawCoreLoggerTests();

    
    if (automaticMode)
    {
        float mbPerSecond[automationBlockCount];
        float ioPerSecond[automationBlockCount];

        for (ULONG i = 0; i < automationBlockCount; i++)
        {
            recordSize = automationBlockSizes[i] * 4096;
            printf("\n");
            printf("BlockSize: %d\n", recordSize);
            status = CoreLoggerPerfTest(logFilePath, logStreamPath, recordSize, logGB, writeToDedicatedOnly, numberThreads, timeToRunInSeconds, mbPerSecond[i], ioPerSecond[i]);
            if (! NT_SUCCESS(status))
            {
                break;
            }
        }

        PrintResults(resultsXml, L"KtlLoggerPerformance", automationBlockCount, automationBlockSizes, mbPerSecond);
    }
    else
    {
        float mbPerSecond, ioPerSecond;

        status = CoreLoggerPerfTest(logFilePath, logStreamPath, recordSize, logGB, writeToDedicatedOnly, numberThreads, timeToRunInSeconds, mbPerSecond, ioPerSecond);
    }

    
    CleanupRawCoreLoggerTests();
    return(status);
}



