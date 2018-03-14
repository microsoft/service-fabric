// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>/<ETWSource>/<FileStore>
    struct ETWSourceDestinationsDescription;
    struct FileStoreETWDescription : FileStoreDescription
    {
    public:
        FileStoreETWDescription();
        FileStoreETWDescription(FileStoreETWDescription const & other);
        FileStoreETWDescription(FileStoreETWDescription && other);

        FileStoreETWDescription const & operator = (FileStoreETWDescription const & other);
        FileStoreETWDescription const & operator = (FileStoreETWDescription && other);

        bool operator == (FileStoreETWDescription const & other) const;
        bool operator != (FileStoreETWDescription const & other) const;

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
