// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class FabricActivityHeader
        : public MessageHeader<MessageHeaderId::FabricActivity>
        , public Serialization::FabricSerializable
    {
    public:
        FabricActivityHeader();

        explicit FabricActivityHeader(Common::Guid const &);

        explicit FabricActivityHeader(Common::ActivityId const &);

        FabricActivityHeader(FabricActivityHeader const & other);

        static FabricActivityHeader FromMessage(Message &);

        __declspec(property(get=get_Guid)) Common::Guid const & Guid;
        Common::Guid const & get_Guid() const { return activityId_.Guid; }

        __declspec(property(get=get_ActivityId)) Common::ActivityId const & ActivityId;
        Common::ActivityId const & get_ActivityId() const { return activityId_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const& f) const;

        std::wstring ToString() const;

        FABRIC_FIELDS_01(activityId_);

    private:
        Common::ActivityId activityId_;
    };
}

