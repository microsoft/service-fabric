// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class LivenessQueryBody : public Serialization::FabricSerializable
    {
    public:
        LivenessQueryBody() 
        {
        }

        LivenessQueryBody(bool value) 
            : value_(value)
        {
        }

        __declspec (property(get = get_Value)) bool Result;
        bool get_Value() const { return this->value_; }

        FABRIC_FIELDS_01(value_);

    private:

        bool value_;
    };
}
