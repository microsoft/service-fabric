// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NamingPropertyHeader
        : public Transport::MessageHeader<Transport::MessageHeaderId::NamingProperty>
        , public Serialization::FabricSerializable
    {
    public:
        NamingPropertyHeader() : name_(), propertyName_()
        {
        }

        explicit NamingPropertyHeader(Common::NamingUri const & name, std::wstring const & propertyName) : name_(name), propertyName_(propertyName)
        { 
        }

        __declspec (property(get=get_Name)) Common::NamingUri const & Name;
        Common::NamingUri const & get_Name() { return name_; }

        __declspec (property(get=get_PropertyName)) std::wstring const & PropertyName;
        std::wstring const & get_PropertyName() { return propertyName_; }

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write(
            "NamingPropertyHeader[{0}:{1}]",
            name_.ToString(),
            propertyName_);
        }

        FABRIC_FIELDS_02(name_, propertyName_);

    private:
        Common::NamingUri name_;
        std::wstring propertyName_;
    };
}
