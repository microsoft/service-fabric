// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>/<CrashDumpSource>/<Destinations>
    struct CrashDumpSourceDescription;
    struct CrashDumpSourceDestinationsDescription
    {
    public:
        CrashDumpSourceDestinationsDescription();
        CrashDumpSourceDestinationsDescription(CrashDumpSourceDestinationsDescription const & other);
        CrashDumpSourceDestinationsDescription(CrashDumpSourceDestinationsDescription && other);

        CrashDumpSourceDestinationsDescription const & operator = (CrashDumpSourceDestinationsDescription const & other);
        CrashDumpSourceDestinationsDescription const & operator = (CrashDumpSourceDestinationsDescription && other);

        bool operator == (CrashDumpSourceDestinationsDescription const & other) const;
        bool operator != (CrashDumpSourceDestinationsDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();
		bool empty();

    public:
        std::vector<LocalStoreDescription> LocalStore;
        std::vector<FileStoreDescription> FileStore;
        std::vector<AzureBlobDescription> AzureBlob;
		
    private:
        friend struct CrashDumpSourceDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
