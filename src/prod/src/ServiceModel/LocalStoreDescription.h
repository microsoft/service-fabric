// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>/.../<LocalStore>
    struct CrashDumpSourceDestinationsDescription;
    struct FolderSourceDestinationsDescription;
    struct LocalStoreDescription
    {
    public:
        LocalStoreDescription();
        LocalStoreDescription(LocalStoreDescription const & other);
        LocalStoreDescription(LocalStoreDescription && other);

        LocalStoreDescription const & operator = (LocalStoreDescription const & other);
        LocalStoreDescription const & operator = (LocalStoreDescription && other);

        bool operator == (LocalStoreDescription const & other) const;
        bool operator != (LocalStoreDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring IsEnabled;
        std::wstring RelativeFolderPath;
        std::wstring DataDeletionAgeInDays;
        ParametersDescription Parameters;

    protected:
        void ReadFromXmlWorker(Common::XmlReaderUPtr const & xmlReader);
		Common::ErrorCode WriteBaseToXml(Common::XmlWriterUPtr const & xmlWritter);

    private:
        friend struct CrashDumpSourceDestinationsDescription;
        friend struct FolderSourceDestinationsDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
		
    };
}
