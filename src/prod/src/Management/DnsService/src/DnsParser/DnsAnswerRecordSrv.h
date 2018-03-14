// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "DnsAnswerRecordBase.h"

namespace DNS
{
    using ::_delete;

    class DnsAnswerRecordSrv :
        public DnsAnswerRecordBase
    {
        K_FORCE_SHARED(DnsAnswerRecordSrv);

    public:
        static void Create(
            __out DnsAnswerRecordSrv::SPtr& spRecord,
            __in KAllocator& allocator,
            __in IDnsRecord& question,
            __in KString& strHost,
            __in USHORT port
        );

    private:
        DnsAnswerRecordSrv(
            __in IDnsRecord& question,
            __in KString& strHost,
            __in USHORT port
        );

    public:
        virtual PVOID ValuePtr() override { return static_cast<PVOID>(&_port); }
        virtual KString::SPtr DebugString() const override;

        KString& Host() const { return *_spHost; }
        USHORT Port() const { return _port; }

    public:
        bool Serialize(
            __in BinaryWriter& writer,
            __in ULONG ttl
        ) const;

    private:
        USHORT _port;
        KString::SPtr _spHost;
    };
}
