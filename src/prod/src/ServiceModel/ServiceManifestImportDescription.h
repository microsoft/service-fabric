// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>
    struct DigestedEnvironmentDescription;
    struct ApplicationManifestDescription;
    struct ServiceManifestImportDescription
    {
    public:
        ServiceManifestImportDescription();
        ServiceManifestImportDescription(ServiceManifestImportDescription const & other);
        ServiceManifestImportDescription(ServiceManifestImportDescription && other);

        ServiceManifestImportDescription const & operator = (ServiceManifestImportDescription const & other);
        ServiceManifestImportDescription const & operator = (ServiceManifestImportDescription && other);

        bool operator == (ServiceManifestImportDescription const & other) const;
        bool operator != (ServiceManifestImportDescription const & other) const;

        void clear();

    public:
        ServiceManifestReference ServiceManifestRef;
        std::vector<Common::ConfigOverrideDescription> ConfigOverrides;
        ResourceOverridesDescription ResourceOverrides;
        std::vector<RunAsPolicyDescription> RunAsPolicies;
        std::vector<SecurityAccessPolicyDescription> SecurityAccessPolicies;
        std::vector<PackageSharingPolicyDescription> PackageSharingPolicies;
        std::vector<EndpointBindingPolicyDescription> EndpointBindingPolicies;
        std::vector<ContainerPoliciesDescription> ContainerHostPolicies;
        ServicePackageContainerPolicyDescription ServicePackageContainerPolicy;
        std::vector<EnvironmentOverridesDescription> EnvironmentOverrides;
        ServicePackageResourceGovernanceDescription ResourceGovernanceDescription;
        ServiceFabricRuntimeAccessDescription SFRuntimeAccessDescription;
        std::vector<ConfigPackagePoliciesDescription> ConfigPackagePolicies;

    private:
        friend struct ApplicationManifestDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
