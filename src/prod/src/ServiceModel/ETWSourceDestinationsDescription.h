// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>/<ETWSource>/<Destinations>
    struct ETWSourceDescription;
    struct ETWSourceDestinationsDescription
    {
    public:
        ETWSourceDestinationsDescription();
        ETWSourceDestinationsDescription(ETWSourceDestinationsDescription const & other);
        ETWSourceDestinationsDescription(ETWSourceDestinationsDescription && other);

        ETWSourceDestinationsDescription const & operator = (ETWSourceDestinationsDescription const & other);
        ETWSourceDestinationsDescription const & operator = (ETWSourceDestinationsDescription && other);

        bool operator == (ETWSourceDestinationsDescription const & other) const;
        bool operator != (ETWSourceDestinationsDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();
		bool empty();

    public:
        std::vector<LocalStoreETWDescription> LocalStore;
        std::vector<FileStoreETWDescription> FileStore;
        std::vector<AzureBlobETWDescription> AzureBlob;

    private:
        friend struct ETWSourceDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
