// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class GlobalTimeExchangeHeader : public Transport::MessageHeader<Transport::MessageHeaderId::GlobalTimeExchange>, public Serialization::FabricSerializable
    {
    public:
        GlobalTimeExchangeHeader()
        {
        }

        GlobalTimeExchangeHeader(int64 epoch, Common::StopwatchTime sendTime, Common::TimeSpan senderLowerLimit, Common::TimeSpan receiverUpperLimit)
            : epoch_(epoch), sendTime_(sendTime), senderLowerLimit_(senderLowerLimit), receiverUpperLimit_(receiverUpperLimit)
        {
        }

        __declspec(property(get = get_Epoch)) int64 Epoch;
        int64 get_Epoch() const { return epoch_; }

        __declspec(property(get = get_SendTime)) Common::StopwatchTime SendTime;
        Common::StopwatchTime get_SendTime() const { return sendTime_; }

        __declspec(property(get = get_SenderLowerLimit)) Common::TimeSpan SenderLowerLimit;
        Common::TimeSpan get_SenderLowerLimit() const { return senderLowerLimit_; }

        __declspec(property(get = get_ReceiverUpperLimit)) Common::TimeSpan ReceiverUpperLimit;
        Common::TimeSpan get_ReceiverUpperLimit() const { return receiverUpperLimit_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write("{0}:{1},{2},{3}", epoch_, sendTime_.Ticks, senderLowerLimit_.Ticks, receiverUpperLimit_.Ticks);
        }

        FABRIC_FIELDS_04(epoch_, sendTime_, senderLowerLimit_, receiverUpperLimit_);

    private:
        int64 epoch_;
        Common::StopwatchTime sendTime_;
        Common::TimeSpan senderLowerLimit_;
        Common::TimeSpan receiverUpperLimit_;
    };
}
