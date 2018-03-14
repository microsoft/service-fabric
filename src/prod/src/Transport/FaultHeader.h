// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class FaultHeader : public MessageHeader<MessageHeaderId::Fault>, public Serialization::FabricSerializable
    {
    public:
        FaultHeader() : errorCodeValue_(), hasFaultBody_(false) { }
        FaultHeader(Common::ErrorCodeValue::Enum const & errorCodeValue, bool hasFaultBody = false) : errorCodeValue_(errorCodeValue), hasFaultBody_(hasFaultBody) { }
        FaultHeader(FaultHeader const & rhs) : errorCodeValue_(rhs.errorCodeValue_), hasFaultBody_(rhs.hasFaultBody_) { }

        __declspec(property(get=get_ErrorCodeValue)) Common::ErrorCodeValue::Enum const & ErrorCodeValue;
        __declspec(property(get=get_HasFaultBody)) bool HasFaultBody;

        Common::ErrorCodeValue::Enum const & get_ErrorCodeValue() const { return errorCodeValue_; }
        bool get_HasFaultBody() const { return hasFaultBody_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w << errorCodeValue_ << ", hasBody=" << hasFaultBody_;
        }

        FABRIC_FIELDS_02(errorCodeValue_, hasFaultBody_);

    private:
        Common::ErrorCodeValue::Enum errorCodeValue_;
        bool hasFaultBody_;
    };
}
