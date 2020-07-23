// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "WexTestClass.h"
#include "LogController.h"

#include <ktl.h>
#include <ktrace.h>

#include <ktllogger.h>
#include <klogicallog.h>

#include "KtlLoggerTests.h"
#include "TestUtil.h"
#include "closesync.h"

#include <shellapi.h>

#include "ControllerHost.h"
#include "llworkload.h"

#ifdef UDRIVER
#include "..\..\ktlshim\ktllogshimkernel.h"
#endif

using namespace WEX::Logging;
using namespace WEX::Common;
using namespace WEX::TestExecution;

using namespace TestIPC;

#define ALLOCATION_TAG 'LLT'

#include "VerifyQuiet.h"

extern KAllocator* g_Allocator;

extern 
DWORD LLWorkloadThread(
    LPVOID Context
    );

DWORD OpenCloseKtlLogManagerThread(
    LPVOID Context
    );

namespace KtlPhysicalLogTest
{
    //
    // Number of host processes running the workload tests
    //
    const PWCHAR NumberConcurrentHostsName = L"NumberConcurrentHosts";

    //
    // Maximum number of seconds to wait between terminating a host
    // while running the LLProcessCrashTest
    //
    const PWCHAR MaxCrashDelayInSecondsName = L"MaxCrashDelayInSeconds";
    
    const PWCHAR LogContainerIdName = L"LogContainerId";

    //
    // Number of logical log workloads running in each host
    //
    const PWCHAR NumberLogicalLogsName = L"NumberLogicalLogs";

    //
    // Minimum length of time that the logical log workload should run
    // before completing the test
    //
    const PWCHAR TestDurationInSecondsName = L"TestDurationInSeconds";

    //
    // Maximum number of records to write in each cycle within the test
    //
    const PWCHAR MaxRecordsWrittenPerCycleName = L"MaxRecordsWrittenPerCycle";

    //
    // Number of ms to delay between writes
    //
    const PWCHAR DelayBetweenWritesInMsName = L"DelayBetweenWritesInMs";

    //
    // Maximum specific write latency to tolerate before failing the
    // test
    //
    const PWCHAR MaxWriteLatencyInMsName = L"MaxWriteLatencyInMs";
    
    //
    // Maximum average write latency to tolerate before failing the
    // test
    //
    const PWCHAR MaxAverageWriteLatencyInMsName = L"MaxAverageWriteLatencyInMs";

    //
    // This is the class that implements the controller
    //
    class KtlLLWorkloadController : public WEX::TestClass<KtlLLWorkloadController>
    {
    public:
        TEST_CLASS(KtlLLWorkloadController)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

        TEST_METHOD(LLManyLogsTest);
        TEST_METHOD(LLProcessCrashTest);
        TEST_METHOD(ProcessCrashOnStartTest);

    private:
        VOID BuildHostCommandLine(
            PWCHAR TestName,								  
            ULONG Offset
            );
        
    public:
        KGuid _DiskId;
        UCHAR _DriveLetter;
        KtlLogManager::SPtr _LogManager;
        ULONGLONG _StartingAllocs;
        KtlSystem* _System;

        //
        // config parameters from command line
        //
        KtlLogContainerId _LogContainerId;
        ULONG _NumberConcurrentHosts;
        ULONG _MaxCrashDelayInSeconds;
        ULONG _NumberLogicalLogs;
        ULONG _TestDurationInSeconds;
        ULONG _MaxRecordsWrittenPerCycle;
        ULONG _MaxWriteLatencyInMs;
        ULONG _MaxAverageWriteLatencyInMs;
        ULONG _DelayBetweenWritesInMs;

        WCHAR _CommandLine[1024];
        WCHAR _Temp[1024];

        WCHAR _DllName[MAX_PATH];
        WCHAR _TEName[MAX_PATH];
        RPC_WSTR _LogContainerIdString;
        
        TestIPCController _IPCController;
    };

    //
    // Command line to use to launch host:
    //
    //    te.exe LLWorkloadDLL
    //               /inproc
    //               /p:IPCController=[connection data] 
    //               /p:IPCControllerOffset=[offset to start]
    //               /p:NumberLogicalLogs=<>
    //               /p:TestDurationInSeconds=<>
    //               /p:DelayBetweenWritesInMs=<>
    //               /p:MaxRecordsWrittenPerCycle=<>
    //               /p:MaxWriteLatencyInMs=<>
    //               /p:MaxAverageWriteLatencyInMs=<>
    //

    VOID KtlLLWorkloadController::BuildHostCommandLine(
		PWCHAR TestName,
        ULONG Offset
        )
    {
        HRESULT hr;
        
        hr = StringCchPrintf(_CommandLine, sizeof(_CommandLine) / sizeof(WCHAR),
                             L"\"%ws\""
                             L"/inproc "
                             L"%ws "
                             L"/name:%ws "  // KtlPhysicalLogTest::KtlLLWorkloadTests::LLWorkloadsTest
                             L"/p:%ws=%ws "
                             L"/p:%ws=%d "
                             L"/p:%ws=%ws "
                             L"/p:%ws=%d "
                             L"/p:%ws=%d "
                             L"/p:%ws=%d "
                             L"/p:%ws=%d "
                             L"/p:%ws=%d "
                             L"/p:%ws=%d ",
                             _TEName, _DllName,
							 TestName,
                             TestIPCController::IPCControllerName,
                             _IPCController.GetConnectionString(),
                             TestIPCController::IPCControllerOffsetName,
                             Offset,
                             LogContainerIdName,
                             _LogContainerIdString,
                             NumberLogicalLogsName,
                             _NumberLogicalLogs,
                             TestDurationInSecondsName,
                             _TestDurationInSeconds,
                             DelayBetweenWritesInMsName,
                             _DelayBetweenWritesInMs,
                             MaxRecordsWrittenPerCycleName,
                             _MaxRecordsWrittenPerCycle,
                             MaxWriteLatencyInMsName,
                             _MaxWriteLatencyInMs,
                             MaxAverageWriteLatencyInMsName,
                             _MaxAverageWriteLatencyInMs);
        VERIFY_IS_TRUE(SUCCEEDED(hr));
    }
    
    void KtlLLWorkloadController::LLManyLogsTest()
    {
        ULONG error;
        BOOL b;
        const ULONG maxProcesses = 256;
        HANDLE processHandles[maxProcesses];
        STARTUPINFO startupInfo;
        HANDLE event;
        ULONGLONG startTime, endTime;

        event = CreateEvent(NULL, TRUE, FALSE, NULL);
        VERIFY_IS_TRUE(event != NULL);

        memset(&startupInfo, 0, sizeof(STARTUPINFO));
        startupInfo.cb = sizeof(STARTUPINFO);
        
        VERIFY_IS_TRUE(_NumberConcurrentHosts < maxProcesses);

        startTime = GetTickCount64();
        for (ULONG i = 0; i < _NumberConcurrentHosts; i++)
        {
            PROCESS_INFORMATION processInfo;

            BuildHostCommandLine(L"KtlPhysicalLogTest::KtlLLWorkloadTests::LLWorkloadsTest",
								 i * (_NumberLogicalLogs * sizeof(LLWORKLOADSHAREDINFO)));
            
            Log::Comment(_CommandLine);
            b = CreateProcess(NULL,                  // lpApplicationName
                              _CommandLine,
                              NULL,                  // lpProcessAttributes
                              NULL,                  // lpThreadAttributes
                              FALSE,                 // bInheritHandles
                              0,                     // dwCreationFlags
                              NULL,                  // lpEnvironment
                              NULL,                  // lpCurrentDirectory
                              &startupInfo,                  // lpStartupInfo
                              &processInfo);

            error = GetLastError();
            VERIFY_IS_TRUE(b ? true : false);

            processHandles[i] = processInfo.hProcess;
            CloseHandle(processInfo.hThread);
        }

        ULONGLONG averageOfAverage = 0;
        ULONGLONG totalBytesWritten = 0;
        ULONG completedHosts = 0;
        ULONG completed;
        ULONG connectionOffset;
        LLWORKLOADSHAREDINFO* llWorkloadInfo;

        while (completedHosts < _NumberConcurrentHosts)
        {
            completed = WaitForMultipleObjects(_NumberConcurrentHosts, processHandles, FALSE, 60*1000);
            VERIFY_IS_TRUE(completed != WAIT_FAILED);

            if (completed != WAIT_TIMEOUT)
            {
                completed = completed - WAIT_OBJECT_0;
                connectionOffset = completed * sizeof(LLWORKLOADSHAREDINFO);
                llWorkloadInfo = (LLWORKLOADSHAREDINFO*)
                                                       ((PUCHAR)(_IPCController.GetPointer()) + connectionOffset);

                if (llWorkloadInfo->Results.CompletionStatus != ERROR_SUCCESS)
                {
                    //
                    // Sprintf to a buffer since TAEF string.Format()
                    // doesn't support ANSI formatting %s
                    //
                    StringCbPrintf(_Temp, sizeof(_Temp),
                                   L"FAILURE %x: %ws at %hs line %d\n",
                               llWorkloadInfo->Results.CompletionStatus,
                               llWorkloadInfo->Results.ErrorMessage,
                               llWorkloadInfo->Results.ErrorFileName,
                               llWorkloadInfo->Results.ErrorLineNumber);
                    VERIFY_IS_TRUE(false, _Temp);                  
                }
                
                LOG_OUTPUT(L"Process %d completed with status %d\n", completed, llWorkloadInfo->Results.CompletionStatus);
                CloseHandle(processHandles[completed]);
                processHandles[completed] = event;
                
                completedHosts++;
            }

            totalBytesWritten = 0;
            averageOfAverage = 0;
            
            for (ULONG i = 0; i < _NumberConcurrentHosts; i++)
            {
                connectionOffset = (i * (_NumberLogicalLogs * sizeof(LLWORKLOADSHAREDINFO)));
                llWorkloadInfo = (LLWORKLOADSHAREDINFO*)
                                                       ((PUCHAR)(_IPCController.GetPointer()) + connectionOffset);

                for (ULONG j = 0; j < _NumberLogicalLogs; j++)
                {               
                    LOG_OUTPUT(L"AverageLatency[%d][%d] = %d", i, j, llWorkloadInfo[j].AverageWriteLatencyInMs);
                    LOG_OUTPUT(L"HighestLatency[%d][%d] = %d", i, j, llWorkloadInfo[j].HighestWriteLatencyInMs);
                    averageOfAverage += llWorkloadInfo[j].AverageWriteLatencyInMs;
                    totalBytesWritten += llWorkloadInfo[j].TotalBytesWritten;
                }
                LOG_OUTPUT(L"\n");
            }
        }
        
        endTime = GetTickCount64();
        totalBytesWritten = totalBytesWritten / ((endTime-startTime) / 1000);
        totalBytesWritten = totalBytesWritten / 0x400;
        LOG_OUTPUT(L"Throughput %ld K/sec", totalBytesWritten);

        averageOfAverage = averageOfAverage / (_NumberConcurrentHosts*_NumberLogicalLogs);
        LOG_OUTPUT(L"Average of Averages: %ld ms", averageOfAverage);
        
        CloseHandle(event);
    }   


    void KtlLLWorkloadController::LLProcessCrashTest()
    {
        ULONG error;
        BOOL b;
        const ULONG maxProcesses = 256;
        HANDLE processHandles[maxProcesses];
        STARTUPINFO startupInfo;
        HANDLE event;

        srand((ULONG)GetTickCount());
        
        event = CreateEvent(NULL, TRUE, FALSE, NULL);
        VERIFY_IS_TRUE(event != NULL);

        memset(&startupInfo, 0, sizeof(STARTUPINFO));
        startupInfo.cb = sizeof(STARTUPINFO);
        
        VERIFY_IS_TRUE(_NumberConcurrentHosts < maxProcesses);

        for (ULONG i = 0; i < _NumberConcurrentHosts; i++)
        {
            PROCESS_INFORMATION processInfo;

            BuildHostCommandLine(L"KtlPhysicalLogTest::KtlLLWorkloadTests::LLWorkloadsTest",
								 i * (_NumberLogicalLogs * sizeof(LLWORKLOADSHAREDINFO)));
            
            Log::Comment(_CommandLine);
            b = CreateProcess(NULL,                  // lpApplicationName
                              _CommandLine,
                              NULL,                  // lpProcessAttributes
                              NULL,                  // lpThreadAttributes
                              FALSE,                 // bInheritHandles
                              0,                     // dwCreationFlags
                              NULL,                  // lpEnvironment
                              NULL,                  // lpCurrentDirectory
                              &startupInfo,                  // lpStartupInfo
                              &processInfo);

            error = GetLastError();
            VERIFY_IS_TRUE(b ? true : false);

            processHandles[i] = processInfo.hProcess;
            CloseHandle(processInfo.hThread);
        }

        ULONG completedHosts = 0;
        ULONG completed;
        ULONG connectionOffset;
        LLWORKLOADSHAREDINFO* llWorkloadInfo;
        ULONG crashDelay;

        while (completedHosts < _NumberConcurrentHosts)
        {
            crashDelay = rand() % _MaxCrashDelayInSeconds;
            completed = WaitForMultipleObjects(_NumberConcurrentHosts, processHandles, FALSE, crashDelay*1000);
            VERIFY_IS_TRUE(completed != WAIT_FAILED);

            if (completed != WAIT_TIMEOUT)
            {
                completed = completed - WAIT_OBJECT_0;
                connectionOffset = completed * sizeof(LLWORKLOADSHAREDINFO);
                llWorkloadInfo = (LLWORKLOADSHAREDINFO*)
                                                       ((PUCHAR)(_IPCController.GetPointer()) + connectionOffset);

                if (llWorkloadInfo->Results.CompletionStatus != ERROR_SUCCESS)
                {
                    //
                    // Sprintf to a buffer since TAEF string.Format()
                    // doesn't support ANSI formatting %s
                    //
                    StringCbPrintf(_Temp, sizeof(_Temp),
                                   L"FAILURE %x: %ws at %hs line %d\n",
                               llWorkloadInfo->Results.CompletionStatus,
                               llWorkloadInfo->Results.ErrorMessage,
                               llWorkloadInfo->Results.ErrorFileName,
                               llWorkloadInfo->Results.ErrorLineNumber);
                    VERIFY_IS_TRUE(false, _Temp);                  
                }
                
                LOG_OUTPUT(L"Process %d completed with status %d\n", completed, llWorkloadInfo->Results.CompletionStatus);
                CloseHandle(processHandles[completed]);
                processHandles[completed] = event;
                
                completedHosts++;
            } else {
                completed = rand() % _NumberConcurrentHosts;
                LOG_OUTPUT(L"Terminate process %d", completed);
                TerminateProcess(processHandles[completed], ERROR_SUCCESS);
            }

            for (ULONG i = 0; i < _NumberConcurrentHosts; i++)
            {
                connectionOffset = (i * (_NumberLogicalLogs * sizeof(LLWORKLOADSHAREDINFO)));
                llWorkloadInfo = (LLWORKLOADSHAREDINFO*)
                                                       ((PUCHAR)(_IPCController.GetPointer()) + connectionOffset);

                for (ULONG j = 0; j < _NumberLogicalLogs; j++)
                {               
                    LOG_OUTPUT(L"AverageLatency[%d][%d] = %d", i, j, llWorkloadInfo[j].AverageWriteLatencyInMs);
                    LOG_OUTPUT(L"HighestLatency[%d][%d] = %d", i, j, llWorkloadInfo[j].HighestWriteLatencyInMs);
                }
                LOG_OUTPUT(L"\n");

            }       
        }
        
        CloseHandle(event);
    }   

    void KtlLLWorkloadController::ProcessCrashOnStartTest()
    {
        ULONG error;
        BOOL b;
        const ULONG maxProcesses = 256;
        HANDLE processHandles[maxProcesses];
        STARTUPINFO startupInfo;
        HANDLE event;

        srand((ULONG)GetTickCount());

        event = CreateEvent(NULL, TRUE, FALSE, NULL);
        VERIFY_IS_TRUE(event != NULL);

        memset(&startupInfo, 0, sizeof(STARTUPINFO));
        startupInfo.cb = sizeof(STARTUPINFO);
        
        VERIFY_IS_TRUE(_NumberConcurrentHosts < maxProcesses);

        for (ULONG i = 0; i < _NumberConcurrentHosts; i++)
        {
            PROCESS_INFORMATION processInfo;

            BuildHostCommandLine(L"KtlPhysicalLogTest::KtlLLWorkloadTests::OpenCloseKtlLogManagerTest",
								 i * (_NumberLogicalLogs * sizeof(LLWORKLOADSHAREDINFO)));
            
            b = CreateProcess(NULL,                  // lpApplicationName
                              _CommandLine,
                              NULL,                  // lpProcessAttributes
                              NULL,                  // lpThreadAttributes
                              FALSE,                 // bInheritHandles
                              0,                     // dwCreationFlags
                              NULL,                  // lpEnvironment
                              NULL,                  // lpCurrentDirectory
                              &startupInfo,                  // lpStartupInfo
                              &processInfo);

            error = GetLastError();
            VERIFY_IS_TRUE(b ? true : false);

            processHandles[i] = processInfo.hProcess;
            CloseHandle(processInfo.hThread);
        }

		ULONG totalCompletedHosts = 65536;
        ULONG completedHosts = 0;
        ULONG completed;
        ULONG connectionOffset;
        LLWORKLOADSHAREDINFO* llWorkloadInfo;
        ULONG crashDelay;

        while (completedHosts < _NumberConcurrentHosts)
        {
            crashDelay = rand() % _MaxCrashDelayInSeconds;
            completed = WaitForMultipleObjects(_NumberConcurrentHosts, processHandles, FALSE, crashDelay*1000);
            VERIFY_IS_TRUE(completed != WAIT_FAILED);

            if (completed != WAIT_TIMEOUT)
            {
                completed = completed - WAIT_OBJECT_0;
                connectionOffset = completed * sizeof(LLWORKLOADSHAREDINFO);
                llWorkloadInfo = (LLWORKLOADSHAREDINFO*)
                                                       ((PUCHAR)(_IPCController.GetPointer()) + connectionOffset);

                if (llWorkloadInfo->Results.CompletionStatus != ERROR_SUCCESS)
                {
                    //
                    // Sprintf to a buffer since TAEF string.Format()
                    // doesn't support ANSI formatting %s
                    //
                    StringCbPrintf(_Temp, sizeof(_Temp),
                                   L"FAILURE %x: %ws at %hs line %d\n",
                               llWorkloadInfo->Results.CompletionStatus,
                               llWorkloadInfo->Results.ErrorMessage,
                               llWorkloadInfo->Results.ErrorFileName,
                               llWorkloadInfo->Results.ErrorLineNumber);
                    VERIFY_IS_TRUE(false, _Temp);                  
                }
                
                LOG_OUTPUT(L"Process %d completed with status %d total completed hosts %d\n", completed, llWorkloadInfo->Results.CompletionStatus, totalCompletedHosts);
                CloseHandle(processHandles[completed]);
                				
				if (totalCompletedHosts > 0)
				{
					totalCompletedHosts--;
					PROCESS_INFORMATION processInfo;

					BuildHostCommandLine(L"KtlPhysicalLogTest::KtlLLWorkloadTests::OpenCloseKtlLogManagerTest",
										 completed * (_NumberLogicalLogs * sizeof(LLWORKLOADSHAREDINFO)));

					b = CreateProcess(NULL,                  // lpApplicationName
									  _CommandLine,
									  NULL,                  // lpProcessAttributes
									  NULL,                  // lpThreadAttributes
									  FALSE,                 // bInheritHandles
									  0,                     // dwCreationFlags
									  NULL,                  // lpEnvironment
									  NULL,                  // lpCurrentDirectory
									  &startupInfo,                  // lpStartupInfo
									  &processInfo);

					error = GetLastError();
					VERIFY_IS_TRUE(b ? true : false);

					processHandles[completed] = processInfo.hProcess;
					CloseHandle(processInfo.hThread);
				} else {
					completedHosts++;
					processHandles[completed] = event;
				}
				
            } else {
                completed = rand() % _NumberConcurrentHosts;
                LOG_OUTPUT(L"Terminate process %d", completed);
                TerminateProcess(processHandles[completed], ERROR_SUCCESS);
            }
        }
        
        CloseHandle(event);

		// CONSIDER: Start and stop the driver				
    }   


	
    bool KtlLLWorkloadController::Setup()
    {

        NTSTATUS status;
        KSynchronizer sync;
        KServiceSynchronizer        serviceSync;
        String logContainerIdString;

        _NumberConcurrentHosts = 1;
        RuntimeParameters::TryGetValue(NumberConcurrentHostsName, _NumberConcurrentHosts);

        _MaxCrashDelayInSeconds = 30;
        RuntimeParameters::TryGetValue(MaxCrashDelayInSecondsName, _MaxCrashDelayInSeconds);

        _NumberLogicalLogs = 1;
        RuntimeParameters::TryGetValue(NumberLogicalLogsName, _NumberLogicalLogs);
        if (_NumberLogicalLogs == 0)
        {
            _NumberLogicalLogs = 1;
        }

        _DelayBetweenWritesInMs = 0;
        RuntimeParameters::TryGetValue(DelayBetweenWritesInMsName, _DelayBetweenWritesInMs);

        _TestDurationInSeconds = 0;
        RuntimeParameters::TryGetValue(TestDurationInSecondsName, _TestDurationInSeconds);

        _MaxRecordsWrittenPerCycle = 0;
        RuntimeParameters::TryGetValue(MaxRecordsWrittenPerCycleName, _MaxRecordsWrittenPerCycle);

        _MaxWriteLatencyInMs = 0;
        RuntimeParameters::TryGetValue(MaxWriteLatencyInMsName, _MaxWriteLatencyInMs);

        _MaxAverageWriteLatencyInMs = 0;
        RuntimeParameters::TryGetValue(MaxAverageWriteLatencyInMsName, _MaxAverageWriteLatencyInMs);
                
        
        status = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
                                       &_System);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

        _System->SetStrictAllocationChecks(TRUE);

        _StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

        EventRegisterMicrosoft_Windows_KTL();

        _DriveLetter = 0;
        status = ::FindDiskIdForDriveLetter(_DriveLetter, _DiskId);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


#if defined(UDRIVER)
        //
        // For UDRIVER, need to perform work done in PNP Device Add
        //
        status = FileObjectTable::CreateAndRegisterOverlayManager(*g_Allocator, ALLOCATION_TAG);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
        
#ifdef UPASSTHROUGH
        status = KtlLogManager::CreateInproc(ALLOCATION_TAG, *g_Allocator, _LogManager);
#else
        status = KtlLogManager::CreateDriver(ALLOCATION_TAG, *g_Allocator, _LogManager);
#endif
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = _LogManager->StartOpenLogManager(NULL, // ParentAsync
                                                 serviceSync.OpenCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Setup the IPC Container
        //
        ULONG error;
        error = _IPCController.Initialize(L"LLWorkloadTest",
                                         _NumberConcurrentHosts * _NumberLogicalLogs * sizeof(LLWORKLOADSHAREDINFO));
        VERIFY_IS_TRUE(error == ERROR_SUCCESS);
                                         
        //
        // Create log container that is used for the test
        //
        KGuid logContainerGuid;
        KtlLogContainerId logContainerId;
        ContainerCloseSynchronizer closeContainerSync;
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        LONGLONG logSize = DefaultTestLogFileSize;
        KtlLogContainer::SPtr logContainer;
        
        logContainerGuid.CreateNew();
        _LogContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

        ULONG numberStreams = (_NumberLogicalLogs * _NumberConcurrentHosts) * 4;
        if (numberStreams < 2)
        {
            numberStreams = 2;
        }
        
        status = _LogManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(_DiskId,
                                             _LogContainerId,
                                             logSize,
                                             numberStreams,            // Max Number Streams
                                             0,            // Max Record Size
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Harvest info to build the command lines to start the hosts
        //
        HRESULT hr;
        PWCHAR commandLine = GetCommandLine();
        int numArgs;
        PWCHAR* argv = CommandLineToArgvW(commandLine, &numArgs);

        hr = StringCbCopy(_TEName, sizeof(_TEName), argv[0]);
        VERIFY_IS_TRUE(SUCCEEDED(hr));
        
        *_DllName = 0;
        for (int i = 1; i < numArgs; i++)
        {
            if (wcsstr(argv[i], L".dll") != NULL)
            {
                hr = StringCbCopy(_DllName, sizeof(_DllName), argv[i]);
                VERIFY_IS_TRUE(SUCCEEDED(hr));
                break;
            }
        }

        VERIFY_IS_TRUE(*_DllName != 0);
        
        GUID g = _LogContainerId.Get();
        error = UuidToString(&g,
                             &_LogContainerIdString);
        VERIFY_IS_TRUE(error == 0);
        
        return true;
    }

    bool KtlLLWorkloadController::Cleanup()
    {
        NTSTATUS status;
        KServiceSynchronizer        serviceSync;
        KSynchronizer        sync;

        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;

        status = _LogManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteContainerAsync->StartDeleteLogContainer(_DiskId,
                                             _LogContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteContainerAsync = nullptr;
        
        
        status = _LogManager->StartCloseLogManager(NULL, // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        _LogManager = nullptr;
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

#if defined(UDRIVER)
        //
        // For UDRIVER need to perform work done in PNP RemoveDevice
        //
        status = FileObjectTable::StopAndUnregisterOverlayManager(*g_Allocator);
        KInvariant(NT_SUCCESS(status));
#endif
        
        EventUnregisterMicrosoft_Windows_KTL();

        KtlSystem::Shutdown();
        
        return true;
    }



    //
    // This is the class that implements the hosting
    //
    class KtlLLWorkloadTests : public WEX::TestClass<KtlLLWorkloadTests>
    {
    public:
        TEST_CLASS(KtlLLWorkloadTests)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

        TEST_METHOD(LLWorkloadsTest);
        TEST_METHOD(OpenCloseKtlLogManagerTest);
        
                
    public:
        KGuid _DiskId;
        UCHAR _DriveLetter;
        KtlLogManager::SPtr _LogManager;
        ULONGLONG _StartingAllocs;
        KtlSystem* _System;

        //
        // config parameters from command line
        //
        KtlLogContainerId _LogContainerId;
        ULONG _NumberLogicalLogs;
        ULONG _TestDurationInSeconds;
        ULONG _MaxRecordsWrittenPerCycle;
        ULONG _MaxWriteLatencyInMs;
        ULONG _MaxAverageWriteLatencyInMs;
        ULONG _DelayBetweenWritesInMs;

        TestIPCHost _IPCHost;
        ULONG _ConnectionOffset;
    };

    void KtlLLWorkloadTests::LLWorkloadsTest()
    {
        ULONG error;
        const ULONG maxThreads = 512;
        HANDLE threadHandles[maxThreads];
        LLWORKLOADSHAREDINFO* llWorkloadInfo;
        
        VERIFY_IS_TRUE(_NumberLogicalLogs < maxThreads);

        llWorkloadInfo = (LLWORKLOADSHAREDINFO*)((PUCHAR)(_IPCHost.GetPointer()) + _ConnectionOffset);
        for (ULONG i = 0; i < _NumberLogicalLogs; i++)
        {
            llWorkloadInfo[i].Context = this;
            llWorkloadInfo[i].AverageWriteLatencyInMs = (i+1)*0x1000;
            
            threadHandles[i] = CreateThread(NULL,           // lpThreadAttributes
                                            0,              // dwStackSize
                                            LLWorkloadThread,
                                            &llWorkloadInfo[i],
                                            0,              // dwCreationFlags
                                            NULL);          // dwThreadId

            error = GetLastError();
            VERIFY_IS_TRUE(threadHandles[i] != NULL);
        }

        WaitForMultipleObjects(_NumberLogicalLogs, threadHandles, TRUE, INFINITE);

        for (ULONG i = 0; i < _NumberLogicalLogs; i++)
        {
            // TODO: see what failed 
            CloseHandle(threadHandles[i]);
        }
    }   

    void KtlLLWorkloadTests::OpenCloseKtlLogManagerTest()
    {
        ULONG error;
        const ULONG maxThreads = 512;
        HANDLE threadHandles[maxThreads];
        LLWORKLOADSHAREDINFO* llWorkloadInfo;
        
        VERIFY_IS_TRUE(_NumberLogicalLogs < maxThreads);

        llWorkloadInfo = (LLWORKLOADSHAREDINFO*)((PUCHAR)(_IPCHost.GetPointer()) + _ConnectionOffset);
        for (ULONG i = 0; i < _NumberLogicalLogs; i++)
        {
            llWorkloadInfo[i].Context = this;
            llWorkloadInfo[i].AverageWriteLatencyInMs = (i+1)*0x1000;
            
            threadHandles[i] = CreateThread(NULL,           // lpThreadAttributes
                                            0,              // dwStackSize
                                            OpenCloseKtlLogManagerThread,
                                            &llWorkloadInfo[i],
                                            0,              // dwCreationFlags
                                            NULL);          // dwThreadId

            error = GetLastError();
            VERIFY_IS_TRUE(threadHandles[i] != NULL);
        }

        WaitForMultipleObjects(_NumberLogicalLogs, threadHandles, TRUE, INFINITE);

        for (ULONG i = 0; i < _NumberLogicalLogs; i++)
        {
            // TODO: see what failed 
            CloseHandle(threadHandles[i]);
        }
    }   
	
    bool KtlLLWorkloadTests::Setup()
    {

        NTSTATUS status;
        KServiceSynchronizer        sync;
        String logContainerIdString;
        String connectionString;
        const wchar_t* logContainerIdS;
        GUID logContainerIdGuid;

        RuntimeParameters::TryGetValue(LogContainerIdName, logContainerIdString);
        logContainerIdS= (const wchar_t*)logContainerIdString;
        status = UuidFromString((RPC_WSTR)logContainerIdS, &logContainerIdGuid);
        VERIFY_IS_TRUE(status == RPC_S_OK);
        KGuid logContainerIdKGuid(logContainerIdGuid);
        _LogContainerId = logContainerIdKGuid;

        connectionString = L"";
        RuntimeParameters::TryGetValue(TestIPCController::IPCControllerName, connectionString);     

        _ConnectionOffset = 0;
        RuntimeParameters::TryGetValue(TestIPCController::IPCControllerOffsetName, _ConnectionOffset);     
        
        _NumberLogicalLogs = 1;
        RuntimeParameters::TryGetValue(NumberLogicalLogsName, _NumberLogicalLogs);

        _TestDurationInSeconds = 0;
        RuntimeParameters::TryGetValue(TestDurationInSecondsName, _TestDurationInSeconds);

        _DelayBetweenWritesInMs = 0;
        RuntimeParameters::TryGetValue(DelayBetweenWritesInMsName, _DelayBetweenWritesInMs);
        
        _MaxRecordsWrittenPerCycle = 0;
        RuntimeParameters::TryGetValue(MaxRecordsWrittenPerCycleName, _MaxRecordsWrittenPerCycle);

        _MaxWriteLatencyInMs = 0;
        RuntimeParameters::TryGetValue(MaxWriteLatencyInMsName, _MaxWriteLatencyInMs);

        _MaxAverageWriteLatencyInMs = 0;
        RuntimeParameters::TryGetValue(MaxAverageWriteLatencyInMsName, _MaxAverageWriteLatencyInMs);
                
        
        status = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
                                       &_System);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

        _System->SetStrictAllocationChecks(TRUE);

        _StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

        EventRegisterMicrosoft_Windows_KTL();

        _DriveLetter = 0;
        status = ::FindDiskIdForDriveLetter(_DriveLetter, _DiskId);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

#ifdef UPASSTHROUGH
        status = KtlLogManager::CreateInproc(ALLOCATION_TAG, *g_Allocator, _LogManager);
#else
        status = KtlLogManager::CreateDriver(ALLOCATION_TAG, *g_Allocator, _LogManager);
#endif
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = _LogManager->StartOpenLogManager(NULL, // ParentAsync
                                                 sync.OpenCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Connect IPC host to the IPC controller
        //
        ULONG error;
        error = _IPCHost.Connect((const wchar_t*)connectionString);     
        VERIFY_IS_TRUE(error == ERROR_SUCCESS);     

        VERIFY_IS_TRUE(_IPCHost.GetSizeInBytes() >= (_NumberLogicalLogs * sizeof(LLWORKLOADSHAREDINFO)));
        
        return true;
    }

    bool KtlLLWorkloadTests::Cleanup()
    {
        NTSTATUS status;
        KServiceSynchronizer        sync;

        
        status = _LogManager->StartCloseLogManager(NULL, // ParentAsync
                                                  sync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        _LogManager = nullptr;
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        EventUnregisterMicrosoft_Windows_KTL();

        KtlSystem::Shutdown();

        return true;
    }
} 

DWORD LLWorkloadThread(
    LPVOID Context
    )
{
    NTSTATUS status;
    LLWORKLOADSHAREDINFO* llWorkloadInfo = (LLWORKLOADSHAREDINFO*)Context;  
    KtlPhysicalLogTest::KtlLLWorkloadTests* workloadTests = (KtlPhysicalLogTest::KtlLLWorkloadTests*)llWorkloadInfo->Context;
    
    llWorkloadInfo->HighestWriteLatencyInMs = 0;
    llWorkloadInfo->AverageWriteLatencyInMs = 0xf0f0f0f0;    
    llWorkloadInfo->Results.CompletionStatus = ERROR_SUCCESS;
    
    status = StartLLWorkload(workloadTests->_LogManager,
                             workloadTests->_DiskId,
                             workloadTests->_LogContainerId,
                             workloadTests->_TestDurationInSeconds,
                             workloadTests->_MaxRecordsWrittenPerCycle,
                             0,                // stream size
                             workloadTests->_MaxWriteLatencyInMs,
                             workloadTests->_MaxAverageWriteLatencyInMs,
                             workloadTests->_DelayBetweenWritesInMs,
                             llWorkloadInfo);

    return(status);
}


DWORD OpenCloseKtlLogManagerThread(
    LPVOID
    )
{
    NTSTATUS status;
    // LLWORKLOADSHAREDINFO* llWorkloadInfo = (LLWORKLOADSHAREDINFO*)Context;  
    // KtlPhysicalLogTest::KtlLLWorkloadTests* workloadTests = (KtlPhysicalLogTest::KtlLLWorkloadTests*)llWorkloadInfo->Context;
	KServiceSynchronizer        sync;


	while (TRUE)
	{
		{
			KtlLogManager::SPtr logManager;
#ifdef UPASSTHROUGH
			status = KtlLogManager::CreateInproc(ALLOCATION_TAG, *g_Allocator, logManager);
#else
			status = KtlLogManager::CreateDriver(ALLOCATION_TAG, *g_Allocator, logManager);
#endif
			VERIFY_IS_TRUE(NT_SUCCESS(status));

			status = logManager->StartOpenLogManager(NULL, // ParentAsync
													 sync.OpenCompletionCallback());
			VERIFY_IS_TRUE(NT_SUCCESS(status));
			status = sync.WaitForCompletion();
			VERIFY_IS_TRUE(NT_SUCCESS(status));

			printf("%x:%x  Log manager opened\n", GetCurrentProcessId(), GetCurrentThreadId());
			
			status = logManager->StartCloseLogManager(NULL, // ParentAsync
													  sync.CloseCompletionCallback());
			VERIFY_IS_TRUE(NT_SUCCESS(status));

			logManager = nullptr;
			status = sync.WaitForCompletion();
			VERIFY_IS_TRUE(NT_SUCCESS(status));
			
		}
	}

    return(status);
}
   
