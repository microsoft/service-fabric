// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

Common::StringLiteral const ServiceName("Trace");

Common::VariableArgument ToVA(
    __in KVariant& arg
)
{
    switch (arg.Type())
    {
    case KVariant::Type_KMemRef:
    {
        KMemRef ref = static_cast<KMemRef>(arg);
        if (ref._Size == 0)
        {
            return Common::VariableArgument(static_cast<LPCSTR>(ref._Address));
        }
        else
        {
            return Common::VariableArgument(static_cast<LPCWSTR>(ref._Address));
        }
    }

    case KVariant::Type_BOOLEAN:
        return Common::VariableArgument(static_cast<BOOLEAN>(arg));

    case KVariant::Type_LONG:
        return Common::VariableArgument(static_cast<LONG>(arg));

    case KVariant::Type_ULONG:
        return Common::VariableArgument(static_cast<ULONG>(arg));

    case KVariant::Type_LONGLONG:
        return Common::VariableArgument(static_cast<LONGLONG>(arg));

    case KVariant::Type_ULONGLONG:
        return Common::VariableArgument(static_cast<ULONGLONG>(arg));

    case KVariant::Type_GUID:
        return Common::VariableArgument(Common::Guid(static_cast<GUID>(arg)));
    }

    return Common::VariableArgument();
}

/*static*/
void NullTracer::Create(
    __out NullTracer::SPtr& spTracer,
    __in KAllocator& allocator
)
{
    spTracer = _new(TAG, allocator) NullTracer();
    KInvariant(spTracer != nullptr);
}

NullTracer::NullTracer()
{
    Common::TraceProvider::SetDefaultLevel(Common::TraceSinkType::Console, Common::LogLevel::Noise);
}

NullTracer::~NullTracer()
{
}

void NullTracer::Trace(
    __in DnsTraceLevel level,
    __in LPCSTR szFormat,
    __in KVariant arg1,
    __in KVariant arg2,
    __in KVariant arg3,
    __in KVariant arg4
)
{
    size_t size = strlen(szFormat);
    if (size == 0)
    {
        return;
    }

    Common::StringLiteral format(szFormat, &szFormat[size - 1]);

    switch (level)
    {
    case DnsTraceLevel_Noise:
        _trace.WriteNoise(ServiceName, format, ToVA(arg1), ToVA(arg2), ToVA(arg3), ToVA(arg4));
        break;

    case DnsTraceLevel_Info:
        _trace.WriteInfo(ServiceName, format, ToVA(arg1), ToVA(arg2), ToVA(arg3), ToVA(arg4));
        break;

    case DnsTraceLevel_Warning:
        _trace.WriteWarning(ServiceName, format, ToVA(arg1), ToVA(arg2), ToVA(arg3), ToVA(arg4));
        break;

    case DnsTraceLevel_Error:
        _trace.WriteError(ServiceName, format, ToVA(arg1), ToVA(arg2), ToVA(arg3), ToVA(arg4));
        break;
    }
}

void NullTracer::TracesDnsServiceStartingOpen(
    __in ULONG port
)
{
    UNREFERENCED_PARAMETER(port);
}

void NullTracer::TraceDnsExchangeOpRemoteResolve(
    __in ULONG count
)
{
    UNREFERENCED_PARAMETER(count);
}

void NullTracer::TraceDnsExchangeOpReadQuestion(
    __in ULONG count
)
{
    UNREFERENCED_PARAMETER(count);
}

void NullTracer::TraceDnsExchangeOpFabricResolve(
    __in ULONG count
)
{
    UNREFERENCED_PARAMETER(count);
}
