// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <string>
#include <PAL.h>
#include "SFBDCtlLibTrace.h"

std::string SFBDCtlLibTrace::tracePrefix_ = "";

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
