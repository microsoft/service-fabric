// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ServiceManifest>/<DataPackage>
    struct DigestedDataPackageDescription;
    struct ServiceManifestDescription;
    struct DataPackageDescription : public Serialization::FabricSerializable
    {
    public:
        DataPackageDescription();
        DataPackageDescription(DataPackageDescription const & other);
        DataPackageDescription(DataPackageDescription && other);

        DataPackageDescription const & operator = (DataPackageDescription const & other);
        DataPackageDescription const & operator = (DataPackageDescription && other);

        bool operator == (DataPackageDescription const & other) const;
        bool operator != (DataPackageDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_DATA_PACKAGE_DESCRIPTION const & publicDescription);
        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __in std::wstring const& serviceManifestName, 
            __in std::wstring const& serviceManifestVersion, 
            __out FABRIC_DATA_PACKAGE_DESCRIPTION & publicDescription) const;

        void clear();

        FABRIC_FIELDS_02(Name, Version);

    public:
        std::wstring Name;
        std::wstring Version;

    private:
        friend struct DigestedDataPackageDescription;
        friend struct ServiceManifestDescription;
		friend struct ETWDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &, bool isDiagnostic = false);
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::DataPackageDescription);
