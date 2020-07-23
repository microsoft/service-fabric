// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef DEFINE_DACVAR
#define DEFINE_DACVAR(type, true_type, id)
#endif

#define UNKNOWN_POINTER_TYPE SIZE_T

// Use this macro to define a static var that is known to DAC, but not captured in a dump.                         
#ifndef DEFINE_DACVAR_NO_DUMP
#define DEFINE_DACVAR_NO_DUMP(type, true_type, id)
#endif

DEFINE_DACVAR(ULONG, PTR_RangeSection, ExecutionManager__m_CodeRangeList)
DEFINE_DACVAR(ULONG, PTR_EECodeManager, ExecutionManager__m_pDefaultCodeMan)
DEFINE_DACVAR(ULONG, LONG, ExecutionManager__m_dwReaderCount)
DEFINE_DACVAR(ULONG, LONG, ExecutionManager__m_dwWriterLock)

DEFINE_DACVAR(ULONG, PTR_EEJitManager, ExecutionManager__m_pEEJitManager)
#ifdef FEATURE_PREJIT
DEFINE_DACVAR(ULONG, PTR_NativeImageJitManager, ExecutionManager__m_pNativeImageJitManager)
#endif
#ifdef FEATURE_READYTORUN
DEFINE_DACVAR(ULONG, PTR_ReadyToRunJitManager, ExecutionManager__m_pReadyToRunJitManager)
#endif

DEFINE_DACVAR_NO_DUMP(ULONG, VMHELPDEF *, dac__hlpFuncTable)
DEFINE_DACVAR(ULONG, VMHELPDEF *, dac__hlpDynamicFuncTable)

#ifdef FEATURE_INCLUDE_ALL_INTERFACES
DEFINE_DACVAR(ULONG, PTR_ConnectionNameTable, CCLRDebugManager__m_pConnectionNameHash)
#endif // FEATURE_INCLUDE_ALL_INTERFACES
DEFINE_DACVAR(ULONG, PTR_StubManager, StubManager__g_pFirstManager)
DEFINE_DACVAR(ULONG, PTR_PrecodeStubManager, PrecodeStubManager__g_pManager)
DEFINE_DACVAR(ULONG, PTR_StubLinkStubManager, StubLinkStubManager__g_pManager)
DEFINE_DACVAR(ULONG, PTR_ThunkHeapStubManager, ThunkHeapStubManager__g_pManager)
DEFINE_DACVAR(ULONG, PTR_JumpStubStubManager, JumpStubStubManager__g_pManager)
DEFINE_DACVAR(ULONG, PTR_RangeSectionStubManager, RangeSectionStubManager__g_pManager)
DEFINE_DACVAR(ULONG, PTR_DelegateInvokeStubManager, DelegateInvokeStubManager__g_pManager)
DEFINE_DACVAR(ULONG, PTR_VirtualCallStubManagerManager, VirtualCallStubManagerManager__g_pManager)

DEFINE_DACVAR(ULONG, PTR_ThreadStore, ThreadStore__s_pThreadStore)

DEFINE_DACVAR(ULONG, int, ThreadpoolMgr__cpuUtilization)
DEFINE_DACVAR(ULONG, ThreadpoolMgr::ThreadCounter, ThreadpoolMgr__WorkerCounter)
DEFINE_DACVAR(ULONG, int, ThreadpoolMgr__MinLimitTotalWorkerThreads)
DEFINE_DACVAR(ULONG, DWORD, ThreadpoolMgr__MaxLimitTotalWorkerThreads)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE /*PTR_WorkRequest*/, ThreadpoolMgr__WorkRequestHead) // PTR_WorkRequest is not defined. So use a pointer type
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE  /*PTR_WorkRequest*/, ThreadpoolMgr__WorkRequestTail) // 
DEFINE_DACVAR(ULONG, ThreadpoolMgr::ThreadCounter, ThreadpoolMgr__CPThreadCounter)
DEFINE_DACVAR(ULONG, LONG, ThreadpoolMgr__MaxFreeCPThreads)
DEFINE_DACVAR(ULONG, LONG, ThreadpoolMgr__MaxLimitTotalCPThreads)
DEFINE_DACVAR(ULONG, LONG, ThreadpoolMgr__MinLimitTotalCPThreads)        
DEFINE_DACVAR(ULONG, LIST_ENTRY, ThreadpoolMgr__TimerQueue)        
DEFINE_DACVAR_NO_DUMP(ULONG, SIZE_T, dac__HillClimbingLog)
DEFINE_DACVAR(ULONG, int, dac__HillClimbingLogFirstIndex)        
DEFINE_DACVAR(ULONG, int, dac__HillClimbingLogSize)        

DEFINE_DACVAR(ULONG, PTR_Thread, dac__g_pFinalizerThread)
DEFINE_DACVAR(ULONG, PTR_Thread, dac__g_pSuspensionThread)

#ifdef FEATURE_SVR_GC
DEFINE_DACVAR(ULONG, DWORD, GCHeap__gcHeapType)
#endif // FEATURE_SVR_GC

DEFINE_DACVAR(ULONG, PTR_BYTE, WKS__gc_heap__alloc_allocated)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE /*PTR_heap_segment*/, WKS__gc_heap__ephemeral_heap_segment)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE /*PTR_CFinalize*/, WKS__gc_heap__finalize_queue)

// Can not use MULTIPLE_HEAPS here because desktop build contains it is not defined for workstation GC
// but we include workstation GC in mscorwks.dll.
#ifdef FEATURE_SVR_GC
DEFINE_DACVAR(ULONG, int, SVR__gc_heap__n_heaps)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE /*(PTR_gc_heap*)*/, SVR__gc_heap__g_heaps)
#endif // FEATURE_SVR_GC
DEFINE_DACVAR(ULONG, oom_history, WKS__gc_heap__oom_info)

DEFINE_DACVAR(ULONG, PTR_SystemDomain, SystemDomain__m_pSystemDomain)
DEFINE_DACVAR(ULONG, ArrayListStatic, SystemDomain__m_appDomainIndexList)
DEFINE_DACVAR(ULONG, BOOL, SystemDomain__s_fForceDebug)
DEFINE_DACVAR(ULONG, BOOL, SystemDomain__s_fForceProfiling)
DEFINE_DACVAR(ULONG, BOOL, SystemDomain__s_fForceInstrument)
DEFINE_DACVAR(ULONG, PTR_SharedDomain, SharedDomain__m_pSharedDomain)


DEFINE_DACVAR(ULONG, DWORD, CExecutionEngine__TlsIndex)

DEFINE_DACVAR(ULONG, LONG, CNameSpace__m_GcStructuresInvalidCnt)

#if defined(FEATURE_INCLUDE_ALL_INTERFACES) || defined(FEATURE_WINDOWSPHONE)
DEFINE_DACVAR(ULONG, int, CCLRErrorReportingManager__g_ECustomDumpFlavor)
#endif

DEFINE_DACVAR(ULONG, PTR_SString, SString__s_Empty)

#ifdef FEATURE_APPX
#if defined(FEATURE_CORECLR)
DEFINE_DACVAR(ULONG, BOOL, dac__g_fAppX)
#else
DEFINE_DACVAR(ULONG, PTR_AppXRTInfo, dac__g_pAppXRTInfo)
#endif
#endif // FEATURE_APPX

DEFINE_DACVAR(ULONG, BOOL, SString__s_IsANSIMultibyte)

#ifdef FEATURE_REMOTING
DEFINE_DACVAR_NO_DUMP(ULONG, MethodTable, CTPMethodTable__s_pThunkTable)
#endif // FEATURE_REMOTING

DEFINE_DACVAR(ULONG, INT32, ArrayBase__s_arrayBoundsZero)

DEFINE_DACVAR(ULONG, BOOL, StackwalkCache__s_Enabled)

DEFINE_DACVAR(ULONG, PTR_JITNotification, dac__g_pNotificationTable)
DEFINE_DACVAR(ULONG, ULONG32, dac__g_dacNotificationFlags)
DEFINE_DACVAR(ULONG, PTR_GcNotification, dac__g_pGcNotificationTable)

#ifndef FEATURE_IMPLICIT_TLS
DEFINE_DACVAR(ULONG, DWORD, dac__gThreadTLSIndex)
DEFINE_DACVAR(ULONG, DWORD, dac__gAppDomainTLSIndex)
#endif

DEFINE_DACVAR(ULONG, PTR_EEConfig, dac__g_pConfig)

DEFINE_DACVAR(ULONG, MscorlibBinder, dac__g_Mscorlib)

#if defined(PROFILING_SUPPORTED) || defined(PROFILING_SUPPORTED_DATA)
DEFINE_DACVAR(ULONG, ProfControlBlock, dac__g_profControlBlock)
#endif // defined(PROFILING_SUPPORTED) || defined(PROFILING_SUPPORTED_DATA)

DEFINE_DACVAR_NO_DUMP(ULONG, SIZE_T, dac__generation_table)
DEFINE_DACVAR(ULONG, PTR_DWORD, dac__g_card_table)
DEFINE_DACVAR(ULONG, PTR_BYTE, dac__g_lowest_address)
DEFINE_DACVAR(ULONG, PTR_BYTE, dac__g_highest_address)

DEFINE_DACVAR(ULONG, GCHeap, dac__g_pGCHeap)

DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pThinLockThreadIdDispenser)    
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pModuleIndexDispenser)    
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pObjectClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pRuntimeTypeClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pCanonMethodTableClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pStringClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pArrayClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pSZArrayHelperClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pNullableClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pExceptionClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pThreadAbortExceptionClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pOutOfMemoryExceptionClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pStackOverflowExceptionClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pExecutionEngineExceptionClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pDelegateClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pMulticastDelegateClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pFreeObjectMethodTable)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pOverlappedDataClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pValueTypeClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pEnumClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pThreadClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pCriticalFinalizerObjectClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pAsyncFileStream_AsyncResultClass)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pPredefinedArrayTypes)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_ArgumentHandleMT)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_ArgIteratorMT)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_TypedReferenceMT)

#ifdef FEATURE_COMINTEROP
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pBaseCOMObject)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pBaseRuntimeClass)
#endif

DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pPrepareConstrainedRegionsMethod)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pExecuteBackoutCodeHelperMethod)

DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pObjectCtorMD)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pObjectFinalizerMD)

DEFINE_DACVAR(ULONG, bool, dac__g_fProcessDetach)
DEFINE_DACVAR(ULONG, DWORD, dac__g_fEEShutDown)
DEFINE_DACVAR(ULONG, DWORD, dac__g_fHostConfig)

DEFINE_DACVAR(ULONG, ULONG, dac__g_CORDebuggerControlFlags)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pDebugger)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pDebugInterface)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pEEDbgInterfaceImpl)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pEEInterface)

DEFINE_DACVAR(ULONG, BOOL, Debugger__s_fCanChangeNgenFlags)

DEFINE_DACVAR(ULONG, PTR_DebuggerPatchTable, DebuggerController__g_patches)
DEFINE_DACVAR(ULONG, BOOL, DebuggerController__g_patchTableValid)


DEFINE_DACVAR(ULONG, SIZE_T, dac__gLowestFCall)
DEFINE_DACVAR(ULONG, SIZE_T, dac__gHighestFCall)
DEFINE_DACVAR(ULONG, SIZE_T, dac__gFCallMethods)

DEFINE_DACVAR(ULONG, PTR_SyncTableEntry, dac__g_pSyncTable)
#ifdef FEATURE_COMINTEROP
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pRCWCleanupList)
DEFINE_DACVAR(ULONG, BOOL, RCWWalker__s_bIsGlobalPeggingOn);
#endif // FEATURE_COMINTEROP

#ifndef FEATURE_PAL
DEFINE_DACVAR(ULONG, SIZE_T, dac__g_runtimeLoadedBaseAddress)
DEFINE_DACVAR(ULONG, SIZE_T, dac__g_runtimeVirtualSize)
#endif // !FEATURE_PAL

DEFINE_DACVAR(ULONG, SyncBlockCache *, SyncBlockCache__s_pSyncBlockCache)

DEFINE_DACVAR(ULONG, HandleTableMap, dac__g_HandleTableMap)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pStressLog)

DEFINE_DACVAR(ULONG, SIZE_T, dac__s_gsCookie)

#ifdef FEATURE_IPCMAN
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE, dac__g_pIPCManagerInterface)
#endif // FEATURE_IPCMAN

DEFINE_DACVAR_NO_DUMP(ULONG, SIZE_T, dac__g_FCDynamicallyAssignedImplementations)

DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE /*BYTE**/, WKS__gc_heap__internal_root_array)
DEFINE_DACVAR(ULONG, size_t, WKS__gc_heap__internal_root_array_index)
DEFINE_DACVAR(ULONG, ULONG, WKS__gc_heap__heap_analyze_success)

DEFINE_DACVAR(ULONG, SIZE_T, WKS__gc_heap__mark_array)
DEFINE_DACVAR(ULONG, SIZE_T, WKS__gc_heap__current_c_gc_state)
DEFINE_DACVAR(ULONG, PTR_BYTE, WKS__gc_heap__next_sweep_obj)
DEFINE_DACVAR(ULONG, UNKNOWN_POINTER_TYPE /* PTR_heap_segment */, WKS__gc_heap__saved_sweep_ephemeral_seg)
DEFINE_DACVAR(ULONG, PTR_BYTE, WKS__gc_heap__saved_sweep_ephemeral_start)
DEFINE_DACVAR(ULONG, PTR_BYTE, WKS__gc_heap__background_saved_lowest_address)
DEFINE_DACVAR(ULONG, PTR_BYTE, WKS__gc_heap__background_saved_highest_address)

#ifdef FEATURE_CORECLR
DEFINE_DACVAR(ULONG, HANDLE, dac__g_hContinueStartupEvent)
DEFINE_DACVAR(ULONG, DWORD, CorHost2__m_dwStartupFlags)
#endif // FEATURE_CORECLR

DEFINE_DACVAR(ULONG, HRESULT, dac__g_hrFatalError)

#if defined(DEBUGGING_SUPPORTED) && defined (FEATURE_PREJIT)
    DEFINE_DACVAR(ULONG, DWORD, PEFile__s_NGENDebugFlags)
#endif //defined(DEBUGGING_SUPPORTED) && defined (FEATURE_PREJIT)

#ifndef FEATURE_CORECLR
DEFINE_DACVAR(ULONG, DWORD, AssemblyUsageLogManager__s_UsageLogFlags)
#endif // FEATURE_CORECLR

#if defined(FEATURE_HOSTED_BINDER) && defined(FEATURE_APPX_BINDER)
DEFINE_DACVAR(ULONG, PTR_CLRPrivBinderAppX, CLRPrivBinderAppX__s_pSingleton)
#endif //defined(FEATURE_HOSTED_BINDER) && defined(FEATURE_APPX)

#ifdef FEATURE_MINIMETADATA_IN_TRIAGEDUMPS
DEFINE_DACVAR(ULONG, DWORD, dac__g_MiniMetaDataBuffMaxSize)
DEFINE_DACVAR(ULONG, TADDR, dac__g_MiniMetaDataBuffAddress)
#endif // FEATURE_MINIMETADATA_IN_TRIAGEDUMPS

#undef DEFINE_DACVAR
#undef DEFINE_DACVAR_NO_DUMP
