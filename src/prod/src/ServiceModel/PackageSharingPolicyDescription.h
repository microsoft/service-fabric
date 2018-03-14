// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <PackageSharingPolicy> element.
    struct DigestedCodePackageDescription;
    struct ServicePackagePoliciesDescription;
	struct ServiceManifestImportDescription;
    struct PackageSharingPolicyDescription
    {
    public:
        PackageSharingPolicyDescription(); 
        PackageSharingPolicyDescription(PackageSharingPolicyDescription const & other);
        PackageSharingPolicyDescription(PackageSharingPolicyDescription && other);

        PackageSharingPolicyDescription const & operator = (PackageSharingPolicyDescription const & other);
        PackageSharingPolicyDescription const & operator = (PackageSharingPolicyDescription && other);

        bool operator == (PackageSharingPolicyDescription const & other) const;
        bool operator != (PackageSharingPolicyDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring PackageRef;
		PackageSharingPolicyTypeScope::Enum Scope;

    private:
        friend struct DigestedCodePackageDescription;
        friend struct ServicePackagePoliciesDescription;
		friend struct ServiceManifestImportDescription;
    
        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
