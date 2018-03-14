// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "DnsAnswerRecordBase.h"

namespace DNS
{
    using ::_delete;

    class DnsAnswerRecordTxt :
        public DnsAnswerRecordBase
    {
        K_FORCE_SHARED(DnsAnswerRecordTxt);

    public:
        static void Create(
            __out DnsAnswerRecordTxt::SPtr& spRecord,
            __in KAllocator& allocator,
            __in IDnsRecord& question,
            __in KString& strText
        );

    private:
        DnsAnswerRecordTxt(
            __in IDnsRecord& question,
            __in KString& strText
        );

    public:
        virtual PVOID ValuePtr() override { return static_cast<PVOID>(_spText.RawPtr()); }
        virtual KString::SPtr DebugString() const override { return _spText; }

        KString& Text() const { return *_spText; }

        bool Serialize(
            __in BinaryWriter& writer,
            __in ULONG ttl
        ) const;

    private:
        KString::SPtr _spText;
    };
}
