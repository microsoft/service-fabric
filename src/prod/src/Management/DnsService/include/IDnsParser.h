// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    enum DnsRecordType : USHORT
    {
        DnsRecordTypeA = 1,
        DnsRecordTypeTxt = 16,
        DnsRecordTypeSrv = 33,
        DnsRecordTypeOpt = 41,
        DnsRecordTypeAll = 255
    };

    enum DnsTextEncodingType : USHORT
    {
        DnsTextEncodingTypeInvalid = 1,
        DnsTextEncodingTypeUTF8,
        DnsTextEncodingTypeIDN
    };

    interface IDnsRecord
    {
        K_SHARED_INTERFACE(IDnsRecord);

    public:
        virtual USHORT Type() const = 0;
        virtual KString& Name() const = 0;
        virtual DnsTextEncodingType Encoding() const = 0;
        virtual PVOID ValuePtr() = 0;
        virtual KString::SPtr DebugString() const = 0;
    };

    interface IDnsMessage
    {
        K_SHARED_INTERFACE(IDnsMessage);

    public:
        virtual USHORT Id() const = 0;
        virtual USHORT Flags() const = 0;
        virtual void SetFlags(USHORT flags) = 0;
        virtual KArray<IDnsRecord::SPtr>& Questions() = 0;
        virtual KArray<IDnsRecord::SPtr>& Answers() = 0;
        virtual KString::SPtr ToString() const = 0;
    };

    interface IDnsParser
    {
        K_SHARED_INTERFACE(IDnsParser);

    public:
        virtual bool Serialize(
            __out KBuffer& buffer,
            __out ULONG& numberOfBytesWritten,
            __in IDnsMessage& message,
            __in ULONG ttl,
            __in DnsTextEncodingType encodingType = DnsTextEncodingTypeUTF8
        ) const = 0;

        virtual bool Deserialize(
            __out IDnsMessage::SPtr& message,
            __in KBuffer& buffer,
            __in ULONG numberOfBytes,
            __in bool fIncludeUnsupportedRecords = false
        ) const = 0;

        virtual bool GetHeader(
            __out USHORT& id,
            __out USHORT& flags,
            __out USHORT& questionCount,
            __out USHORT& answerCount,
            __out USHORT& nsCount,
            __out USHORT& adCount,
            __in KBuffer& buffer,
            __in ULONG numberOfBytes
        ) const = 0;

        virtual bool SetHeader(
            __in KBuffer& buffer,
            __in USHORT id,
            __in USHORT flags,
            __in USHORT qdCount,
            __in USHORT anCount,
            __in USHORT nsCount,
            __in USHORT adCount
        ) const = 0;

        virtual IDnsMessage::SPtr CreateQuestionMessage(
            __in USHORT id,
            __in KString& strQuestion,
            __in DnsRecordType type,
            __in DnsTextEncodingType encodingType = DnsTextEncodingTypeUTF8
        ) const = 0;

        virtual IDnsRecord::SPtr CreateRecordA(
            __in IDnsRecord& question,
            __in ULONG address
        ) const = 0;

        virtual IDnsRecord::SPtr CreateRecordTxt(
            __in IDnsRecord& question,
            __in KString& strText
        ) const = 0;

        virtual IDnsRecord::SPtr CreateRecordSrv(
            __in IDnsRecord& question,
            __in KString& strHost,
            __in USHORT port
        ) const = 0;
    };

    void CreateDnsParser(
        __out IDnsParser::SPtr& spDnsParser,
        __in KAllocator& allocator
    );

    namespace DnsFlags
    {
        enum DNS_FLAG : USHORT
        {
            RECURSION_AVAILABLE = (1 << 7),
            RECURSION_DESIRED = (1 << 8),
            TRUNCATION = (1 << 9),
            AUTHORITY = (1 << 10),
            RESPONSE = (1 << 15),
        };

        enum DNS_RESPONSE_CODE : USHORT
        {
            RC_NOERROR = 0,
            RC_FORMERR = 1,
            RC_SERVFAIL = 2,
            RC_NXDOMAIN = 3
        };

        void SetFlag(__out USHORT& flags, DNS_FLAG flag);
        void UnsetFlag(__out USHORT& flags, DNS_FLAG flag);
        bool IsFlagSet(__in USHORT flags, DNS_FLAG flag);
        void SetResponseCode(__out USHORT& flags, DNS_RESPONSE_CODE code);
        USHORT GetResponseCode(__in USHORT flags);
        bool IsSuccessResponseCode(__in USHORT rc);
    };
}
