// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS { namespace Test
{
    using ::_delete;

    class NullTracer :
        public KShared<NullTracer>,
        public IDnsTracer
    {
        K_FORCE_SHARED(NullTracer);
        K_SHARED_INTERFACE_IMP(IDnsTracer);

    public:
        static void Create(
            __out NullTracer::SPtr& spTracer,
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
        )override;

        virtual void TraceDnsExchangeOpRemoteResolve(
            __in ULONG port
        ) override;

        virtual void TraceDnsExchangeOpReadQuestion(
            __in ULONG port
        ) override;

        virtual void TraceDnsExchangeOpFabricResolve(
            __in ULONG port
        ) override;

    private:
        Common::TextTraceComponent<Common::TraceTaskCodes::DNS> _trace;
    };
}}
