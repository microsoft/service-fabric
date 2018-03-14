// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <SecurityOptionsDescription> element.
    struct SecurityOptionsDescription : 
        public Serialization::FabricSerializable
    {
    public:
        SecurityOptionsDescription();

        SecurityOptionsDescription(SecurityOptionsDescription const & other) = default;
        SecurityOptionsDescription(SecurityOptionsDescription && other) = default;

        SecurityOptionsDescription & operator = (SecurityOptionsDescription const & other) = default;
        SecurityOptionsDescription & operator = (SecurityOptionsDescription && other) = default;

        bool operator == (SecurityOptionsDescription const & other) const;
        bool operator != (SecurityOptionsDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        FABRIC_FIELDS_01(Value);

    public:
        std::wstring Value;


    private:
        friend struct DigestedCodePackageDescription;
        friend struct ServicePackagePoliciesDescription;
        friend struct ContainerPoliciesDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
DEFINE_USER_ARRAY_UTILITY(ServiceModel::SecurityOptionsDescription);
