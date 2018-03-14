// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    class StringBody : public Serialization::FabricSerializable
    {
    public:
        StringBody()
        {
        }

        StringBody(std::wstring const& string)
            : string_(string)
        {
        }

        __declspec(property(get=get_TestSessionData)) std::wstring const & String;
        std::wstring const & get_TestSessionData() const { return string_; }

        FABRIC_FIELDS_01(string_);

    private:
        std::wstring string_;
    };
};
