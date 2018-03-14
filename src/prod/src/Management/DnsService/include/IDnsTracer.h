// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    #define WSTR(value__) KMemRef(1, (PVOID) ((((LPCWSTR)value__) != nullptr) ? ((LPCWSTR)value__) : L""))

    #define STR(value__) KMemRef(0, (PVOID) ((((LPCSTR)value__) != nullptr) ? ((LPCSTR)value__) : ""))

    enum DnsTraceLevel
    {
        DnsTraceLevel_Noise = 1,
        DnsTraceLevel_Info,
        DnsTraceLevel_Warning,
        DnsTraceLevel_Error
    };

    interface IDnsTracer
    {
        K_SHARED_INTERFACE(IDnsTracer);

    public:
        virtual void Trace(
            __in DnsTraceLevel level,
            __in LPCSTR szFormat,
            __in KVariant arg1 = KVariant(),
            __in KVariant arg2 = KVariant(),
            __in KVariant arg3 = KVariant(),
            __in KVariant arg4 = KVariant()
        ) = 0;

        virtual void TracesDnsServiceStartingOpen(
            __in ULONG port
        ) = 0;

        virtual void TraceDnsExchangeOpRemoteResolve(
            __in ULONG count
        ) = 0;

        virtual void TraceDnsExchangeOpReadQuestion(
            __in ULONG count
        ) = 0;

        virtual void TraceDnsExchangeOpFabricResolve(
            __in ULONG count
        ) = 0;
    };
}


