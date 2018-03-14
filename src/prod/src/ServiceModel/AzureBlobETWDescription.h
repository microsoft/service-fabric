// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>/<ETWSource>/<AzureBlob>
    struct ETWSourceDestinationsDescription;
    struct AzureBlobETWDescription : AzureBlobDescription
    {
    public:
        AzureBlobETWDescription();
        AzureBlobETWDescription(AzureBlobETWDescription const & other);
        AzureBlobETWDescription(AzureBlobETWDescription && other);

        AzureBlobETWDescription const & operator = (AzureBlobETWDescription const & other);
        AzureBlobETWDescription const & operator = (AzureBlobETWDescription && other);

        bool operator == (AzureBlobETWDescription const & other) const;
        bool operator != (AzureBlobETWDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring LevelFilter;

    private:
        friend struct ETWSourceDestinationsDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
