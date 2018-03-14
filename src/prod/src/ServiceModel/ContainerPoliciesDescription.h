// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ContainerPoliciesDescription> element.
    struct ContainerPoliciesDescription
    {
    public:
        ContainerPoliciesDescription(); 

        ContainerPoliciesDescription(ContainerPoliciesDescription const & other) = default;
        ContainerPoliciesDescription(ContainerPoliciesDescription && other) = default;

        ContainerPoliciesDescription & operator = (ContainerPoliciesDescription const & other) = default;
        ContainerPoliciesDescription & operator = (ContainerPoliciesDescription && other) = default;

        bool operator == (ContainerPoliciesDescription const & other) const;
        bool operator != (ContainerPoliciesDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        RepositoryCredentialsDescription RepositoryCredentials;
        ContainerHealthConfigDescription HealthConfig;
        LogConfigDescription LogConfig;
        NetworkConfigDescription NetworkConfig;
        std::vector<PortBindingDescription> PortBindings;
        std::vector<ContainerVolumeDescription> Volumes;
        std::vector<ContainerCertificateDescription> CertificateRef;
        std::vector<SecurityOptionsDescription> SecurityOptions;
        ImageOverridesDescription ImageOverrides;

        std::wstring CodePackageRef;
        ContainerIsolationMode::Enum Isolation;
        std::wstring Hostname;
        bool RunInteractive;
        LONG ContainersRetentionCount;

    private:
        friend struct DigestedCodePackageDescription;
        friend struct ServicePackagePoliciesDescription;
        friend struct ServiceManifestImportDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
