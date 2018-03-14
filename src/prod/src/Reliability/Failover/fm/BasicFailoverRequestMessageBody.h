// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class BasicFailoverRequestMessageBody : public Serialization::FabricSerializable
    {
    public:

        BasicFailoverRequestMessageBody()
        {}

        explicit BasicFailoverRequestMessageBody(std::wstring const & value)
            : value_(value)
        {
        }

        __declspec (property(get=get_Value)) std::wstring const & Value;
        std::wstring const & get_Value() const { return value_; }

        FABRIC_FIELDS_01(value_);

    private:

        std::wstring value_;
    };
}
