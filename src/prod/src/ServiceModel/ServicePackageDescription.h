// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

#include "ServicePackageResourceGovernanceDescription.h"
#include "ServicePackageContainerPolicyDescription.h"

namespace ServiceModel
{
    // <ServicePackage> element.
    struct Parser;
    struct Serializer;
    struct ServicePackageDescription
    {
    public:
        ServicePackageDescription(); 
        ServicePackageDescription(ServicePackageDescription const & other);
        ServicePackageDescription(ServicePackageDescription && other);

        ServicePackageDescription const & operator = (ServicePackageDescription const & other);
        ServicePackageDescription const & operator = (ServicePackageDescription && other);

        bool operator == (ServicePackageDescription const & other) const;
        bool operator != (ServicePackageDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        Common::ErrorCode FromXml(std::wstring const & fileName);

        std::map<std::wstring, std::wstring> GetEndpointNetworkMap(wstring const & networkTypeStr) const;

        std::vector<std::wstring> GetNetworks(wstring const & networkTypeStr) const;

    public:
        std::wstring PackageName;
        std::wstring ManifestName;
        std::wstring ManifestVersion;
        std::wstring ManifestChecksum;
        std::wstring ContentChecksum;
        RolloutVersion RolloutVersionValue;
        
        DigestedServiceTypesDescription DigestedServiceTypes;
        std::vector<DigestedCodePackageDescription> DigestedCodePackages;
        std::vector<DigestedConfigPackageDescription> DigestedConfigPackages;
        std::vector<DigestedDataPackageDescription> DigestedDataPackages;        
        DigestedResourcesDescription DigestedResources;      
        ServiceDiagnosticsDescription Diagnostics;
        ServicePackageResourceGovernanceDescription ResourceGovernanceDescription;
        ServicePackageContainerPolicyDescription ContainerPolicyDescription;
        ServiceFabricRuntimeAccessDescription SFRuntimeAccessDescription;
        NetworkPoliciesDescription NetworkPolicies;

    private:
        friend struct Parser;
        friend struct Serializer;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ParseServiceFabricRuntimeAccessDescription(Common::XmlReaderUPtr const & xmlReader);
        void ParseResourceGovernanceDescription(Common::XmlReaderUPtr const & xmlReader);
        void ParseContainerPolicyDescription(Common::XmlReaderUPtr const & xmlReader);
        void ParseDigestedServiceTypes(Common::XmlReaderUPtr const & xmlReader);
        void ParseDigestedCodePackages(Common::XmlReaderUPtr const & xmlReader);
        void ParseDigestedConfigPackages(Common::XmlReaderUPtr const & xmlReader);
        void ParseDigestedDataPackages(Common::XmlReaderUPtr const & xmlReader);
        void ParseDigestedResources(Common::XmlReaderUPtr const & xmlReader);
        void ParseNetworkPolicies(Common::XmlReaderUPtr const & xmlReader);
        void ParseDiagnostics(Common::XmlReaderUPtr const & xmlReader);
        bool MatchNetworkTypeRef(std::wstring const & networkTypeStr, std::wstring const & networkRef) const;

        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
