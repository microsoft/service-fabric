// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/StackTrace.h"

#include <execinfo.h>
#include <cxxabi.h>

using namespace Common;

void StackTrace::CaptureFromContext( CONTEXT* contextRecord, bool ignoreFirstFrame)
{
}

void StackTrace::WriteTo( TextWriter& w, const Common::FormatOptions& ) const
{
    if (frameCount <= 1)
    {
        w << "Missing frames (frameCount = " << frameCount << ")";
        return;
    }

    BackTrace bt(address, frameCount);
    bt.ResolveSymbols();

    w << bt;
}

std::wstring StackTrace::ToString() const
{
    std::wstring result;
    Common::StringWriter(result).Write(*this);
    return result;
}

void StackTrace::CaptureCurrentPosition()
{
    frameCount = backtrace(address, sizeof(address) / sizeof(void*));
    if ((frameCount > 0) && (address[frameCount-1] == 0))
    {
        --frameCount; //remove trailing 0, backtrace() bug? 
    }
}
