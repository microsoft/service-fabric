// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>

#include "KtlPhysicalLog.h"

#include "WexTestClass.h"

using namespace WEX::Logging;
using namespace WEX::Common;
using namespace WEX::TestExecution;

#include "VerifyQuiet.h"

#if KTL_USER_MODE
 #define _printf(...)   printf(__VA_ARGS__)
// #define _printf(...)   KDbgPrintf(__VA_ARGS__)

extern volatile LONGLONG gs_AllocsRemaining;
#else
 #define _printf(...)   KDbgPrintf(__VA_ARGS__)
 #define wprintf(...)    KDbgPrintf(__VA_ARGS__)
#endif

#define ALLOCATION_TAG 'LLKT'
 
extern KAllocator* g_Allocator;


//
// Shim classes
//
class RvdLogStreamShim  : public RvdLogStream
{
    K_FORCE_SHARED_WITH_INHERITANCE(RvdLogStreamShim);

public:
    LONGLONG
    QueryLogStreamUsage() override ;
    
    VOID
    QueryLogStreamType(__out RvdLogStreamType& LogStreamType) override ;

    VOID
    QueryLogStreamId(__out RvdLogStreamId& LogStreamId) override ;

    NTSTATUS
    QueryRecordRange(
        __out_opt RvdLogAsn* const LowestAsn,
        __out_opt RvdLogAsn* const HighestAsn,
        __out_opt RvdLogAsn* const LogTruncationAsn) override ;

    ULONGLONG QueryCurrentReservation() override ;

    class AsyncWriteContextShim : public RvdLogStream::AsyncWriteContext
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncWriteContextShim);

    public:
        VOID
        StartWrite(
            __in BOOLEAN LowPriorityIO,
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

        VOID
        StartWrite(
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

        VOID
        StartReservedWrite(
            __in ULONGLONG ReserveToUse,
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
        
        VOID
        StartReservedWrite(
            __in BOOLEAN LowPriorityIO,
            __in ULONGLONG ReserveToUse,
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
    };

    NTSTATUS
    CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context) override ;

    class AsyncReadContextShim : public RvdLogStream::AsyncReadContext
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncReadContextShim);

    public:
        VOID
        StartRead(
            __in RvdLogAsn RecordAsn,
            __out_opt ULONGLONG* const Version,
            __out KBuffer::SPtr& MetaDataBuffer,
            __out KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

        typedef enum { ReadTypeNotSpecified, ReadExactRecord, ReadNextRecord, ReadPreviousRecord, ReadContainingRecord, 
                       ReadNextFromSpecificAsn, ReadPreviousFromSpecificAsn }
        ReadType;
        
        VOID
        StartRead(
            __in RvdLogAsn RecordAsn,
            __in RvdLogStream::AsyncReadContext::ReadType Type,
            __out_opt RvdLogAsn* const ActualRecordAsn,
            __out_opt ULONGLONG* const Version,
            __out KBuffer::SPtr& MetaDataBuffer,
            __out KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;        
    };

    NTSTATUS
    CreateAsyncReadContext(__out AsyncReadContext::SPtr& Context) override ;
	
			
    NTSTATUS
    QueryRecord(
        __in RvdLogAsn RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out_opt RvdLogStream::RecordDisposition* const Disposition,
        __out_opt ULONG* const IoBufferSize = nullptr,
        __out_opt ULONGLONG* const DebugInfo1 = nullptr) override ;

    
    NTSTATUS
    QueryRecord(
        __inout RvdLogAsn& RecordAsn,
        __in RvdLogStream::AsyncReadContext::ReadType Type,
        __out_opt ULONGLONG* const Version,
        __out_opt RvdLogStream::RecordDisposition* const Disposition,
        __out_opt ULONG* const IoBufferSize = nullptr,
        __out_opt ULONGLONG* const DebugInfo1 = nullptr) override ;

	
    NTSTATUS
    QueryRecords(
        __in RvdLogAsn LowestAsn,
        __in RvdLogAsn HighestAsn,
        __in KArray<RvdLogStream::RecordMetadata>& ResultsArray) override ;

    NTSTATUS
    DeleteRecord(
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version) override ;
    
    VOID
    Truncate(__in RvdLogAsn TruncationPoint, __in RvdLogAsn PreferredTruncationPoint) override ;

    VOID
    TruncateBelowVersion(__in RvdLogAsn TruncationPoint, __in ULONGLONG Version) override ;

    class AsyncReservationContextShim : public RvdLogStream::AsyncReservationContext
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncReservationContextShim);

    public:
        VOID
        StartUpdateReservation(
            __in LONGLONG Delta,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
    };

    NTSTATUS
    CreateUpdateReservationContext(__out AsyncReservationContext::SPtr& Context) override ;

	private:
		RvdLogStream::SPtr _CoreLogStream;
};

class RvdLogShim : public RvdLog
{
    K_FORCE_SHARED_WITH_INHERITANCE(RvdLogShim);

public:
    VOID
    QueryLogType(__out KWString& LogType) override ;

    VOID
    QueryLogId(__out KGuid& DiskId, __out RvdLogId& LogId) override ;

    VOID
    QuerySpaceInformation(
        __out_opt ULONGLONG* const TotalSpace,
        __out_opt ULONGLONG* const FreeSpace = nullptr) override ;

    ULONGLONG QueryReservedSpace() override ;
	
    ULONGLONG QueryCurrentReservation() override ;

    ULONG QueryMaxAllowedStreams() override ;

    ULONG QueryMaxRecordSize() override ;

    ULONG QueryMaxUserRecordSize() override ;

    ULONG QueryUserRecordSystemMetadataOverhead() override ;
	
    VOID
    QueryCacheSize(__out_opt LONGLONG* const ReadCacheSizeLimit, __out_opt LONGLONG* const ReadCacheUsage) override ;

    VOID
    SetCacheSize(__in LONGLONG CacheSizeLimit) override ;

    class AsyncCreateLogStreamContextShim : public RvdLog::AsyncCreateLogStreamContext
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncCreateLogStreamContextShim);
    public:
        VOID
        StartCreateLogStream(
            __in const RvdLogStreamId& LogStreamId,
            __in const RvdLogStreamType& LogStreamType,
            __out RvdLogStream::SPtr& LogStream,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
    };

    NTSTATUS
    CreateAsyncCreateLogStreamContext(__out AsyncCreateLogStreamContext::SPtr& Context) override ;

    class AsyncDeleteLogStreamContextShim : public RvdLog::AsyncDeleteLogStreamContext
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncDeleteLogStreamContextShim);
    public:
        VOID
        StartDeleteLogStream(
            __in const RvdLogStreamId& LogStreamId,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
    };

    NTSTATUS
    CreateAsyncDeleteLogStreamContext(__out AsyncDeleteLogStreamContext::SPtr& Context) override ;

    class AsyncOpenLogStreamContextShim : public RvdLog::AsyncOpenLogStreamContext
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncOpenLogStreamContextShim);
    public:
        VOID
        StartOpenLogStream(
            __in const RvdLogStreamId& LogStreamId,
            __out RvdLogStream::SPtr& LogStream,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
    };

    NTSTATUS
    CreateAsyncOpenLogStreamContext(__out AsyncOpenLogStreamContext::SPtr& Context) override ;

    BOOLEAN
    IsStreamIdValid(__in RvdLogStreamId StreamId) override ;

    BOOLEAN
    GetStreamState(
        __in RvdLogStreamId StreamId,
        __out_opt BOOLEAN * const IsOpen = nullptr,
        __out_opt BOOLEAN * const IsClosed = nullptr,
        __out_opt BOOLEAN * const IsDeleted = nullptr) override ;

    BOOLEAN
    IsLogFlushed() override ;

    ULONG
    GetNumberOfStreams() override ;

    ULONG
    GetStreams(__in ULONG MaxResults, __in RvdLogStreamId* Streams) override ;

    NTSTATUS
    SetShutdownEvent(__in_opt KAsyncEvent* const EventToSignal) override ;

	private:
		RvdLog::SPtr _CoreLog;
};

class RvdLogManagerShim : public RvdLogManager
{
    K_FORCE_SHARED_WITH_INHERITANCE(RvdLogManagerShim);

public:
    static NTSTATUS
    Create(__in ULONG AllocationTag, __in KAllocator& Allocator, __out RvdLogManager::SPtr& Result);

    NTSTATUS
    Activate(
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

    VOID
    Deactivate() override ;

    class AsyncEnumerateLogsShim : public RvdLogManager::AsyncEnumerateLogs
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncEnumerateLogsShim);

    public:
        VOID
        StartEnumerateLogs(
            __in const KGuid& DiskId,
            __out KArray<RvdLogId>& LogIdArray,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

	protected:
		VOID
		OnStart(
			) override ;

		VOID
		OnReuse(
			) override ;

		VOID
		OnCancel(
			) override ;

	private:
		VOID OperationCompletion(
			__in_opt KAsyncContextBase* const ParentAsync,
			__in KAsyncContextBase& Async
			);
		
	private:
		//
		// General members
		//

		//
		// Parameters to api
		//
		KGuid _DiskId;
		KArray<RvdLogId>* _LogIdArray;

		//
		// Members needed for functionality
		//		
		RvdLogManager::AsyncEnumerateLogs::SPtr _CoreEnumerateLogs;
    };

    NTSTATUS
    CreateAsyncEnumerateLogsContext(__out AsyncEnumerateLogs::SPtr& Context)= 0;

    class AsyncCreateLogShim : public RvdLogManager::AsyncCreateLog
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncCreateLogShim);

    public:
        VOID
        StartCreateLog(
            __in KGuid const& DiskId,
            __in RvdLogId const& LogId,
            __in KWString& LogType,
            __in LONGLONG LogSize,
            __out RvdLog::SPtr& Log,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

        VOID
        StartCreateLog(
            __in KGuid const& DiskId,
            __in RvdLogId const& LogId,
            __in KWString& LogType,
            __in LONGLONG LogSize,
            __in ULONG MaxAllowedStreams,
            __in ULONG MaxRecordSize,
            __out RvdLog::SPtr& Log,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

        VOID
        StartCreateLog(
            __in KStringView const& FullyQualifiedLogFilePath,
            __in RvdLogId const& LogId,
            __in KWString& LogType,
            __in LONGLONG LogSize,
            __in ULONG MaxAllowedStreams,
            __in ULONG MaxRecordSize,
            __out RvdLog::SPtr& Log,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

        VOID
        StartCreateLog(
            __in KGuid const& DiskId,
            __in RvdLogId const& LogId,
            __in KWString& LogType,
            __in LONGLONG LogSize,
            __in ULONG MaxAllowedStreams,
            __in ULONG MaxRecordSize,
            __in DWORD Flags,
            __out RvdLog::SPtr& Log,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

        VOID
        StartCreateLog(
            __in KStringView const& FullyQualifiedLogFilePath,
            __in RvdLogId const& LogId,
            __in KWString& LogType,
            __in LONGLONG LogSize,
            __in ULONG MaxAllowedStreams,
            __in ULONG MaxRecordSize,
            __in DWORD Flags,
            __out RvdLog::SPtr& Log,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

	protected:
		VOID
		OnStart(
			) override ;

		VOID
		OnReuse(
			) override ;

		VOID
		OnCancel(
			) override ;

	private:
		VOID OperationCompletion(
			__in_opt KAsyncContextBase* const ParentAsync,
			__in KAsyncContextBase& Async
			);
		
	private:		
	    KGuid _DiskId;
		RvdLogId _LogId;
		KWString* _LogType,
		LONGLONG _LogSize;
		ULONG _MaxAllowedStreams;
		ULONG _MaxRecordSize;
		DWORD _Flags;
		RvdLog::SPtr* _Log;

	    RvdLogManager::AsyncCreateLog::SPtr _CoreCreateLog;
    };

    NTSTATUS
    CreateAsyncCreateLogContext(__out AsyncCreateLog::SPtr& Context) override ;

    class AsyncOpenLogShim : public RvdLogManager::AsyncOpenLog
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncOpenLogShim);

    public:
        VOID
        StartOpenLog(
            __in const KGuid& DiskId,
            __in const RvdLogId& LogId,
            __out RvdLog::SPtr& Log,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

        VOID
        StartOpenLog(
            __in KStringView& FullyQualifiedLogFilePath,
            __out RvdLog::SPtr& Log,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
		
	protected:
		VOID
		OnStart(
			) override ;

		VOID
		OnReuse(
			) override ;

		VOID
		OnCancel(
			) override ;

	private:
		VOID OperationCompletion(
			__in_opt KAsyncContextBase* const ParentAsync,
			__in KAsyncContextBase& Async
			);
		
	private:
		RvdLogManager::AsyncOpenLog::SPtr _CoreOpenLog;
    };

    NTSTATUS
    CreateAsyncOpenLogContext(__out AsyncOpenLog::SPtr& Context) override ;

    class AsyncDeleteLogShim : public RvdLogManager::AsyncDeleteLog
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncDeleteLogShim);

    public:
        VOID
        StartDeleteLog(
            __in const KGuid& DiskId,
            __in const RvdLogId& LogId,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

        VOID
        StartDeleteLog(
            __in KStringView& FullyQualifiedLogFilePath,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

	protected:
		VOID
		OnStart(
			) override ;

		VOID
		OnReuse(
			) override ;

		VOID
		OnCancel(
			) override ;

	private:
		VOID OperationCompletion(
			__in_opt KAsyncContextBase* const ParentAsync,
			__in KAsyncContextBase& Async
			);		
		
	private:
		RvdLogManager::AsyncDeleteLog::SPtr _CoreDeleteLog;
    };

    NTSTATUS
    CreateAsyncDeleteLogContext(__out AsyncDeleteLog::SPtr& Context) override ;


    NTSTATUS
    RegisterVerificationCallback(
        __in const RvdLogStreamType& LogStreamType,
        __in VerificationCallback Callback) override ;

    VerificationCallback
    UnRegisterVerificationCallback(__in const RvdLogStreamType& LogStreamType) override ;

    VerificationCallback
    QueryVerificationCallback(__in const RvdLogStreamType& LogStreamType) override ;

	private:
		NTSTATUS CreateRvdLogShim(__in RvdLog& CoreRvdLog,
								  __out RvdLogShim::SPtr& Context);
		
	private:
		RvdLogManager::SPtr _CoreLogManager;		
};

//
// RvdLogManagerShim
//
NTSTATUS
RvdLogManagerShim::Create(__in ULONG AllocationTag, __in KAllocator& Allocator, __out RvdLogManager::SPtr& Result)
{
	NTSTATUS status;
	RvdLogManagerShim::SPtr context;
    RvdLogManager::SPtr coreLogManager;

	context = _new(AllocationTag, Allocator) RvdLogManagerShim();
	if (! context)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return(status);		
	}

	status = context->Status();
	if (! NT_SUCCESS(status))
	{
		return(status);
	}
	
    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), coreLogManager);
	if (! NT_SUCCESS(status))
	{
		return(status);
	}

	_CoreLogManager = coreLogManager;

	Context = context.RawPtr();
	return(status);
}

NTSTATUS
RvdLogManagerShim::Activate(
	__in_opt KAsyncContextBase* const ParentAsyncContext,
	__in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
	_CoreLogManager->Activate(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogManagerShim::Deactivate()
{
	_CoreLogManager->Deactivate();
}

//
// AsyncEnumerateLogsShim
//
VOID
RvdLogManagerShim::AsyncEnumerateLogsShim::OperationCompletion(
	__in_opt KAsyncContextBase* const ParentAsync,
	__in KAsyncContextBase& Async
	)
{
	UNREFERENCED_PARAMETER(ParentAsync);
	Complete(Async.Status());
}

VOID
RvdLogManagerShim::AsyncEnumerateLogsShim::OnStart(
	)
{
	_CoreEnumerateLogs->StartEnumerateLogs(_DiskId, *_LogIdArray);
}

VOID
RvdLogManagerShim::AsyncEnumerateLogsShim::OnReuse(
	)
{
	_CoreEnumerateLogs->Reuse();
}

VOID
RvdLogManagerShim::AsyncEnumerateLogsShim::OnCancel(
	)
{
	_CoreEnumerateLogs->Cancel();
}

VOID
RvdLogManagerShim::AsyncEnumerateLogsShim::StartEnumerateLogs(
	__in const KGuid& DiskId,
	__out KArray<RvdLogId>& LogIdArray,
	__in_opt KAsyncContextBase* const ParentAsyncContext,
	__in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
	_DiskId = DiskId;
	_LogIdArray = &_LogIdArray;
}

NTSTATUS
RvdLogManagerShim::CreateAsyncEnumerateLogsContext(__out AsyncEnumerateLogs::SPtr& Context)
{
    NTSTATUS status;
	AsyncEnumerateLogsShim::SPtr context;
	AsyncEnumerateLogs::SPtr coreEnumerateLogs;
	
	context = _new(GetThisAllocationTag(), GetThisAllocator) AsyncEnumerateLogsShim();
	if (! context)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return(status);
	}

	status = context->Status();
	if (! NT_SUCCESS(status))
	{
		return(status);
	}

	status = _CoreLogManager->CreateAsyncEnumerateLogsContext(coreEnumerateLogs);
	if (! NT_SUCCESS(status))
	{
		return(status);
	}

	context->_CoreEnumerateLogs = coreEnumerateLogs;

	Context = context.RawPtr();
	return(status);
}

//
// AsyncCreateLogShim
//
VOID
RvdLogManagerShim::AsyncCreateLogShim::OperationCompletion(
	__in_opt KAsyncContextBase* const ParentAsync,
	__in KAsyncContextBase& Async
	)
{
	UNREFERENCED_PARAMETER(ParentAsync);
	Complete(Async.Status());
}

VOID
RvdLogManagerShim::AsyncCreateLogShim::OnStart(
	)
{
	_CoreCreateLog->StartCreateLog(_DiskId, *_LogIdArray);  // TODO: 
}

VOID
RvdLogManagerShim::AsyncCreateLogShim::OnReuse(
	)
{
	_CoreEnumerateLog->Reuse();
}

VOID
RvdLogManagerShim::AsyncCreateLogShim::OnCancel(
	)
{
	_CoreCreateLog->Cancel();
}

VOID
RvdLogManagerShim::AsyncCreateLogShim::StartCreateLog(
	__in KGuid const& DiskId,
	__in RvdLogId const& LogId,
	__in KWString& LogType,
	__in LONGLONG LogSize,
	__out RvdLog::SPtr& Log,
	__in_opt KAsyncContextBase* const ParentAsyncContext,
	__in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{

}

VOID
RvdLogManagerShim::AsyncCreateLogShim::StartCreateLog(
	__in KGuid const& DiskId,
	__in RvdLogId const& LogId,
	__in KWString& LogType,
	__in LONGLONG LogSize,
	__in ULONG MaxAllowedStreams,
	__in ULONG MaxRecordSize,
	__out RvdLog::SPtr& Log,
	__in_opt KAsyncContextBase* const ParentAsyncContext,
	__in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
}

VOID
RvdLogManagerShim::AsyncCreateLogShim::StartCreateLog(
	__in KStringView const& FullyQualifiedLogFilePath,
	__in RvdLogId const& LogId,
	__in KWString& LogType,
	__in LONGLONG LogSize,
	__in ULONG MaxAllowedStreams,
	__in ULONG MaxRecordSize,
	__out RvdLog::SPtr& Log,
	__in_opt KAsyncContextBase* const ParentAsyncContext,
	__in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
}

VOID
RvdLogManagerShim::AsyncCreateLogShim::StartCreateLog(
	__in KGuid const& DiskId,
	__in RvdLogId const& LogId,
	__in KWString& LogType,
	__in LONGLONG LogSize,
	__in ULONG MaxAllowedStreams,
	__in ULONG MaxRecordSize,
	__in DWORD Flags,
	__out RvdLog::SPtr& Log,
	__in_opt KAsyncContextBase* const ParentAsyncContext,
	__in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
}

VOID
StartCreateLog(
	__in KStringView const& FullyQualifiedLogFilePath,
	__in RvdLogId const& LogId,
	__in KWString& LogType,
	__in LONGLONG LogSize,
	__in ULONG MaxAllowedStreams,
	__in ULONG MaxRecordSize,
	__in DWORD Flags,
	__out RvdLog::SPtr& Log,
	__in_opt KAsyncContextBase* const ParentAsyncContext,
	__in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
}


NTSTATUS
RvdLogManagerShim::CreateAsyncCreateLogContext(__out AsyncCreateLogShim::SPtr& Context)
{
    NTSTATUS status;
	AsyncCreateLogShim::SPtr context;
	AsyncCreateLogShim::SPtr coreCreateLog;
	
	context = _new(GetThisAllocationTag(), GetThisAllocator) AsyncCreateLogShim();
	if (! context)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return(status);
	}

	status = context->Status();
	if (! NT_SUCCESS(status))
	{
		return(status);
	}

	status = _CoreLogManager->CreateAsyncCreateLogContext(coreEnumerateLogs);
	if (! NT_SUCCESS(status))
	{
		return(status);
	}

	context->_CoreEnumerateLogs = coreEnumerateLogs;

	Context = context.RawPtr();
	return(status);
}






NTSTATUS
CreateAsyncCreateLogContext(__out AsyncCreateLog::SPtr& Context) override ;

NTSTATUS
CreateAsyncOpenLogContext(__out AsyncOpenLog::SPtr& Context) override ;

NTSTATUS
CreateAsyncDeleteLogContext(__out AsyncDeleteLog::SPtr& Context) override ;


NTSTATUS
RegisterVerificationCallback(
	__in const RvdLogStreamType& LogStreamType,
	__in VerificationCallback Callback) override ;

VerificationCallback
UnRegisterVerificationCallback(__in const RvdLogStreamType& LogStreamType) override ;

VerificationCallback
QueryVerificationCallback(__in const RvdLogStreamType& LogStreamType) override ;

//
// RvdLog
//
VOID
QueryLogType(__out KWString& LogType) override ;

VOID
QueryLogId(__out KGuid& DiskId, __out RvdLogId& LogId) override ;

VOID
QuerySpaceInformation(
	__out_opt ULONGLONG* const TotalSpace,
	__out_opt ULONGLONG* const FreeSpace = nullptr) override ;

ULONGLONG QueryReservedSpace() override ;

ULONGLONG QueryCurrentReservation() override ;

ULONG QueryMaxAllowedStreams() override ;

ULONG QueryMaxRecordSize() override ;

ULONG QueryMaxUserRecordSize() override ;

ULONG QueryUserRecordSystemMetadataOverhead() override ;

VOID
QueryCacheSize(__out_opt LONGLONG* const ReadCacheSizeLimit, __out_opt LONGLONG* const ReadCacheUsage) override ;

VOID
SetCacheSize(__in LONGLONG CacheSizeLimit) override ;

NTSTATUS
CreateAsyncCreateLogStreamContext(__out AsyncCreateLogStreamContext::SPtr& Context) override ;

NTSTATUS
CreateAsyncDeleteLogStreamContext(__out AsyncDeleteLogStreamContext::SPtr& Context) override ;

NTSTATUS
CreateAsyncOpenLogStreamContext(__out AsyncOpenLogStreamContext::SPtr& Context) override ;

BOOLEAN
IsStreamIdValid(__in RvdLogStreamId StreamId) override ;

BOOLEAN
GetStreamState(
	__in RvdLogStreamId StreamId,
	__out_opt BOOLEAN * const IsOpen = nullptr,
	__out_opt BOOLEAN * const IsClosed = nullptr,
	__out_opt BOOLEAN * const IsDeleted = nullptr) override ;

BOOLEAN
IsLogFlushed() override ;

ULONG
GetNumberOfStreams() override ;

ULONG
GetStreams(__in ULONG MaxResults, __in RvdLogStreamId* Streams) override ;

NTSTATUS
SetShutdownEvent(__in_opt KAsyncEvent* const EventToSignal) override ;

//
// RvdLogStream
//
LONGLONG
QueryLogStreamUsage() override ;

VOID
QueryLogStreamType(__out RvdLogStreamType& LogStreamType) override ;

VOID
QueryLogStreamId(__out RvdLogStreamId& LogStreamId) override ;

NTSTATUS
QueryRecordRange(
	__out_opt RvdLogAsn* const LowestAsn,
	__out_opt RvdLogAsn* const HighestAsn,
	__out_opt RvdLogAsn* const LogTruncationAsn) override ;

ULONGLONG QueryCurrentReservation() override ;

NTSTATUS
CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context) override ;

NTSTATUS
CreateAsyncReadContext(__out AsyncReadContext::SPtr& Context) override ;


NTSTATUS
QueryRecord(
	__in RvdLogAsn RecordAsn,
	__out_opt ULONGLONG* const Version,
	__out_opt RvdLogStream::RecordDisposition* const Disposition,
	__out_opt ULONG* const IoBufferSize = nullptr,
	__out_opt ULONGLONG* const DebugInfo1 = nullptr) override ;


NTSTATUS
QueryRecord(
	__inout RvdLogAsn& RecordAsn,
	__in RvdLogStream::AsyncReadContext::ReadType Type,
	__out_opt ULONGLONG* const Version,
	__out_opt RvdLogStream::RecordDisposition* const Disposition,
	__out_opt ULONG* const IoBufferSize = nullptr,
	__out_opt ULONGLONG* const DebugInfo1 = nullptr) override ;


NTSTATUS
QueryRecords(
	__in RvdLogAsn LowestAsn,
	__in RvdLogAsn HighestAsn,
	__in KArray<RvdLogStream::RecordMetadata>& ResultsArray) override ;

NTSTATUS
DeleteRecord(
	__in RvdLogAsn RecordAsn,
	__in ULONGLONG Version) override ;

VOID
Truncate(__in RvdLogAsn TruncationPoint, __in RvdLogAsn PreferredTruncationPoint) override ;

VOID
TruncateBelowVersion(__in RvdLogAsn TruncationPoint, __in ULONGLONG Version) override ;

NTSTATUS
CreateUpdateReservationContext(__out AsyncReservationContext::SPtr& Context) override ;

