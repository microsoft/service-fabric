// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>/.../<FileStore>
    struct CrashDumpSourceDestinationsDescription;
    struct FolderSourceDestinationsDescription;
    struct FileStoreDescription
    {
    public:
        FileStoreDescription();
        FileStoreDescription(FileStoreDescription const & other);
        FileStoreDescription(FileStoreDescription && other);

        FileStoreDescription const & operator = (FileStoreDescription const & other);
        FileStoreDescription const & operator = (FileStoreDescription && other);

        bool operator == (FileStoreDescription const & other) const;
        bool operator != (FileStoreDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring IsEnabled;
        std::wstring Path;
        std::wstring UploadIntervalInMinutes;
        std::wstring DataDeletionAgeInDays;
        std::wstring AccountType;
        std::wstring AccountName;
        std::wstring Password;
        std::wstring PasswordEncrypted;
        ParametersDescription Parameters;

    protected:
        void ReadFromXmlWorker(Common::XmlReaderUPtr const & xmlReader);
		Common::ErrorCode WriteBaseToXml(Common::XmlWriterUPtr const &);
    private:
        friend struct CrashDumpSourceDestinationsDescription;
        friend struct FolderSourceDestinationsDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
		
    };
}
