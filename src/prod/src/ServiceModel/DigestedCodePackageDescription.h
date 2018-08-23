// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <DigestedCodePackage> element.
    struct ServicePackageDescription;
    struct DigestedCodePackageDescription
    {
    public:
        DigestedCodePackageDescription();
        DigestedCodePackageDescription(DigestedCodePackageDescription const & other);
        DigestedCodePackageDescription(DigestedCodePackageDescription && other);

        DigestedCodePackageDescription & operator = (DigestedCodePackageDescription const & other) = default;
        DigestedCodePackageDescription & operator = (DigestedCodePackageDescription && other) = default;

        bool operator == (DigestedCodePackageDescription const & other) const;
        bool operator != (DigestedCodePackageDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

         __declspec(property(get=get_Name)) std::wstring const & Name;
        inline std::wstring const & get_Name() const { return CodePackage.Name; }

    public:
        std::wstring ContentChecksum;
        RolloutVersion RolloutVersionValue;
        CodePackageDescription CodePackage;
        RunAsPolicyDescription RunAsPolicy;
        RunAsPolicyDescription SetupRunAsPolicy;
        DebugParametersDescription DebugParameters;
        ContainerPoliciesDescription ContainerPolicies;
        ResourceGovernancePolicyDescription ResourceGovernancePolicy;
        ConfigPackagePoliciesDescription ConfigPackagePolicy;

        bool IsShared;

    private:
        friend struct ServicePackageDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ParseRunasPolicies(Common::XmlReaderUPtr const & xmlReader);
        void ParseDebugParameters(Common::XmlReaderUPtr const & xmlReader);
        void ParseContainerPolicies(Common::XmlReaderUPtr const & xmlReader);
        void ParseResourceGovernancePolicy(Common::XmlReaderUPtr const & xmlReader);
        void ParseConfigPackagePolicy(Common::XmlReaderUPtr const & xmlReader);

        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
