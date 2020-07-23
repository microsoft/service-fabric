// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <DigestedConfigPackage> element.
    struct ServicePackageDescription;
    struct DigestedConfigPackageDescription
    {
    public:
        DigestedConfigPackageDescription();
        DigestedConfigPackageDescription(DigestedConfigPackageDescription const & other);
        DigestedConfigPackageDescription(DigestedConfigPackageDescription && other);

        DigestedConfigPackageDescription const & operator = (DigestedConfigPackageDescription const & other);
        DigestedConfigPackageDescription const & operator = (DigestedConfigPackageDescription && other);

        bool operator == (DigestedConfigPackageDescription const & other) const;
        bool operator != (DigestedConfigPackageDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        __declspec(property(get=get_Name)) std::wstring const & Name;
        inline std::wstring const & get_Name() const { return ConfigPackage.Name; }

    public:
        std::wstring ContentChecksum;
        RolloutVersion RolloutVersionValue;
        ConfigPackageDescription ConfigPackage;
        Common::ConfigOverrideDescription ConfigOverrides;
        DebugParametersDescription DebugParameters;
        bool IsShared;

    private:
        friend struct ServicePackageDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ParseDebugParameters(Common::XmlReaderUPtr const &xmlReader);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
