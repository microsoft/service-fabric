// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class DnsParser :
        public KShared<DnsParser>,
        public IDnsParser
    {
        K_FORCE_SHARED(DnsParser);
        K_SHARED_INTERFACE_IMP(IDnsParser);

    public:
        static void Create(
            __out DnsParser::SPtr& spDnsParser,
            __in KAllocator& allocator
        );

    public:
        // IDnsParser Impl.
        virtual bool Serialize(
            __out KBuffer& buffer,
            __out ULONG& numberOfBytesWritten,
            __in IDnsMessage& message,
            __in ULONG ttl,
            __in DnsTextEncodingType encodingType
        ) const override;

        virtual bool Deserialize(
            __out IDnsMessage::SPtr& message,
            __in KBuffer& buffer,
            __in ULONG numberOfBytes,
            __in bool fIncludeUnsupportedRecords = false
        ) const override;

        virtual bool GetHeader(
            __out USHORT& id,
            __out USHORT& flags,
            __out USHORT& questionCount,
            __out USHORT& answerCount,
            __out USHORT& nsCount,
            __out USHORT& adCount,
            __in KBuffer& buffer,
            __in ULONG numberOfBytes
        ) const override;

        virtual bool SetHeader(
            __in KBuffer& buffer,
            __in USHORT id,
            __in USHORT flags,
            __in USHORT qdCount,
            __in USHORT anCount,
            __in USHORT nsCount,
            __in USHORT adCount
        ) const override;

        virtual IDnsMessage::SPtr CreateQuestionMessage(
            __in USHORT id,
            __in KString& strQuestion,
            __in DnsRecordType type,
            __in DnsTextEncodingType encodingType
        ) const override;

        virtual IDnsRecord::SPtr CreateRecordA(
            __in IDnsRecord& question,
            __in ULONG address
        ) const override;

        virtual IDnsRecord::SPtr CreateRecordTxt(
            __in IDnsRecord& question,
            __in KString& strText
        ) const override;

        virtual IDnsRecord::SPtr CreateRecordSrv(
            __in IDnsRecord& question,
            __in KString& strHost,
            __in USHORT port
        ) const override;
    };
}
