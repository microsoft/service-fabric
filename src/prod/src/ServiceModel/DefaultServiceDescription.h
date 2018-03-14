// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct ApplicationInstanceDescription;
    struct DefaultServiceDescription
    {
    public:
        DefaultServiceDescription();
        DefaultServiceDescription(DefaultServiceDescription const & other);
        DefaultServiceDescription(DefaultServiceDescription && other);

        DefaultServiceDescription const & operator = (DefaultServiceDescription const & other);
        DefaultServiceDescription const & operator = (DefaultServiceDescription && other);

        bool operator == (DefaultServiceDescription const & other) const;
        bool operator != (DefaultServiceDescription const & other) const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring Name;
        ServiceModel::ServicePackageActivationMode::Enum PackageActivationMode;
		std::wstring ServiceDnsName;
        ApplicationServiceDescription DefaultService;

    private:
        friend struct ApplicationInstanceDescription;
		friend struct ApplicationManifestDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);

        void ReadPackageActivationModeFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WritePackageActivationModeToXml(Common::XmlWriterUPtr const &);
    };
}
