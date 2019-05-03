// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/StackTrace.h"
#include <dbghelp.h>

using namespace Common;

namespace
{
    Global<RwLock> DbgHelpApiLock = make_global<RwLock>();
}

void StackTrace::WriteTo( TextWriter& w, const Common::FormatOptions& ) const
{
    auto start = Stopwatch::Now();
    auto sympath = FabricSymInitializeOnce();
    auto loadingTime = Stopwatch::Now() - start;
    w << "Symbol paths: " << sympath << "\r\n";
    w << wformatString("Symbol loading time: {0}\r\n", loadingTime);
    w.WriteLine("Stack trace:");

    if (frameCount <= 1)
    {
        w << "Missing frames (frameCount = " << frameCount << ")";
        return;
    }

    // lock is needed because SymGetSymFromAddr64 is Dbghelp API, not MT safe.
    AcquireExclusiveLock acquire(*DbgHelpApiLock);

    for ( int i = 0; i < frameCount - 1; ++i )
    {
        DWORD64     disp;
        DWORD       disp2;
        BYTE        buf[sizeof( IMAGEHLP_SYMBOL64 ) + SYMBOL_NAME_MAX];
        std::string funcName;
        memset( buf, 0, sizeof( IMAGEHLP_SYMBOL64 ) + SYMBOL_NAME_MAX );

        IMAGEHLP_SYMBOL64* symbol = reinterpret_cast<IMAGEHLP_SYMBOL64*>( buf );
        symbol->SizeOfStruct = sizeof( buf );
        symbol->MaxNameLength = SYMBOL_NAME_MAX;

        if ( SymGetSymFromAddr64( GetCurrentProcess(), address[i], &disp, symbol ))
        {
            funcName.assign( symbol->Name );

            if ( funcName.compare( "RaiseException" ) == 0 ||
                funcName.compare( "NLG_Return" ) == 0 ||
                funcName.compare( "EH_prolog" ) == 0 ||
                funcName.compare( "NlsMbOemCodePageTag" ) == 0 ||
                funcName.compare( "RtlSetUserValueHeap" ) == 0 ||
                funcName.compare( "LdrUnloadDll" ) == 0 ||
                funcName.compare( "RtlLengthSecurityDescriptor" ) == 0 ||
                funcName.compare( "GetModuleFileNameA" ) == 0 ||
                funcName.compare( "_unDNameEx" ) == 0 ||
                funcName.compare( "RtlTimeToSecondsSince1980" ) == 0 ||
                funcName.compare( "RtlDeregisterWait" ) == 0 ||
                funcName.compare( "RtlAddAccessAllowedAceEx" ) == 0 ||
                funcName.compare( "KiUserApcDispatcher" ) == 0 ||
                funcName.compare( "_CxxExceptionFilter" ) == 0 ||
                funcName.compare( "_CxxFrameHandler2" ) == 0 ||
                funcName.compare( "RtlRaiseStatus" ) == 0 ||

                funcName.compare( "RtlIpv6StringToAddressW" ) == 0 ||
                funcName.compare( "RtlTimeFieldsToTime" ) == 0 ||
                funcName.compare( "GetModuleHandleA" ) == 0 ||

                funcName.compare( "_CxxThrowException" ) == 0 ||
                funcName.compare( "CxxThrowException" ) == 0 ||
                funcName.compare( "Common::LeakDetector::Init" ) == 0 ||
                funcName.compare( "Common::RefCounted::operator new" ) == 0 ||
                funcName.compare( "RtlUpcaseUnicodeStringToOemString" ) == 0 ||
                funcName.compare( "FlsSetValue" ) == 0
                )
                continue;

            if ( funcName.compare( "wmainCRTStartup" ) == 0 )
                break;

            w << formatString("    {0} + 0x{1:x}", funcName, disp);

            IMAGEHLP_LINE64 lineInfo;
            ::ZeroMemory( &lineInfo, sizeof lineInfo );
            lineInfo.SizeOfStruct = sizeof( lineInfo );

            if ( SymGetLineFromAddr64( GetCurrentProcess(), address[i], &disp2, &lineInfo ))
            {
                w << L"  [" << std::string(lineInfo.FileName ) << ':' << lineInfo.LineNumber << ']';
            }
            w << L"\r\n";
        }
        else
        {
            LARGE_INTEGER q;
            q.QuadPart = address[i];
            w << formatString( "    {0:08x}:{1:08x}( {2} )\r\n", q.HighPart, q.LowPart, microsoft::GetLastErrorCode() );
        }
    }
} // StackTrace::WriteTo

std::wstring StackTrace::ToString() const
{
    std::wstring result;
    Common::StringWriter(result).Write(*this);
    return result;
}

void StackTrace::CaptureCurrentPosition()
{
    // unlike SymGetSymFromAddr64, RtlCaptureStackBackTrace is not DbgHelp API, which is not mt-safe
    // so RtlCaptureStackBackTrace call does not need to be protected by a process wide lock
    frameCount = RtlCaptureStackBackTrace(
        0,
        MAX_FRAME,
        (PVOID *)address,
        nullptr);
}
