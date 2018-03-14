// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "DnsAnswerRecordBase.h"

namespace DNS
{
    using ::_delete;

    class DnsAnswerRecordA :
        public DnsAnswerRecordBase
    {
        K_FORCE_SHARED(DnsAnswerRecordA);

    public:
        static void Create(
            __out DnsAnswerRecordA::SPtr& spRecord,
            __in KAllocator& allocator,
            __in IDnsRecord& question,
            __in ULONG address
        );

    private:
        DnsAnswerRecordA(
            __in IDnsRecord& question,
            __in ULONG address
        );

    public:
        static bool Deserialize(
            __out DnsAnswerRecordA::SPtr& spRecord,
            __in KAllocator& allocator,
            __in BinaryReader& reader,
            __in IDnsRecord& question
        );

    public:
        virtual PVOID ValuePtr() override { return static_cast<PVOID>(&_address); }
        virtual KString::SPtr DebugString() const override;

        ULONG Address() const { return _address; }

        bool Serialize(
            __in BinaryWriter& writer,
            __in ULONG ttl
        ) const;

    private:
        ULONG _address;
    };
}
