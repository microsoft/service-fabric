// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <Diagnostics>\<ETW> element in serivce manifest and service package
    struct ServiceDiagnosticsDescription;
    struct ETWDescription
    {
    public:
        ETWDescription();
        ETWDescription(ETWDescription const & other);
        ETWDescription(ETWDescription && other);

        ETWDescription const & operator = (ETWDescription const & other);
        ETWDescription const & operator = (ETWDescription && other);

        bool operator == (ETWDescription const & other) const;
        bool operator != (ETWDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
		bool isEmpty();
        void clear();

    public:
        std::vector<std::wstring> ProviderGuids;
        std::vector<DataPackageDescription> ManifestDataPackages;


    private:
        friend struct ServiceDiagnosticsDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
		Common::ErrorCode WriteProviderGuids(Common::XmlWriterUPtr const &);
		Common::ErrorCode WriteManifestDataPackages(Common::XmlWriterUPtr const &);
    };
}
