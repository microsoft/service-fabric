// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ContainerCertificateDescription> element.
    struct ContainerCertificateDescription : 
        public Serialization::FabricSerializable
    {
    public:
        ContainerCertificateDescription();

        ContainerCertificateDescription(ContainerCertificateDescription const & other) = default;
        ContainerCertificateDescription(ContainerCertificateDescription && other) = default;

        ContainerCertificateDescription & operator = (ContainerCertificateDescription const & other) = default;
        ContainerCertificateDescription & operator = (ContainerCertificateDescription && other) = default;

        bool operator == (ContainerCertificateDescription const & other) const;
        bool operator != (ContainerCertificateDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;


        void clear();

        FABRIC_FIELDS_08(X509FindValue, X509StoreName, Name, DataPackageRef, DataPackageVersion, RelativePath, Password, IsPasswordEncrypted);

    public:
        std::wstring X509StoreName;
        std::wstring X509FindValue;
        std::wstring DataPackageRef;
        std::wstring DataPackageVersion;
        std::wstring RelativePath;
        std::wstring Password;
        bool IsPasswordEncrypted;
        std::wstring Name;

    private:
        friend struct DigestedCodePackageDescription;
        friend struct ServicePackagePoliciesDescription;
        friend struct ContainerPoliciesDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
DEFINE_USER_ARRAY_UTILITY(ServiceModel::ContainerCertificateDescription);
