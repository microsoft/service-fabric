// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#if defined(PLATFORM_UNIX)
#include <boost/test/unit_test.hpp>
#include "boost-taef.h"
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>

#include "KtlPhysicalLog.h"

#include "MockRvdLogStream.h"

#if !defined(PLATFORM_UNIX)
#include "WexTestClass.h"

using namespace WEX::Logging;
using namespace WEX::Common;
using namespace WEX::TestExecution;
#endif

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

RvdLogStreamShim::RvdLogStreamShim()
{
}

RvdLogStreamShim::~RvdLogStreamShim()
{
}


LONGLONG
RvdLogStreamShim::QueryLogStreamUsage()
{
    return(_CoreLogStream->QueryLogStreamUsage());
}

VOID
RvdLogStreamShim::QueryLogStreamType(__out RvdLogStreamType& LogStreamType)
{
    _CoreLogStream->QueryLogStreamType(LogStreamType);
}

VOID
RvdLogStreamShim::QueryLogStreamId(__out RvdLogStreamId& LogStreamId)
{
    _CoreLogStream->QueryLogStreamId(LogStreamId);
}

NTSTATUS
RvdLogStreamShim::QueryRecordRange(
    __out_opt RvdLogAsn* const LowestAsn,
    __out_opt RvdLogAsn* const HighestAsn,
    __out_opt RvdLogAsn* const LogTruncationAsn)
{
    return(_CoreLogStream->QueryRecordRange(LowestAsn, HighestAsn, LogTruncationAsn));
}

ULONGLONG
RvdLogStreamShim::QueryCurrentReservation()
{
    return(_CoreLogStream->QueryCurrentReservation());
}

NTSTATUS
RvdLogStreamShim::SetTruncationCompletionEvent(__in_opt KAsyncEvent* const EventToSignal)
{
    return(_CoreLogStream->SetTruncationCompletionEvent(EventToSignal));
}


NTSTATUS
RvdLogStreamShim::CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context)
{
    return(_CoreLogStream->CreateAsyncWriteContext(Context));
}

NTSTATUS
RvdLogStreamShim::CreateAsyncReadContext(__out AsyncReadContext::SPtr& Context)
{
    return(_CoreLogStream->CreateAsyncReadContext(Context));
}

NTSTATUS
RvdLogStreamShim::QueryRecord(
    __in RvdLogAsn RecordAsn,
    __out_opt ULONGLONG* const Version,
    __out_opt RvdLogStream::RecordDisposition* const Disposition,
    __out_opt ULONG* const IoBufferSize,
    __out_opt ULONGLONG* const DebugInfo1)
{
    return(_CoreLogStream->QueryRecord(RecordAsn,
                                       Version,
                                       Disposition,
                                       IoBufferSize,
                                       DebugInfo1));
}


NTSTATUS
RvdLogStreamShim::QueryRecord(
    __inout RvdLogAsn& RecordAsn,
    __in RvdLogStream::AsyncReadContext::ReadType Type,
    __out_opt ULONGLONG* const Version,
    __out_opt RvdLogStream::RecordDisposition* const Disposition,
    __out_opt ULONG* const IoBufferSize,
    __out_opt ULONGLONG* const DebugInfo1)
{
    return(_CoreLogStream->QueryRecord(RecordAsn,
                                       Type,
                                       Version,
                                       Disposition,
                                       IoBufferSize,
                                       DebugInfo1));
}


NTSTATUS
RvdLogStreamShim::QueryRecords(
    __in RvdLogAsn LowestAsn,
    __in RvdLogAsn HighestAsn,
    __in KArray<RvdLogStream::RecordMetadata>& ResultsArray)
{
    return(_CoreLogStream->QueryRecords(LowestAsn,
                                        HighestAsn,
                                        ResultsArray));
}

NTSTATUS
RvdLogStreamShim::DeleteRecord(
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version)
{
    return(_CoreLogStream->DeleteRecord(RecordAsn,
                                        Version));
}

VOID
RvdLogStreamShim::Truncate(__in RvdLogAsn TruncationPoint, __in RvdLogAsn PreferredTruncationPoint)
{
    _CoreLogStream->Truncate(TruncationPoint, PreferredTruncationPoint);
}

VOID
RvdLogStreamShim::TruncateBelowVersion(__in RvdLogAsn TruncationPoint, __in ULONGLONG Version)
{
    _CoreLogStream->TruncateBelowVersion(TruncationPoint, Version);
}

NTSTATUS
RvdLogStreamShim::CreateUpdateReservationContext(__out AsyncReservationContext::SPtr& Context)
{
    return(_CoreLogStream->CreateUpdateReservationContext(Context));
}
