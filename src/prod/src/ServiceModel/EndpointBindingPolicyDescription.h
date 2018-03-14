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
    struct EndpointBindingPolicyDescription
    {
    public:
        EndpointBindingPolicyDescription(); 
        EndpointBindingPolicyDescription(EndpointBindingPolicyDescription const & other);
        EndpointBindingPolicyDescription(EndpointBindingPolicyDescription && other);

        EndpointBindingPolicyDescription const & operator = (EndpointBindingPolicyDescription const & other);
        EndpointBindingPolicyDescription const & operator = (EndpointBindingPolicyDescription && other);

        bool operator == (EndpointBindingPolicyDescription const & other) const;
        bool operator != (EndpointBindingPolicyDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring EndpointRef;
        std::wstring CertificateRef;

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
