//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ServicePackageContainerPolicyDescription> element.
    struct ServicePackageContainerPolicyDescription
    {
    public:
        ServicePackageContainerPolicyDescription(); 

        ServicePackageContainerPolicyDescription(ServicePackageContainerPolicyDescription const & other) = default;
        ServicePackageContainerPolicyDescription(ServicePackageContainerPolicyDescription && other) = default;

        ServicePackageContainerPolicyDescription & operator = (ServicePackageContainerPolicyDescription const & other) = default;
        ServicePackageContainerPolicyDescription & operator = (ServicePackageContainerPolicyDescription && other) = default;

        bool operator == (ServicePackageContainerPolicyDescription const & other) const;
        bool operator != (ServicePackageContainerPolicyDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::vector<PortBindingDescription> PortBindings;
        std::wstring Hostname;
        std::wstring Isolation;

    private:
        friend struct DigestedCodePackageDescription;
        friend struct ServicePackagePoliciesDescription;
        friend struct ServiceManifestImportDescription;
        friend struct ServicePackageDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
