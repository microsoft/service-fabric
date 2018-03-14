// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class HighPriorityHeader : public MessageHeader<MessageHeaderId::HighPriority>, public Serialization::FabricSerializable
    {
    public:
        HighPriorityHeader() : highPriority_(true) { }
        HighPriorityHeader(bool highPriority) : highPriority_(highPriority) { }

        __declspec(property(get=get_HighPriority)) bool HighPriority;

        bool get_HighPriority() const { return highPriority_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w << highPriority_;
        }

        FABRIC_FIELDS_01(highPriority_);
    private:
        bool highPriority_;
    };
}
