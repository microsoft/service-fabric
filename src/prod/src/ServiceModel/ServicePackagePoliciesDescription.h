// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ServicePackage><Policies> element.
    struct ServicePackagePoliciesDescription
    {
    public:
        ServicePackagePoliciesDescription(); 
        ServicePackagePoliciesDescription(ServicePackagePoliciesDescription const & other);
        ServicePackagePoliciesDescription(ServicePackagePoliciesDescription && other);

        ServicePackagePoliciesDescription const & operator = (ServicePackagePoliciesDescription const & other);
        ServicePackagePoliciesDescription const & operator = (ServicePackagePoliciesDescription && other);

        bool operator == (ServicePackagePoliciesDescription const & other) const;
        bool operator != (ServicePackagePoliciesDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    public:
        std::vector<RunAsPolicyDescription> RunAsPolicies;
        std::vector<SecurityAccessPolicyDescription> SecurityAccessPolicies;
        std::vector<PackageSharingPolicyDescription> PackageSharingPolicies;
        std::vector<EndpointBindingPolicyDescription> EndPointBindingPolicies;
        std::vector<ResourceGovernancePolicyDescription> ResourceGovernancePolicies;
        ServicePackageResourceGovernanceDescription ResourceGovernanceDescription;

    private:
        void ReadFromXml(Common::XmlReaderUPtr const &);
    };
}
