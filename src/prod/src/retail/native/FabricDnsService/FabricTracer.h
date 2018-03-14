// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "DnsEventSource.h"

namespace DNS
{
    using ::_delete;

    class FabricTracer :
        public KShared<FabricTracer>,
        public IDnsTracer
    {
        K_FORCE_SHARED(FabricTracer);
        K_SHARED_INTERFACE_IMP(IDnsTracer);

    public:
        static void Create(
            __out FabricTracer::SPtr& spTracer,
            __in KAllocator& allocator
        );

    public:
        virtual void Trace(
            __in DnsTraceLevel level,
            __in LPCSTR szFormat,
            __in KVariant arg1 = KVariant(),
            __in KVariant arg2 = KVariant(),
            __in KVariant arg3 = KVariant(),
            __in KVariant arg4 = KVariant()
        ) override;

        virtual void TracesDnsServiceStartingOpen(
            __in ULONG port
        ) override;

        virtual void TraceDnsExchangeOpRemoteResolve(
            __in ULONG count
        ) override;

        virtual void TraceDnsExchangeOpReadQuestion(
            __in ULONG count
        ) override;

        virtual void TraceDnsExchangeOpFabricResolve(
            __in ULONG count
        ) override;

    private:
        static Common::VariableArgument ToVA(
            __in KVariant& arg
        );

    private:
        Common::TextTraceComponent<Common::TraceTaskCodes::DNS> _trace;
        DnsEventSource _eventSource;
    };
}
