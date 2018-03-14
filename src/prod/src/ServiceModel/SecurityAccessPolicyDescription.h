// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <SecurityAccessPolicy> element.
    struct DigestedEndpointDescription;
    struct ServicePackagePoliciesDescription;
    struct ApplicationPoliciesDescription;
	struct ServiceManifestImportDescription;
	struct ApplicationManifestPoliciesDescription;
    struct SecurityAccessPolicyDescription
    {
    public:
        SecurityAccessPolicyDescription(); 
        SecurityAccessPolicyDescription(SecurityAccessPolicyDescription const & other);
        SecurityAccessPolicyDescription(SecurityAccessPolicyDescription && other);

        SecurityAccessPolicyDescription const & operator = (SecurityAccessPolicyDescription const & other);
        SecurityAccessPolicyDescription const & operator = (SecurityAccessPolicyDescription && other);

        bool operator == (SecurityAccessPolicyDescription const & other) const;
        bool operator != (SecurityAccessPolicyDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring ResourceRef;
        std::wstring PrincipalRef;
        GrantAccessType::Enum Rights;
		SecurityAccessPolicyTypeResourceType::Enum ResourceType;


    private:
        friend struct DigestedEndpointDescription;
        friend struct ServicePackagePoliciesDescription;
        friend struct ApplicationPoliciesDescription;
		friend struct ServiceManifestImportDescription;
		friend struct ApplicationManifestPoliciesDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
