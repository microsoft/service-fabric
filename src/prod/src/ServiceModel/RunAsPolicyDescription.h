// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <RunAsPolicy> element.
    struct DigestedCodePackageDescription;
    struct ServicePackagePoliciesDescription;
	struct ServiceManifestImportDescription;
    struct RunAsPolicyDescription
    {
    public:
        RunAsPolicyDescription(); 
        RunAsPolicyDescription(RunAsPolicyDescription const & other);
        RunAsPolicyDescription(RunAsPolicyDescription && other);

        RunAsPolicyDescription const & operator = (RunAsPolicyDescription const & other);
        RunAsPolicyDescription const & operator = (RunAsPolicyDescription && other);

        bool operator == (RunAsPolicyDescription const & other) const;
        bool operator != (RunAsPolicyDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_RUNAS_POLICY_DESCRIPTION & publicDescription) const;

        void clear();

    public:
        std::wstring CodePackageRef;
        std::wstring UserRef;
        RunAsPolicyTypeEntryPointType::Enum EntryPointType; 

    private:
        friend struct DigestedCodePackageDescription;
        friend struct ServicePackagePoliciesDescription;
		friend struct ServiceManifestImportDescription;
    
        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
