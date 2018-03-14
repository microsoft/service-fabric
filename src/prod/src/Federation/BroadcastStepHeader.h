// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class BroadcastStepHeader : public Transport::MessageHeader<Transport::MessageHeaderId::BroadcastStep>, public Serialization::FabricSerializable
    {
    public:
        BroadcastStepHeader()
        {
        }

        BroadcastStepHeader(int count)
            : count_(count)
        {
        }

        __declspec(property(get=get_Count)) int Count;

        int get_Count() const { return this->count_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const 
        { 
            w << "[Count: " << this->count_ << "]";
        }

        FABRIC_FIELDS_01(count_);

    private:
        int count_;
    };
}
