// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ServiceManifestRef> element.
    struct ApplicationManifestDescription;
	struct ServiceManifestImportDescription;
    struct ServiceManifestReference : public Serialization::FabricSerializable
    {
        ServiceManifestReference(); 
        ServiceManifestReference(ServiceManifestReference const & other);
        ServiceManifestReference(ServiceManifestReference && other);

        ServiceManifestReference const & operator = (ServiceManifestReference const & other);
        ServiceManifestReference const & operator = (ServiceManifestReference && other);

        bool operator == (ServiceManifestReference const & other) const;
        bool operator != (ServiceManifestReference const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(Name, Version);

    public:
        std::wstring Name;
        std::wstring Version;

    private:
        friend struct ApplicationManifestDescription;
		friend struct ServiceManifestImportDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
        void clear();
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ServiceManifestReference)
