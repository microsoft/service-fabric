// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class MessageId
    {
    public:
        struct Hasher
        {
            size_t operator() (MessageId const & key) const 
            { 
                return key.GetHash();
            }
            bool operator() (MessageId const & left, MessageId const & right) const 
            { 
                return left == right;
            }
        };

    public:
        MessageId();
        MessageId(MessageId const & rhs);
        MessageId(Common::Guid const & guid, DWORD index);

        __declspec(property(get=get_Guid)) Common::Guid const & Guid;
        __declspec(property(get=get_Index)) DWORD Index;

        Common::Guid const & get_Guid() const;
        DWORD get_Index() const;

        bool operator == (MessageId const & rhs) const;
        bool operator != (MessageId const rhs) const;
        bool operator < (MessageId const & rhs) const;

        bool IsEmpty() const;

        size_t GetHash() const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

        FABRIC_PRIMITIVE_FIELDS_02(guid_, index_);

    private:
        Common::Guid guid_;
        uint32 index_;

        static Common::Guid const localProcessGuid_;
        static Common::atomic_long localProcessCounter_;
    };
}
