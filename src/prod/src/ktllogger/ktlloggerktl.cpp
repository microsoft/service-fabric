// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace KtlLogger;

const ULONG KtlLogContainer::MaxAliasLength; 

//
// KtlLoggerInitializationContext
//
VOID
KtlLoggerInitializationContext::OnCompleted(
    )
{
    _LogManager = nullptr;
    _LogContainer = nullptr;
    _ConfigureAsync = nullptr;
    _InBuffer = nullptr;
    _OutBuffer = nullptr;
}

VOID
KtlLoggerInitializationContext::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &KtlLoggerInitializationContext::OperationCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        _KtlLoggerNode->WriteTraceError(__LINE__, _State, Status);
    }

    switch (_State)
    {
        case Initial:
        {
            KAsyncServiceBase::OpenCompletionCallback openCompletion(this, &KtlLoggerInitializationContext::OpenCloseCompletion);

            _LogManager = nullptr;
            _LogContainer = nullptr;

            if (_UseInprocLogger)
            {
                Status = KtlLogManager::CreateInproc(GetThisAllocationTag(), GetThisAllocator(), _LogManager);
            } else {
                Status = KtlLogManager::CreateDriver(GetThisAllocationTag(), GetThisAllocator(), _LogManager);
            }
            
            if (! NT_SUCCESS(Status))
            {               
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _KtlLoggerNode->WriteTraceError(__LINE__, _State, Status);
                Complete(Status);
                return;
            }

            _State = OpenLogManagerX;
            Status = _LogManager->StartOpenLogManager(this,
                                                     openCompletion);
            if (! NT_SUCCESS(Status))
            {               
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _KtlLoggerNode->WriteTraceError(__LINE__, _State, Status);
                Complete(Status);
                return;
            }
            break;
        }
        
        case OpenLogManagerX:
        {
            if (! NT_SUCCESS(Status))
            {
                _KtlLoggerNode->WriteTraceError(__LINE__, _State, Status);
                Complete(Status);
                return;
            }

            //
            // Beyond this point all failures must goto
            // StartCleanupOnError
            //
            
            Status = _LogManager->CreateAsyncConfigureContext(_ConfigureAsync);
            if (! NT_SUCCESS(Status))
            {               
                KTraceFailedAsyncRequest(Status, this, _State, 0);              
                _KtlLoggerNode->WriteTraceError(__LINE__, _State, Status);
                goto StartCleanupOnError;
            }

            Status = KBuffer::Create(sizeof(KtlLogManager::MemoryThrottleLimits), _InBuffer, GetThisAllocator());
            if (! NT_SUCCESS(Status))
            {               
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _KtlLoggerNode->WriteTraceError(__LINE__, _State, Status);
                goto StartCleanupOnError;
            }
            
            KtlLogManager::MemoryThrottleLimits* memoryThrottleLimits = (KtlLogManager::MemoryThrottleLimits*)_InBuffer->GetBuffer();
            *memoryThrottleLimits = _MemoryLimits;

            KDbgCheckpointWDataInformational(0,
                                "ConfigureMemoryThrottleLimits", Status,
                                (ULONGLONG)(memoryThrottleLimits->WriteBufferMemoryPoolMax),
                                (ULONGLONG)(memoryThrottleLimits->WriteBufferMemoryPoolMin),
                                (ULONGLONG)(memoryThrottleLimits->WriteBufferMemoryPoolPerStream),
                                (ULONGLONG)((100000000 * memoryThrottleLimits->PeriodicFlushTimeInSec) +
                                           memoryThrottleLimits->PeriodicTimerIntervalInSec));


            _State = ConfigureThrottleSettings3;
            _ConfigureAsync->StartConfigure(KtlLogManager::ConfigureCode::ConfigureMemoryThrottleLimits3,
                                            _InBuffer.RawPtr(),
                                            _Result,
                                            _OutBuffer,
                                            this,
                                            completion);
            break;
        }
        
        case ConfigureThrottleSettings3:
        {
            if (! NT_SUCCESS(Status))
            {
                //
                // If ConfigureMemoryThrottleLimits3 failed then we may
                // have an older driver. In this case retry with the
                // older ConfigureMemoryThrottleLimits2
                //
                _State = ConfigureThrottleSettings2;
                _ConfigureAsync->StartConfigure(KtlLogManager::ConfigureCode::ConfigureMemoryThrottleLimits2,
                                                _InBuffer.RawPtr(),
                                                _Result,
                                                _OutBuffer,
                                                this,
                                                completion);
                break;
            }

            goto ConfigureThrottleSettings;
        }

        case ConfigureThrottleSettings2:
        {
            if (! NT_SUCCESS(Status))
            {
                //
                // If ConfigureMemoryThrottleLimits2 failed then we may
                // have an older driver. In this case retry with the
                // older ConfigureMemoryThrottleLimits
                //
                _State = ConfigureThrottleSettings;
                _ConfigureAsync->Reuse();
                _ConfigureAsync->StartConfigure(KtlLogManager::ConfigureCode::ConfigureMemoryThrottleLimits,
                                                _InBuffer.RawPtr(),
                                                _Result,
                                                _OutBuffer,
                                                this,
                                                completion);
                break;
            }
            
            // Fall through
        }
        
        case ConfigureThrottleSettings:
        {
ConfigureThrottleSettings:          
            if (! NT_SUCCESS(Status))
            {               
                _KtlLoggerNode->WriteTraceError(__LINE__, _State, Status);
                goto StartCleanupOnError;
            }

            Status = KBuffer::Create(sizeof(KtlLogManager::AcceleratedFlushLimits), _InBuffer, GetThisAllocator());
            if (! NT_SUCCESS(Status))
            {               
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _KtlLoggerNode->WriteTraceError(__LINE__, _State, Status);
                goto StartCleanupOnError;
            }
            
            KtlLogManager::AcceleratedFlushLimits* accelerateFlushLimits = (KtlLogManager::AcceleratedFlushLimits*)_InBuffer->GetBuffer();
            *accelerateFlushLimits = _AccelerateFlushLimits;
            
            _State = ConfigureAccelerateFlushSettings;
            _ConfigureAsync->Reuse();
            _ConfigureAsync->StartConfigure(KtlLogManager::ConfigureCode::ConfigureAcceleratedFlushLimits,
                                            _InBuffer.RawPtr(),
                                            _Result,
                                            _OutBuffer,
                                            this,
                                            completion);
            break;

            
        }

        case ConfigureAccelerateFlushSettings:
        {            
            if (! NT_SUCCESS(Status))
            {               
                _KtlLoggerNode->WriteTraceError(__LINE__, _State, Status);
                goto StartCleanupOnError;
            }
            
            _InBuffer = nullptr;
            _OutBuffer = nullptr;
            _ConfigureAsync->Reuse();
            
            Status = KBuffer::Create(sizeof(KtlLogManager::SharedLogContainerSettings), _InBuffer, GetThisAllocator());
            if (! NT_SUCCESS(Status))
            {               
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _KtlLoggerNode->WriteTraceError(__LINE__, _State, Status);
                goto StartCleanupOnError;
            }

            KtlLogManager::SharedLogContainerSettings* sharedLogSettings = (KtlLogManager::SharedLogContainerSettings*)_InBuffer->GetBuffer();
            *sharedLogSettings = _SharedLogSettings;            

            KDbgPrintfInformational("Configure shared log %ws LogId %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x \n",
                sharedLogSettings->Path, 
                sharedLogSettings->LogContainerId.Get().Data1, 
                sharedLogSettings->LogContainerId.Get().Data2, 
                sharedLogSettings->LogContainerId.Get().Data3, 
                sharedLogSettings->LogContainerId.Get().Data4[0],
                sharedLogSettings->LogContainerId.Get().Data4[1],
                sharedLogSettings->LogContainerId.Get().Data4[2],
                sharedLogSettings->LogContainerId.Get().Data4[3],
                sharedLogSettings->LogContainerId.Get().Data4[4],
                sharedLogSettings->LogContainerId.Get().Data4[5],
                sharedLogSettings->LogContainerId.Get().Data4[6],
                sharedLogSettings->LogContainerId.Get().Data4[7]);
            
            KDbgCheckpointWDataInformational(0,
                                "ConfigureSharedLogSettings", Status,
                                (ULONGLONG)sharedLogSettings->LogContainerId.Get().Data1,
                                (ULONGLONG)sharedLogSettings->LogSize,
                                (ULONGLONG)((100000000 * sharedLogSettings->MaximumNumberStreams) +
                                            sharedLogSettings->MaximumRecordSize),
                                (ULONGLONG)0);

            //
            // For now, skip the shared log destaging step as this has
            // been causing random bugchecks.
            //
#if 0
            _State = ConfigureSharedLog;
#else
            _State = CloseSharedLog;
#endif
            _ConfigureAsync->StartConfigure(KtlLogManager::ConfigureCode::ConfigureSharedLogContainerSettings,
                                            _InBuffer.RawPtr(),
                                            _Result,
                                            _OutBuffer,
                                            this,
                                            completion);
            break;
        }
        
        case ConfigureSharedLog:
        {
            KtlLogManager::AsyncOpenLogContainer::SPtr openLogContainer;
            
            if (! NT_SUCCESS(Status))
            {
                goto StartCleanupOnError;
            }
            
            _State = OpenSharedLog;

            Status = _LogManager->CreateAsyncOpenLogContainerContext(openLogContainer);
            if (! NT_SUCCESS(Status))
            {               
                KTraceFailedAsyncRequest(Status, this, _State, 0);

                //
                // Failing to open the log container is not a fatal
                // problem and can be ignored.
                //
                Status = STATUS_SUCCESS;
                goto StartCleanupOnError;
            }

            //
            // Map shared log guid to default if need be
            //
            static const KGuid guidNull(0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
            static const KGuid guidSharedLogDefault(0x3CA2CCDA, 0xDD0F, 0x49c8, 0xA7, 0x41, 0x62, 0xAA, 0xC0, 0xD4, 0xEB, 0x62);
            KGuid guidSharedLog;

            if (_SharedLogSettings.LogContainerId.Get() == guidNull)
            {
                guidSharedLog = guidSharedLogDefault;
            }
            else 
            {
                guidSharedLog = _SharedLogSettings.LogContainerId.Get();
            }

            //
            // Map shared log path to default if need be
            //
            KString::SPtr path;
            if (*_SharedLogSettings.Path == 0)
            {
                Status = KString::Create(path, GetThisAllocator(), MAX_PATH);
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    
                    //
                    // Failing to open the log container is not a fatal
                    // problem and can be ignored.
                    //
                    Status = STATUS_SUCCESS;
                    goto StartCleanupOnError;
                }

#if defined(PLATFORM_UNIX)
                Status = path;
#else
                Status = path->Concat(L"\\??\\");
#endif
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    
                    //
                    // Failing to open the log container is not a fatal
                    // problem and can be ignored.
                    //
                    Status = STATUS_SUCCESS;
                    goto StartCleanupOnError;
                }

                Status = path->Concat(_NodeWorkDirectory);
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    
                    //
                    // Failing to open the log container is not a fatal
                    // problem and can be ignored.
                    //
                    Status = STATUS_SUCCESS;
                    goto StartCleanupOnError;
                }

                Status = path->Concat(L"\\ReplicatorLog\\ReplicatorShared.log");
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    
                    //
                    // Failing to open the log container is not a fatal
                    // problem and can be ignored.
                    //
                    Status = STATUS_SUCCESS;
                    goto StartCleanupOnError;
                }
            }
            else 
            {
                path = KString::Create(_SharedLogSettings.Path,
                                       GetThisAllocator());
                
                if (path == nullptr)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    KTraceFailedAsyncRequest(Status, this, _State, 0);

                    //
                    // Failing to open the log container is not a fatal
                    // problem and can be ignored.
                    //
                    Status = STATUS_SUCCESS;
                    goto StartCleanupOnError;
                }

            }

            KDbgPrintfInformational("Try destage shared log %ws LogId %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x \n", (PWSTR)*path, 
                guidSharedLog.Data1, 
                guidSharedLog.Data2, 
                guidSharedLog.Data3, 
                guidSharedLog.Data4[0],
                guidSharedLog.Data4[1],
                guidSharedLog.Data4[2],
                guidSharedLog.Data4[3],
                guidSharedLog.Data4[4],
                guidSharedLog.Data4[5],
                guidSharedLog.Data4[6],
                guidSharedLog.Data4[7]);
            
            openLogContainer->StartOpenLogContainer(*path,
                                                    guidSharedLog,
                                                    _LogContainer,
                                                    this,
                                                    completion);

            
            break;
        }
        
        case OpenSharedLog:
        {
            if (! NT_SUCCESS(Status))
            {               
                //
                // Failing to open the log container is not a fatal
                // problem and can be ignored.
                //
                Status = STATUS_SUCCESS;
                goto StartCleanupOnError;
            }
            
            _State = CloseSharedLog;
            
            KtlLogContainer::CloseCompletionCallback containerCloseCompletion(this, &KtlLoggerInitializationContext::CloseContainerCompletion);
            _LogContainer->StartClose(this,
                                      containerCloseCompletion);
            
            break;
        }

        case CloseSharedLog:
        {
            Status = STATUS_SUCCESS;
            
StartCleanupOnError:
            _State = CloseLogManagerX;

            _FinalStatus = Status;
            
            KAsyncServiceBase::CloseCompletionCallback closeCompletion(this, &KtlLoggerInitializationContext::OpenCloseCompletion);            
            Status = _LogManager->StartCloseLogManager(this,
                                                       closeCompletion);
            
            if (! NT_SUCCESS(Status))
            {
                //
                // In this case we aren't able to close the log
                // manager so it will get cleaned up when the process
                // dies
                //
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _KtlLoggerNode->WriteTraceError(__LINE__, _State, Status);
                Complete(Status);
                return;
            }
            break;
        }

        case CloseLogManagerX:
        {
            Complete(_FinalStatus);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
KtlLoggerInitializationContext::CloseContainerCompletion(
    __in_opt KAsyncContextBase* const Parent,
    __in KtlLogContainer& LogContainer,
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(Parent);
    UNREFERENCED_PARAMETER(LogContainer);

    FSMContinue(Status);
}    

VOID KtlLoggerInitializationContext::OpenCloseCompletion(
    __in KAsyncContextBase* const ParentAsync,
    __in KAsyncServiceBase& Async,
    __in NTSTATUS Status)
{
    UNREFERENCED_PARAMETER(ParentAsync);
    UNREFERENCED_PARAMETER(Async);
    FSMContinue(Status);
}

VOID
KtlLoggerInitializationContext::OperationCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
KtlLoggerInitializationContext::OnStart(
    )
{
    if (! NT_SUCCESS(_FinalStatus))
    {
        Complete(_FinalStatus);
        return;
    }
    
    _State = Initial;

    FSMContinue(STATUS_SUCCESS);   
}

VOID
KtlLoggerInitializationContext::StartInitializeKtlLogger(
    __in BOOLEAN UseInprocLogger,
    __in KtlLogManager::MemoryThrottleLimits& MemoryLimits,
    __in KtlLogManager::AcceleratedFlushLimits& AccelerateFlushLimits,
    __in KtlLogManager::SharedLogContainerSettings& SharedLogSettings,
    __in LPCWSTR NodeWorkDirectory,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    HRESULT hr;

    _UseInprocLogger = UseInprocLogger;
    _MemoryLimits = MemoryLimits;
    _AccelerateFlushLimits = AccelerateFlushLimits;
    _SharedLogSettings = SharedLogSettings;

    hr = StringCchCopy(_NodeWorkDirectory, MAX_PATH, NodeWorkDirectory);

    if (! SUCCEEDED(hr))
    {
        _FinalStatus = STATUS_INVALID_PARAMETER;
        _KtlLoggerNode->WriteTraceError(__LINE__, _State, hr);
    } else {
        _FinalStatus = STATUS_SUCCESS;
    }

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KtlLoggerInitializationContext::OnReuse(
    )
{
}

VOID
KtlLoggerInitializationContext::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

KtlLoggerInitializationContext::KtlLoggerInitializationContext()
{
}

KtlLoggerInitializationContext::~KtlLoggerInitializationContext()
{
}

NTSTATUS
KtlLoggerInitializationContext::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in KtlLoggerNode* KtlLoggerNode,
    __out KtlLoggerInitializationContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLoggerInitializationContext::SPtr context;

    
    context = _new(AllocationTag, Allocator) KtlLoggerInitializationContext();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, AllocationTag, 0);
        return(status);
        
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_KtlLoggerNode = KtlLoggerNode;
    
    Context = Ktl::Move(context);
    
    return(STATUS_SUCCESS); 
}

