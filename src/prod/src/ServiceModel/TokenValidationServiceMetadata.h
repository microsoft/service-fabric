// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class TokenValidationServiceMetadata
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:

        TokenValidationServiceMetadata() {}

        TokenValidationServiceMetadata(
            std::wstring const&metadata,
            std::wstring const& name,
            std::wstring const& dnsName)
            : metadata_(metadata)
            , name_(name)
            , dnsName_(dnsName)
        {
        }

        __declspec(property(get=get_Metadata)) std::wstring const & Metadata;
        __declspec(property(get=get_Name)) std::wstring const & ServiceName;
        __declspec(property(get=get_DnsName)) std::wstring const & ServiceDnsName;

        std::wstring const & get_Metadata() const { return metadata_; }
        std::wstring const & get_DnsName() const { return dnsName_; }
        std::wstring const & get_Name() const { return name_; }

        FABRIC_FIELDS_03(metadata_, name_, dnsName_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Metadata, metadata_)
            SERIALIZABLE_PROPERTY(Constants::ServiceName, name_)
            SERIALIZABLE_PROPERTY(Constants::ServiceDnsName, dnsName_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:

        std::wstring metadata_;
        std::wstring name_;
        std::wstring dnsName_;
    };
}
