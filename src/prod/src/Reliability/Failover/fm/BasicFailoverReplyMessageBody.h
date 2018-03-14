// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class BasicFailoverReplyMessageBody : public Serialization::FabricSerializable
    {
    public:

        BasicFailoverReplyMessageBody()
        {}

        explicit BasicFailoverReplyMessageBody(Common::ErrorCodeValue::Enum errorCodeValue)
            : errorCodeValue_(errorCodeValue)
        {
        }

        __declspec (property(get=get_ErrorCodeValue)) Common::ErrorCodeValue::Enum ErrorCodeValue;
        Common::ErrorCodeValue::Enum get_ErrorCodeValue() const { return errorCodeValue_; }

        FABRIC_FIELDS_01(errorCodeValue_);

    private:

        Common::ErrorCodeValue::Enum errorCodeValue_;
    };
}
