// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>/.../<AzureBlob>
    struct CrashDumpSourceDestinationsDescription;
    struct FolderSourceDestinationsDescription;
    struct AzureBlobDescription : AzureStoreBaseDescription
    {
    public:
        AzureBlobDescription();
        AzureBlobDescription(AzureBlobDescription const & other);
        AzureBlobDescription(AzureBlobDescription && other);

        AzureBlobDescription const & operator = (AzureBlobDescription const & other);
        AzureBlobDescription const & operator = (AzureBlobDescription && other);

        bool operator == (AzureBlobDescription const & other) const;
        bool operator != (AzureBlobDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring ContainerName;

    protected:
        void ReadFromXmlWorker(Common::XmlReaderUPtr const & xmlReader);
		Common::ErrorCode WriteBaseToXml(Common::XmlWriterUPtr const & xmlWriter);
            
    private:
        friend struct CrashDumpSourceDestinationsDescription;
        friend struct FolderSourceDestinationsDescription;
		
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
        void ReadFromXml(Common::XmlReaderUPtr const &);
    };
}
