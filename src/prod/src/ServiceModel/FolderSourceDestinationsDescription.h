// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>/<FolderSource>/<Destinations>
    struct FolderSourceDescription;
    struct FolderSourceDestinationsDescription
    {
    public:
        FolderSourceDestinationsDescription();
        FolderSourceDestinationsDescription(FolderSourceDestinationsDescription const & other);
        FolderSourceDestinationsDescription(FolderSourceDestinationsDescription && other);

        FolderSourceDestinationsDescription const & operator = (FolderSourceDestinationsDescription const & other);
        FolderSourceDestinationsDescription const & operator = (FolderSourceDestinationsDescription && other);

        bool operator == (FolderSourceDestinationsDescription const & other) const;
        bool operator != (FolderSourceDestinationsDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();
		bool empty();

    public:
        std::vector<LocalStoreDescription> LocalStore;
        std::vector<FileStoreDescription> FileStore;
        std::vector<AzureBlobDescription> AzureBlob;

    private:
        friend struct FolderSourceDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
