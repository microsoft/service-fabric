// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <DigestedDataPackage> element.
    struct ServicePackageDescription;
    struct DigestedDataPackageDescription
    {
    public:
        DigestedDataPackageDescription();
        DigestedDataPackageDescription(DigestedDataPackageDescription const & other);
        DigestedDataPackageDescription(DigestedDataPackageDescription && other);

        DigestedDataPackageDescription const & operator = (DigestedDataPackageDescription const & other);
        DigestedDataPackageDescription const & operator = (DigestedDataPackageDescription && other);

        bool operator == (DigestedDataPackageDescription const & other) const;
        bool operator != (DigestedDataPackageDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        __declspec(property(get=get_Name)) std::wstring const & Name;
        inline std::wstring const & get_Name() const { return DataPackage.Name; }

    public:
        std::wstring ContentChecksum;
        RolloutVersion RolloutVersionValue;
        DataPackageDescription DataPackage;
        DebugParametersDescription DebugParameters;
        bool IsShared;

    private:
        friend struct ServicePackageDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ParseDebugParameters(Common::XmlReaderUPtr const &xmlReader);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
