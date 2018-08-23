// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class DnsMessage :
        public KShared<DnsMessage>,
        public IDnsMessage
    {
        K_FORCE_SHARED(DnsMessage);
        K_SHARED_INTERFACE_IMP(IDnsMessage);

    public:
        static void Create(
            __out DnsMessage::SPtr& spMessage,
            __in KAllocator& allocator,
            __in USHORT id,
            __in USHORT flags
        );

    private:
        DnsMessage(
            __in USHORT id,
            __in USHORT flags
        );

    public:
        // IDnsMessage Impl.
        virtual USHORT Id() const override { return _id; }
        virtual USHORT Flags() const override { return _flags; }
        virtual void SetFlags(USHORT flags) override { _flags = flags; }
        virtual KArray<IDnsRecord::SPtr>& Questions() override { return _arrQuestions; }
        virtual KArray<IDnsRecord::SPtr>& Answers() override { return _arrAnswers; }
        virtual KString::SPtr ToString() const override;

    private:
        USHORT _id;
        USHORT _flags;
        KDateTime _time;
        KArray<IDnsRecord::SPtr> _arrQuestions;
        KArray<IDnsRecord::SPtr> _arrAnswers;
    };
}
