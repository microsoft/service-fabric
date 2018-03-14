// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceTypeUpdateReplyMessageBody : public Serialization::FabricSerializable
    {
    public:
        ServiceTypeUpdateReplyMessageBody()
        {
        }

        ServiceTypeUpdateReplyMessageBody(uint64 sequenceNumber)
            : sequenceNumber_(sequenceNumber)
        {
        }

        __declspec (property(get=get_SequenceNumber)) uint64 SequenceNumber;
        uint64 get_SequenceNumber() const { return sequenceNumber_; }

        FABRIC_FIELDS_01(sequenceNumber_);

    private:

        uint64 sequenceNumber_;
    };
}
