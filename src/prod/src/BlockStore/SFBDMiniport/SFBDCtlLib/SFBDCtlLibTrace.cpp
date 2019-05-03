// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

// Convert the printf/wprintf format to string
// Write formatted output to buffer and construct string from buffer
// Only Support char/wchar_t for now
template <class T>
std::string SFBDCtlLibTrace::format(const T *formatstring, ...)
{
    try
    {
        va_list args;
        va_start(args, formatstring);

        // Get the number of characters
        size_t nSize = (typeid(T) == typeid(wchar_t)) ? _vscwprintf((const wchar_t*)formatstring, args) : _vscprintf((const char *)formatstring, args);
        
        // To handle formatstring == "" or null pointer
        if (nSize <= 0)
        {
            throw "Empty string or null point";
        }

        // Plus 1 for terminating '\0'
        nSize++;

        vector<T> buffer(nSize);
        // The third parameter count: Maximum number of characters to write (not including the terminating null)
        if (typeid(T) == typeid(wchar_t))
        {
            _vsnwprintf_s((wchar_t *)&buffer[0], nSize, nSize - 1, (const wchar_t*)formatstring, args);
        }
        else
        {
            _vsnprintf_s((char *)&buffer[0], nSize, nSize - 1, (const char *)formatstring, args);
        }
        
        va_end(args);

        return string(buffer.begin(), buffer.end());
    }
    catch (std::exception &ex)
    {
        return "Unable to write trace due to exception: " + string(ex.what());
    }
}

template string SFBDCtlLibTrace::format(const char *formatstring, ...);
template string SFBDCtlLibTrace::format(const wchar_t *formatstring, ...);

string SFBDCtlLibTrace::tracePrefix_ = "";

#ifdef ETL_TRACING_ENABLED
using namespace Common;
StringLiteral const TraceComponent("SFBDControlLib");

void SFBDCtlLibTrace::TraceError(string expr)
{
#ifdef ETL_TRACE_TO_FILE_ENABLED
    TraceInit();
#endif
    Trace.WriteError(TraceComponent, "{0}", tracePrefix_ + expr);
}

void SFBDCtlLibTrace::TraceWarning(string expr)
{
#ifdef ETL_TRACE_TO_FILE_ENABLED
    TraceInit();
#endif
    Trace.WriteWarning(TraceComponent, "{0}", tracePrefix_ + expr);
}

void SFBDCtlLibTrace::TraceInfo(string expr)
{
#ifdef ETL_TRACE_TO_FILE_ENABLED
    TraceInit();
#endif
    Trace.WriteInfo(TraceComponent, "{0}", tracePrefix_ + expr);
}

void SFBDCtlLibTrace::TraceNoise(string expr)
{
#ifdef ETL_TRACE_TO_FILE_ENABLED
    TraceInit();
#endif
    Trace.WriteNoise(TraceComponent, "{0}", tracePrefix_ + expr);
}

#ifdef ETL_TRACE_TO_FILE_ENABLED
std::atomic<bool> SFBDCtlLibTrace::fInitialized_(false);
void SFBDCtlLibTrace::TraceInit()
{
    // Only do init once
    if (fInitialized_) return;

    ComPointer<IFabricNodeContextResult> nodeContext;
    ComPointer<IFabricCodePackageActivationContext> activationContext;
    ASSERT_IFNOT(
        ::FabricGetNodeContext(nodeContext.VoidInitializationAddress()) == S_OK,
        "Failed to create fabric runtime");

    ASSERT_IFNOT(
        ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContext.VoidInitializationAddress()) == S_OK,
        "Failed to get activation context in EnableTracing");
    FABRIC_NODE_CONTEXT const * nodeContextResult = nodeContext->get_NodeContext();

    wstring traceFileName = wformatString("{0}_{1}", nodeContextResult->NodeName, DateTime::Now().Ticks);
    wstring traceFilePath = Path::Combine(activationContext->get_WorkDirectory(), traceFileName);

    TraceProvider::GetSingleton()->SetDefaultLevel(TraceSinkType::ETW, LogLevel::Info);
    TraceTextFileSink::SetOption(L"");
    TraceTextFileSink::SetPath(traceFilePath);

    wstring traceLog = wformatString("Node Name = {0}, IP = {1}", nodeContextResult->NodeName, nodeContextResult->IPAddressOrFQDN);
    Trace.WriteInfo(TraceComponent, "{0}", string(traceLog.begin(), traceLog.end()));

    fInitialized_ = true;
}
#endif //ETL_TRACE_TO_FILE_ENABLED

#endif //ETL_TRACING_ENABLED