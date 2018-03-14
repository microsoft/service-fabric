// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class NodeDeactivateRequestMessageBody : public Serialization::FabricSerializable
    {
    public:
        NodeDeactivateRequestMessageBody()
        {
        }

        NodeDeactivateRequestMessageBody(int64 sequenceNumber, bool isFromFmm)
        : sequenceNumber_(sequenceNumber),
          isFromFmm_(isFromFmm)
        {
        }

        __declspec(property(get=get_SequenceNumber)) int64 SequenceNumber;
        int64 get_SequenceNumber() const { return sequenceNumber_; }

        __declspec(property(get=get_IsFromFmm)) bool IsFromFmm;
        bool get_IsFromFmm() const { return isFromFmm_; }

        __declspec(property(get = get_Sender)) Reliability::FailoverManagerId Sender;
        Reliability::FailoverManagerId get_Sender() const { return Reliability::FailoverManagerId(isFromFmm_); }

        void WriteToEtw(uint16 contextSequenceId) const;
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(sequenceNumber_, isFromFmm_);

    private:
        int64 sequenceNumber_;
        bool isFromFmm_;
    };
}

