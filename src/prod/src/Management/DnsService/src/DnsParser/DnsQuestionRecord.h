// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "DnsText.h"

namespace DNS
{
    using ::_delete;

    class DnsQuestionRecord :
        public KShared<DnsQuestionRecord>,
        public IDnsRecord
    {
        K_FORCE_SHARED(DnsQuestionRecord);
        K_SHARED_INTERFACE_IMP(IDnsRecord);

    public:
        static void Create(
            __out DnsQuestionRecord::SPtr& spRecord,
            __in KAllocator& allocator,
            __in DnsRecordType type,
            __in KString& strName,
            __in DnsTextEncodingType encodingType
        );

    private:
        DnsQuestionRecord(
            __in DnsRecordType type,
            __in KString& strName,
            __in DnsTextEncodingType encodingType
        );

    public:
        // IDnsRecord Impl.
        virtual USHORT Type() const override { return _type; }
        virtual KString& Name() const override { return *_spName; }
        virtual DnsTextEncodingType Encoding() const override { return _encodingType; }
        virtual PVOID ValuePtr() override { return static_cast<PVOID>(_spName.RawPtr()); }
        virtual KString::SPtr DebugString() const override { return _spName; }

    public:
        static bool Deserialize(
            __out DnsQuestionRecord::SPtr& spRecord,
            __in KAllocator& allocator,
            __in BinaryReader& reader
        );

        bool Serialize(
            __in BinaryWriter& writer
        ) const;

    private:
        DnsRecordType _type;
        KString::SPtr _spName;
        DnsTextEncodingType _encodingType;
    };
}
