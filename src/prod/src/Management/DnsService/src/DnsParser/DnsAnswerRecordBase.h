// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "BinaryReader.h"
#include "BinaryWriter.h"

namespace DNS
{
    using ::_delete;

    class DnsAnswerRecordBase :
        public KShared<DnsAnswerRecordBase>,
        public IDnsRecord
    {
        K_FORCE_SHARED_WITH_INHERITANCE(DnsAnswerRecordBase);
        K_SHARED_INTERFACE_IMP(IDnsRecord);

    protected:
        DnsAnswerRecordBase(
            __in USHORT type,
            __in IDnsRecord& question
        );

    public:
        static bool DeserializeHeader(
            __out USHORT& type,
            __out USHORT& rdLength,
            __in BinaryReader& reader
        );

    public:
        // IDnsRecord Impl.
        virtual USHORT Type() const override { return _type; }
        virtual KString& Name() const override { return _question.Name(); }
        virtual DnsTextEncodingType Encoding() const override { return _question.Encoding(); }

    protected:
        bool Serialize(
            __in BinaryWriter& writer,
            __in ULONG ttl,
            __in USHORT nClass = 1
        ) const;

    protected:
        const USHORT _type;
        IDnsRecord& _question;
    };

    class DnsAnswerUnsupportedRecord :
        public DnsAnswerRecordBase
    {
        K_FORCE_SHARED(DnsAnswerUnsupportedRecord);

    public:
        static void Create(
            __out DnsAnswerUnsupportedRecord::SPtr& spRecord,
            __in KAllocator& allocator,
            __in IDnsRecord& question,
            __in USHORT type
        );

    private:
        DnsAnswerUnsupportedRecord(
            __in IDnsRecord& question,
            __in USHORT type
        );

    public:
        virtual PVOID ValuePtr() { return nullptr; }
        virtual KString::SPtr DebugString() const { return nullptr; }
    };
}
