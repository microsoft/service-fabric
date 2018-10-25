// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "DnsAnswerRecordBase.h"

namespace DNS
{
    using ::_delete;

    class DnsAnswerRecordOpt :
        public DnsAnswerRecordBase
    {
        K_FORCE_SHARED(DnsAnswerRecordOpt);

    public:
        static void Create(
            __out DnsAnswerRecordOpt::SPtr& spRecord,
            __in KAllocator& allocator,
            __in IDnsRecord& question,
            __in USHORT size
        );

    private:
        DnsAnswerRecordOpt(
            __in IDnsRecord& question,
            __in USHORT size
        );

    public:
        virtual PVOID ValuePtr() override { return static_cast<PVOID>(&_size); }
        virtual KString::SPtr DebugString() const override { return nullptr; }

        USHORT Size() const { return _size; }

        bool Serialize(
            __in BinaryWriter& writer,
            __in ULONG ttl = 0
        ) const;

    private:
        USHORT _size;
    };
}
